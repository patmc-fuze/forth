// ForthVocabulary.cpp: implementation of the ForthVocabulary class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthVocabulary.h"
#include "ForthEngine.h"
#include "ForthShell.h"

//############################################################################
//
//   vocabulary/symbol manipulation
//
//############################################################################


// symbol entry layout:
// offset   contents
//  0..3    4-byte symbol value - high byte is symbol type
//  4       symbol length (not including padding)
//  5..n    symbol characters
//
//  n is chosen so that the total symbol entry length is a multiple of 4,
//  padded with 0s if necessary.  This allows faster dictionary searches

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

ForthVocabulary::ForthVocabulary( ForthEngine   *pEngine,
                                  const char    *pName,
                                  int           valueLongs,
                                  int           storageBytes )
: mpEngine( pEngine )
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
    Empty();
}

ForthVocabulary::~ForthVocabulary()
{
    delete [] mpStorageBase;

    ForthVocabulary **mppNext = &mpChainHead;
    while ( mppNext != NULL ) {
        if ( *mppNext == this ) {
            *mppNext = mpChainNext;
            break;
        }
        mppNext = &((*mppNext)->mpChainNext);
    }

    if ( mpName != NULL ) {
        delete [] mpName;
    }
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


long
ForthVocabulary::AddSymbol( const char      *pSymName,
                            int             symType,
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
        symValue = mpEngine->AddOp( (long *) symValue );
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
    
    return symValue;
}


// copy a symbol table entry, presumably from another vocabulary
void
ForthVocabulary::CopyEntry( const void *pEntry )
{
    int numLongs;

    numLongs = (long *) NextEntry( pEntry ) - (long *) pEntry;
    mpStorageBottom -= numLongs;
    memcpy( mpStorageBottom, pEntry, numLongs * sizeof(long) );
    mNumSymbols++;
}


// delete single symbol entry
void
ForthVocabulary::DeleteEntry( void *pEntry )
{
    void *pNextEntry;
    int numLongs, entryLongs;
    long *pSrc, *pDst;

    pNextEntry = NextEntry( pEntry );
    entryLongs = (long *) pNextEntry - (long *) pEntry;
    if ( pEntry != mpStorageBottom ) {
        // there are newer symbols than entry, need to move newer symbols up
        pSrc = ((long *) pEntry) - 1;
        pDst = ((long *) pNextEntry) - 1;
        numLongs = ((long *) pEntry) - mpStorageBottom;
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
                pNewBottom = (long *) NextEntry( pEntry );
            } else {
                pEntry = (long *) NextEntry( pEntry );
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
    bool done;

    // go through the vocabulary looking for symbols which are greater than op#
    pEntry = mpStorageBottom;

    // new bottom of vocabulary after forget
    pNewBottom = NULL;
    // how many symbols are left after forget
    symbolsLeft = mNumSymbols;

    done = false;
    while ( !done && (symbolsLeft > 0) ) {

        opType = GetEntryType( pEntry );
        opVal = GetEntryValue( pEntry );
        if ( opVal < op ) {
            done = true;
            pNewBottom = (long *) pEntry;
        } else {
            symbolsLeft--;
            pEntry = (long *) NextEntry( pEntry );
        }
    }
    if ( pNewBottom != NULL ) {
        //
        // if we get here, really do the forget operation
        //
        mpStorageBottom = (long *) pNewBottom;
        mNumSymbols = symbolsLeft;
    }
}


// return ptr to vocabulary entry for symbol
void *
ForthVocabulary::FindSymbol( const char *pSymName )
{
    long tmpSym[SYM_MAX_LONGS];
    ForthParseInfo parseInfo( tmpSym, SYM_MAX_LONGS );

    parseInfo.SetToken( pSymName );

    return FindSymbol( &parseInfo );
}


// return ptr to vocabulary entry for symbol
void *
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
        pEntry = (long *) NextEntry( pEntry );
    }
    
    // symbol isn't in vocabulary
    return NULL;
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
///     ForthLocalVarVocabulary
//
//

ForthLocalVarVocabulary::ForthLocalVarVocabulary( ForthEngine   *pEngine,
                                                  const char    *pName,
                                                  int           storageBytes )
: ForthVocabulary( pEngine, pName, storageBytes )
{
}

ForthLocalVarVocabulary::~ForthLocalVarVocabulary()
{
}

// delete symbol entry and all newer entries
// return true IFF symbol was forgotten
bool
ForthLocalVarVocabulary::ForgetSymbol( const char *pSymName )
{
    return false;
}


// forget all ops with a greater op#
void
ForthLocalVarVocabulary::ForgetOp( long op )
{
}
