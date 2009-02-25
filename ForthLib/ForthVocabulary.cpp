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
, mpName( NULL )
, mValueLongs( valueLongs )
, mLastSerial( 0 )
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

    delete [] mpName;
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

int ForthVocabulary::GetEntryName( const long *pEntry, char *pDstBuff, int buffSize )
{
    int symLen = GetEntryNameLength( pEntry );
    int len = (symLen < buffSize) ? symLen : buffSize - 1;

    memcpy( pDstBuff, (void *) GetEntryName( pEntry ), len );
    pDstBuff[len] = '\0';
    return len;
}

void
ForthVocabulary::Empty( void )
{
    mNumSymbols = 0;
    mpStorageBottom = mpStorageBase + mStorageLongs;
    mpNewestSymbol = NULL;
}


#ifdef MAP_LOOKUP
void
ForthVocabulary::InitLookupMap( void )
{
    mLookupMap.RemoveAll();
    long* pEntry = mpStorageBottom;
    char buff[ 256 ];
    for ( int i = 0; i < mNumSymbols; i++ )
    {
        GetEntryName( pEntry, buff, sizeof(buff) );
        mLookupMap.SetAt( buff, pEntry );
        pEntry = NextEntry( pEntry );
    }
}
#endif

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
#ifdef MAP_LOOKUP
        InitLookupMap();
#endif
    }

    if ( addToEngineOps ){        
        // for executable ops, add the IP of symbol to op table
        // value of symbol is index into op table, not IP
        symValue = mpEngine->AddOp( (long *) symValue, (forthOpType) symType );
    }
    
    SPEW_VOCABULARY( "Adding symbol %s type %d value 0x%x to %s\n",
        pSymName, (int) symType, symValue, GetName() );

    mpStorageBottom = pBase;
#ifdef MAP_LOOKUP
    mLookupMap.SetAt( pSymName, mpStorageBottom );
#endif
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
#ifdef MAP_LOOKUP
    InitLookupMap();
#endif
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
#ifdef MAP_LOOKUP
    InitLookupMap();
#endif
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
        if ( (opType == kOpBuiltIn) || (opType == kOpBuiltInImmediate) ) {
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
#ifdef MAP_LOOKUP
        InitLookupMap();
#endif
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

    while ( symbolsLeft > 0 )
    {
        symbolsLeft--;
        opType = GetEntryType( pEntry );
        switch ( opType )
        {
            case kOpBuiltIn:
            case kOpBuiltInImmediate:
                // can't forget builtin ops
                symbolsLeft = 0;
                break;

            case kOpUserDef:
            case kOpUserDefImmediate:
            case kOpUserCode:
            case kOpUserCodeImmediate:
                opVal = GetEntryValue( pEntry );
                if ( opVal >= op )
                {
                    pEntry = NextEntry( pEntry );
                    mpStorageBottom = pEntry;
                    mNumSymbols = symbolsLeft;
                }
                else
                {
                    // this symbol was defined before the forgotten op, so we are done with this vocab
                    symbolsLeft = 0;
                }
                break;

             case kOpConstant:
                opVal = CODE_TO_STRUCT_INDEX( pEntry[1] );
                if ( opVal >= op )
                {
                    pEntry = NextEntry( pEntry );
                    mpStorageBottom = pEntry;
                    mNumSymbols = symbolsLeft;
                }
                else
                {
                    // this symbol was defined before the forgotten op, so we are done with this vocab
                    symbolsLeft = 0;
                }
                break;

            case kOpDLLEntryPoint:
                opVal = CODE_TO_DLL_ENTRY_INDEX( GetEntryValue( pEntry ) );
                if ( opVal >= op )
                {
                    pEntry = NextEntry( pEntry );
                    mpStorageBottom = pEntry;
                    mNumSymbols = symbolsLeft;
                }
                else
                {
                    // this symbol was defined before the forgotten op, so we are done with this vocab
                    symbolsLeft = 0;
                }
                break;

             default:
                break;
        }
    }
}


// return ptr to vocabulary entry for symbol
long *
ForthVocabulary::FindSymbol( const char *pSymName, ulong serial )
{
    long tmpSym[SYM_MAX_LONGS];
    ForthParseInfo parseInfo( tmpSym, SYM_MAX_LONGS );

#ifdef MAP_LOOKUP
    void *pEntryVoid;
    if ( mLookupMap.Lookup( pSymName, pEntryVoid ) )
    {
        return (long *) pEntryVoid;
    }
#endif
    parseInfo.SetToken( pSymName );

    return FindSymbol( &parseInfo, serial );
}


// return ptr to vocabulary entry for symbol
long *
ForthVocabulary::FindSymbol( ForthParseInfo *pInfo, ulong serial )
{
    int i, j, symLen;
    long *pEntry, *pTmp;
    long *pToken;

    if ( (serial != 0) && (serial == mLastSerial) )
    {
        // this vocabulary was already searched
        return NULL;
    }

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
    mLastSerial = serial;
    return NULL;
}

