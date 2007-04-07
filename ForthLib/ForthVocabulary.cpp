//////////////////////////////////////////////////////////////////////
//
// ForthVocabulary.cpp: implementation of the ForthVocabulary class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthVocabulary.h"
#include "ForthEngine.h"
#include "ForthShell.h"
#include "ForthForgettable.h"

//############################################################################
//
//   vocabulary/symbol manipulation
//
//############################################################################


// symbol entry layout:
// offset   contents
//  0..3    4-byte symbol value - high byte is symbol type - usually this is opcode for symbol
//  ...     0 or more extra symbol value fields
//  N       1-byte symbol length (not including padding)
//  N+1     symbol characters
//
//  The total symbol entry length is always a multiple of 4, padded with 0s
//  if necessary.  This allows faster dictionary searches.

// vocabulary symbol storage grows downward, and is searched from the
// bottom upward (try to match newest symbols first)

#define SYM_MAX_LONGS 64

// this many longs are added to vocab storage whenever it runs out of room
#define VOCAB_EXPANSION_INCREMENT   128

// this is head of a chain which links all vocabs
ForthVocabulary *ForthVocabulary::mpChainHead = NULL;

//////////////////////////////////////////////////////////////////////
////
///     ForthVocabulary
//
//

ForthVocabulary::ForthVocabulary( const char    *pName,
                                  int           valueLongs,
                                  int           storageBytes,
                                  void*         pForgetLimit,
                                  long          op )
: ForthForgettable( pForgetLimit, op )
, mpSearchNext( NULL )
, mpName( NULL )
, mValueLongs( valueLongs ) 
{
    mStorageLongs = ((storageBytes + 3) & ~3) >> 2;
    mpStorageBase = new long[mStorageLongs];

    // add this vocab to linked list of all vocabs
    mpChainNext = mpChainHead;
    mpChainHead = this;

    if ( pName != NULL ) {
        SetName( pName );
    }
    mpEngine = ForthEngine::GetInstance();
    Empty();
}

ForthVocabulary::~ForthVocabulary()
{
    delete [] mpStorageBase;

    ForthVocabulary **ppNext = &mpChainHead;
    while ( ppNext != NULL ) {
        if ( *ppNext == this ) {
            *ppNext = mpChainNext;
            break;
        }
        ppNext = &((*ppNext)->mpChainNext);
    }

    if ( mpName != NULL ) {
        delete [] mpName;
    }
}


void
ForthVocabulary::ForgetCleanup( void *pForgetLimit, long op )
{
   // this is invoked from the ForthForgettable chain to propagate a "forget"
   ForgetOp( op );
}


void
ForthVocabulary::SetName( const char *pVocabName )
{
    int len;

    if ( pVocabName != NULL ) {
        len = strlen( pVocabName );
        mpName = new char[len + 1];
        strcpy( mpName, pVocabName );
    }
}


char *
ForthVocabulary::GetName( void )
{
    return (mpName == NULL) ? "Unknown" : mpName;
}

void
ForthVocabulary::Empty( void )
{
    mNumSymbols = 0;
    mpStorageBottom = mpStorageBase + mStorageLongs;
    mpNewestSymbol = NULL;
}

void
ForthVocabulary::SetNextSearchVocabulary( ForthVocabulary *pNextVocab )
{
    mpSearchNext = pNextVocab;
}

ForthVocabulary *
ForthVocabulary::GetNextSearchVocabulary( void )
{
    return mpSearchNext;
}