// return ptr to vocabulary entry given its value
long *
ForthVocabulary::FindSymbolByValue( long val, ulong serial )

{
    int i;
    long *pEntry;

    if ( (serial != 0) && (serial == mLastSerial) )
    {
        // this vocabulary was already searched
        return NULL;
    }

    // go through the vocabulary looking for match with value
    pEntry = mpStorageBottom;
    for ( i = 0; i < mNumSymbols; i++ ) {
        if ( *pEntry == val )
        {
            // found it
            return pEntry;
        }
        pEntry = NextEntry( pEntry );
    }
    
    // symbol isn't in vocabulary
    mLastSerial = serial;
    return NULL;
}

// process symbol entry
eForthResult
ForthVocabulary::ProcessEntry( long* pEntry )
{
    eForthResult exitStatus = kResultOk;
    bool compileIt = false;
    if ( mpEngine->IsCompiling() )
    {
        switch ( FORTH_OP_TYPE( *pEntry ) )
        {
            case kOpBuiltInImmediate:
            case kOpUserDefImmediate:
            case kOpUserCodeImmediate:
                break;
            default:
                compileIt = true;
        }
    }
    if ( compileIt )
    {
        mpEngine->CompileOpcode( *pEntry );
    }
    else
    {
        // execute the opcode
        exitStatus = mpEngine->ExecuteOneOp( *pEntry );
    }

    return exitStatus;
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

const char*
ForthVocabulary::GetType( void )
{
    return "userOp";
}


void
ForthVocabulary::DoOp( ForthCoreState *pCore )
{
    long* pEntry;
    char* pSymbol;
    ulong opVal, opType;
    bool addToEngineOps;
    ForthVocabularyStack* pVocabStack;

    // IP points to data field
    switch ( GET_VAR_OPERATION )
    {
        case kVocabSetCurrent:
            pVocabStack = mpEngine->GetVocabularyStack();
            pVocabStack->SetTop( this );
            break;

        case kVocabNewestEntry:
            SPUSH( (long) mpNewestSymbol );
            break;

        case kVocabFindEntry:
            pSymbol = (char *) (SPOP);
            pEntry = FindSymbol( pSymbol );
            SPUSH( (long) pEntry );
            break;

        case kVocabFindEntryValue:
            opVal = SPOP;
            pEntry = FindSymbolByValue( (long) opVal );
            SPUSH( (long) pEntry );
            break;

        case kVocabAddEntry:
            opVal = SPOP;
            opType = SPOP;
            pSymbol = (char *) (SPOP);
            addToEngineOps = (opType <= kOpDLLEntryPoint);
            AddSymbol( pSymbol, opType, opVal, addToEngineOps );
            break;

        case kVocabRemoveEntry:
            pEntry = (long *) (SPOP);
            DeleteEntry( pEntry );
            break;

        case kVocabEntryLength:
            SPUSH( mValueLongs );
            break;

        case kVocabNumEntries:
            SPUSH( mNumSymbols );
            break;

        default:
            mpEngine->SetError( kForthErrorBadVarOperation, " vocabulary operation out of range" );
            break;
    }
    CLEAR_VAR_OPERATION;
}


void
ForthVocabulary::PrintEntry( long*   pEntry )
{
#define BUFF_SIZE 256
    char buff[BUFF_SIZE];
    ForthCoreState* pCore = mpEngine->GetCoreState();
    sprintf( buff, "%02x:%06x    ", GetEntryType( pEntry ), GetEntryValue( pEntry ) );
    CONSOLE_STRING_OUT( buff );

    for ( int j = 1; j < mValueLongs; j++ )
    {
        sprintf( buff, "%08x    ", pEntry[j] );
        CONSOLE_STRING_OUT( buff );
    }

    GetEntryName( pEntry, buff, BUFF_SIZE );
    CONSOLE_STRING_OUT( buff );
}

//////////////////////////////////////////////////////////////////////
////
///     ForthLocalVocabulary
//
//

ForthLocalVocabulary::ForthLocalVocabulary( const char    *pName,
                                                      int           valueLongs,
                                                      int           storageBytes )
: ForthVocabulary( pName, valueLongs, storageBytes )
{
}

ForthLocalVocabulary::~ForthLocalVocabulary()
{
}

const char*
ForthLocalVocabulary::GetType( void )
{
    return "local";
}

//////////////////////////////////////////////////////////////////////
////
///     ForthDLLVocabulary
//
//

ForthDLLVocabulary::ForthDLLVocabulary( const char      *pName,
                                        const char      *pDLLName,
                                        int             valueLongs,
                                        int             storageBytes,
                                        void*           pForgetLimit,
                                        long            op )
: ForthVocabulary( pName, valueLongs, storageBytes, pForgetLimit, op )
{
    int len = strlen( pDLLName );
    mpDLLName = new char[len + 1];
    strcpy( mpDLLName, pDLLName );

    mhDLL = LoadLibrary( mpDLLName );
}

ForthDLLVocabulary::~ForthDLLVocabulary()
{
    if ( mhDLL != 0 )
    {
        UnloadDLL();
    }
    delete [] mpDLLName;
}

long ForthDLLVocabulary::LoadDLL( void )
{
    mhDLL = LoadLibrary( mpDLLName );
    return (long) mhDLL;
}

void ForthDLLVocabulary::UnloadDLL( void )
{
    FreeLibrary( mhDLL );
    mhDLL = 0;
}

long * ForthDLLVocabulary::AddEntry( const char *pFuncName, long numArgs )
{
    long *pEntry = NULL;
    long pFunc = (long) GetProcAddress( mhDLL, pFuncName );
    if ( pFunc )
    {
        pEntry = AddSymbol( pFuncName, kOpDLLEntryPoint, pFunc, true );
        // the opcode at this point is just the opType and dispatch table index or-ed together
        // we need to combine in the argument count
        *pEntry |= (numArgs << 19);
    }
    else
    {
        mpEngine->SetError( kForthErrorUnknownSymbol, " unknown entry point" );
    }

    return pEntry;
}

const char*
ForthDLLVocabulary::GetType( void )
{
    return "dllOp";
}

//////////////////////////////////////////////////////////////////////
////
///     ForthVocabularyStack
//
//

ForthVocabularyStack::ForthVocabularyStack( int maxDepth )
: mStack( NULL )
, mMaxDepth( maxDepth )
, mTop( 0 )
, mSerial( 0 )
{
    mpEngine = ForthEngine::GetInstance();
}

ForthVocabularyStack::~ForthVocabularyStack()
{
    delete mStack;
}

void ForthVocabularyStack::Initialize( void )
{
    delete mStack;
    mTop = 0;
    mStack = new ForthVocabulary* [ mMaxDepth ];
}

void ForthVocabularyStack::DupTop( void )
{
    if ( mTop < (mMaxDepth - 1) )
    {
        mTop++;
        mStack[ mTop ] = mStack[ mTop - 1 ];
    }
    else
    {
        // TBD: report overflow
    }
}

bool ForthVocabularyStack::DropTop( void )
{
    if ( mTop )
    {
        mTop--;
    }
    else
    {
        return false;
    }
    return true;
}

void ForthVocabularyStack::Clear( void )
{
    mTop = 0;
    mStack[0] = mpEngine->GetForthVocabulary();
//    mStack[1] = mpEngine->GetPrecedenceVocabulary();
}

void ForthVocabularyStack::SetTop( ForthVocabulary* pVocab )
{
    mStack[mTop] = pVocab;
}

ForthVocabulary* ForthVocabularyStack::GetTop( void )
{
    return mStack[mTop];
}

ForthVocabulary* ForthVocabularyStack::GetElement( int depth )
{
    return (depth > mTop) ? NULL : mStack[mTop - depth];
}

// return pointer to symbol entry, NULL if not found
// ppFoundVocab will be set to the vocabulary the symbol was actually found in
// set ppFoundVocab to NULL to search just this vocabulary (not the search chain)
long * ForthVocabularyStack::FindSymbol( const char *pSymName, ForthVocabulary** ppFoundVocab )
{
    long *pEntry = NULL;

    mSerial++;
    for ( int i = mTop; i >= 0; i-- )
    {
        pEntry = mStack[i]->FindSymbol( pSymName, mSerial );
        if ( pEntry )
        {
            if ( ppFoundVocab != NULL )
            {
                *ppFoundVocab = mStack[i];
            }
            break;
        }
    }
    return pEntry;
}

// return pointer to symbol entry, NULL if not found, given its value
long * ForthVocabularyStack::FindSymbolByValue( long val, ForthVocabulary** ppFoundVocab )
{
    long *pEntry = NULL;

    mSerial++;
    for ( int i = mTop; i >= 0; i-- )
    {
        pEntry = mStack[i]->FindSymbolByValue( val, mSerial );
        if ( pEntry )
        {
            if ( ppFoundVocab != NULL )
            {
                *ppFoundVocab = mStack[i];
            }
            break;
        }
    }
    return pEntry;
}

// return pointer to symbol entry, NULL if not found
// pSymName is required to be a longword aligned address, and to be padded with 0's
// to the next longword boundary
long * ForthVocabularyStack::FindSymbol( ForthParseInfo *pInfo, ForthVocabulary** ppFoundVocab )
{
    long *pEntry = NULL;

    mSerial++;
    for ( int i = mTop; i >= 0; i-- )
    {
        pEntry = mStack[i]->FindSymbol( pInfo, mSerial );
        if ( pEntry )
        {
            if ( ppFoundVocab != NULL )
            {
                *ppFoundVocab = mStack[i];
            }
            break;
        }
    }
    return pEntry;
}