long *
ForthVocabulary::AddSymbol( const char      *pSymName,
                            long            symType,
                            long            symValue,
                            bool            addToEngineOps )
{
    char *pVC;
    long *pBase, *pSrc, *pDst;
    int i, nameLen, symSize, newLen;

    nameLen = (pSymName == NULL) ? 0 : strlen( pSymName );
    if ( nameLen > 255 ) {
        nameLen = 255;
    }
    else
    {
        strcpy( mNewestSymbol, pSymName );
    }

    symSize = mValueLongs + ( ((nameLen + 4) & ~3) >> 2 );
    pBase = mpStorageBottom - symSize;
    if ( pBase < mpStorageBase ) {
        //
        // new symbol wont fit, increase storage size
        //
        newLen = mStorageLongs + VOCAB_EXPANSION_INCREMENT;
        SPEW_VOCABULARY( "Increasing %s vocabulary size to %d longs\n", GetName(), newLen );
        pBase = new long[newLen];
        pSrc = mpStorageBase + mStorageLongs;
        pDst = pBase + newLen;
        // copy existing entries into new storage
        while ( pSrc > mpStorageBottom ) {
            *--pDst = *--pSrc;
        }

        delete [] mpStorageBase;
        mpStorageBottom = pDst;
        mpStorageBase = pBase;
        mStorageLongs = newLen;
        pBase = mpStorageBottom - symSize;
    }

    if ( addToEngineOps ){        
        // for executable ops, add the IP of symbol to op table
        // value of symbol is index into op table, not IP
        symValue = mpEngine->AddOp( (long *) symValue, (forthOpType) symType );
    }
    
    SPEW_VOCABULARY( "Adding symbol %s type %d value 0x%x to %s\n",
        pSymName, (int) symType, symValue, GetName() );

    mpStorageBottom = pBase;
    symValue += ((int) symType << 24);
    mpNewestSymbol = mpStorageBottom;
    // TBD: check for storage overflow
    *pBase++ = symValue;
    for ( i = 1; i < mValueLongs; i++ ) {
        *pBase++ = 0;
    }
    pVC = (char *) pBase;
    *pVC++ = nameLen;
    for ( i = 0; i < nameLen; i++ ) {
        *pVC++ = *pSymName++;
    }
    // pad with 0s to make the total symbol entry a multiple of longwords long
    nameLen++;
    while ( (nameLen & 3) != 0 ) {
        *pVC++ = 0;
        nameLen++;
    }
    
    assert( (((ulong) pVC) & 3) == 0 );
    
    mNumSymbols++;
    
    return mpStorageBottom;
}


// copy a symbol table entry, presumably from another vocabulary
void
ForthVocabulary::CopyEntry( long *pEntry )
{
    int numLongs;

    numLongs = NextEntry( pEntry ) - pEntry;
    mpStorageBottom -= numLongs;
    memcpy( mpStorageBottom, pEntry, numLongs * sizeof(long) );
    mNumSymbols++;
}


// delete single symbol entry
void
ForthVocabulary::DeleteEntry( long *pEntry )
{
    long *pNextEntry;
    int numLongs, entryLongs;
    long *pSrc, *pDst;

    pNextEntry = NextEntry( pEntry );
    entryLongs = pNextEntry - pEntry;
    if ( pEntry != mpStorageBottom ) {
        // there are newer symbols than entry, need to move newer symbols up
        pSrc = pEntry - 1;
        pDst = pNextEntry - 1;
        numLongs = pEntry - mpStorageBottom;
        while ( numLongs-- > 0 ) {
            *pDst-- = *pSrc--;
        }
    }
    mpStorageBottom += entryLongs;
    mNumSymbols--;
}


// delete symbol entry and all newer entries
// return true IFF symbol was forgotten
bool
ForthVocabulary::ForgetSymbol( const char *pSymName )
{
    int j, symLen, symbolsLeft;
    long *pEntry, *pTmp, *pNewBottom;
    forthOpType opType;
    bool done;
    long tmpSym[SYM_MAX_LONGS];
    ForthParseInfo parseInfo( tmpSym, SYM_MAX_LONGS );

    parseInfo.SetToken( pSymName );

    symLen = parseInfo.GetNumLongs();

    // go through the vocabulary looking for match with name
    pEntry = mpStorageBottom;

    // new bottom of vocabulary after forget
    pNewBottom = NULL;
    // how many symbols are left after forget
    symbolsLeft = mNumSymbols;

    done = false;
    while ( !done && (symbolsLeft > 0) ) {

        opType = GetEntryType( pEntry );
        if ( opType == kOpBuiltIn ) {
            // sym is unknown, or in built-in ops - no way
            TRACE( "Error - attempt to forget builtin op %s from %s\n", pSymName, GetName() );
            done = true;
        } else {
            // skip value field
            pTmp = pEntry + 1;
            for ( j = 0; j < symLen; j++ ) {
                if ( pTmp[j] != tmpSym[j] ) {
                    // not a match
                    break;
                }
            }
            symbolsLeft--;
            if ( j == symLen ) {
                // found it
                done = true;
                pNewBottom = NextEntry( pEntry );
            } else {
                pEntry = NextEntry( pEntry );
            }
        }
    }
    if ( pNewBottom != NULL ) {
        //
        // if we get here, really do the forget operation
        //
        mpStorageBottom = (long *) pNewBottom;
        mNumSymbols = symbolsLeft;
        return true;
    }

    return false;
}


// forget all ops with a greater op#
void
ForthVocabulary::ForgetOp( long op )
{
    int symbolsLeft;
    long *pEntry, *pNewBottom;
    forthOpType opType;
    long opVal;

    // go through the vocabulary looking for symbols which are greater than op#
    pEntry = mpStorageBottom;

    // new bottom of vocabulary after forget
    pNewBottom = NULL;
    // how many symbols are left after forget
    symbolsLeft = mNumSymbols;

    while ( symbolsLeft > 0) {

        opType = GetEntryType( pEntry );
        if ( opType == kOpBuiltIn )
        {
            // can't forget builtin ops
            break;
        }
        opVal = GetEntryValue( pEntry );
        if ( opVal >= op )
        {
            symbolsLeft--;
            pEntry = NextEntry( pEntry );
            mpStorageBottom = pEntry;
            mNumSymbols = symbolsLeft;
        }
        else
        {
            break;
        }
    }
}


// return ptr to vocabulary entry for symbol
long *
ForthVocabulary::FindSymbol( const char *pSymName )
{
    long tmpSym[SYM_MAX_LONGS];
    ForthParseInfo parseInfo( tmpSym, SYM_MAX_LONGS );

    parseInfo.SetToken( pSymName );

    return FindSymbol( &parseInfo );
}


// return ptr to vocabulary entry for symbol
long *
ForthVocabulary::FindSymbol( ForthParseInfo     *pInfo )
{
    int i, j, symLen;
    long *pEntry, *pTmp;
    long *pToken;

    pToken = pInfo->GetTokenAsLong();
    symLen = pInfo->GetNumLongs();
    
    // go through the vocabulary looking for match with name
    pEntry = mpStorageBottom;
    for ( i = 0; i < mNumSymbols; i++ ) {
        // skip value field
        pTmp = pEntry + mValueLongs;
        for ( j = 0; j < symLen; j++ ) {
            if ( pTmp[j] != pToken[j] ) {
                // not a match
                break;
            }
        }
        if ( j == symLen ) {
            // found it
            return pEntry;
        }
        pEntry = NextEntry( pEntry );
    }
    
    // symbol isn't in vocabulary
    return NULL;
}


// lookup symbol, and if found, compile or execute corresponding opcode
// return vocab entry pointer if found, else NULL
// execute opcode in context of thread g
// exitStatus is only set if opcode is executed
long *
ForthVocabulary::ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus )
{
    long *pEntry = FindSymbol( pInfo );
    if ( pEntry != NULL )
    {
        if ( mpEngine->IsCompiling() )
        {
            mpEngine->CompileOpcode( *pEntry );
        }
        else
        {
            // execute the opcode
            exitStatus = mpEngine->ExecuteOneOp( *pEntry );
            if ( exitStatus == kResultDone )
            {
                exitStatus = kResultOk;
            }
        }
    }
    return pEntry;
}


void
ForthVocabulary::SmudgeNewestSymbol( void )
{
    // smudge by setting highest bit of 1st character of name
    assert( mpNewestSymbol != NULL );
    ((char *) mpNewestSymbol)[1 + (mValueLongs << 2)] |= 0x80;
}


void
ForthVocabulary::UnSmudgeNewestSymbol( void )
{
    // unsmudge by clearing highest bit of 1st character of name
    assert( mpNewestSymbol != NULL );
    ((char *) mpNewestSymbol)[1 + (mValueLongs << 2)] &= 0x7F;
}


//////////////////////////////////////////////////////////////////////
////
///     ForthPrecedenceVocabulary
//
//

ForthPrecedenceVocabulary::ForthPrecedenceVocabulary( const char    *pName,
                                                      int           valueLongs,
                                                      int           storageBytes )
: ForthVocabulary( pName, valueLongs, storageBytes )
{
}

ForthPrecedenceVocabulary::~ForthPrecedenceVocabulary()
{
}

// lookup symbol, and if found, execute corresponding opcode
// return vocab entry pointer if found, else NULL
// execute opcode in context of thread g
// exitStatus is only set if opcode is executed
long *
ForthPrecedenceVocabulary::ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus )
{
    long *pEntry = FindSymbol( pInfo );
    if ( pEntry != NULL )
    {
        // execute the opcode
        exitStatus = mpEngine->ExecuteOneOp( *pEntry );
        if ( exitStatus == kResultDone )
        {
            exitStatus = kResultOk;
        }
    }
    return pEntry;
}


#if 0
//////////////////////////////////////////////////////////////////////
////
///     ForthLocalsVocabulary
//
//

ForthLocalsVocabulary::ForthLocalsVocabulary( ForthEngine   *pEngine,
                                                  int           storageBytes )
: ForthVocabulary( pEngine, "locals", NUM_LOCALS_VOCAB_VALUE_LONGS, storageBytes )
{
}

ForthLocalsVocabulary::~ForthLocalsVocabulary()
{
}

// delete symbol entry and all newer entries
// return true IFF symbol was forgotten
bool
ForthLocalsVocabulary::ForgetSymbol( const char *pSymName )
{
    return false;
}


// forget all ops with a greater op#
void
ForthLocalsVocabulary::ForgetOp( long op )
{
}

#endif

