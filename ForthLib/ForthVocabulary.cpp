// ForthVocabulary.cpp: implementation of the ForthVocabulary class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthVocabulary.h"

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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ForthVocabulary::ForthVocabulary( ForthEngine   *pEngine,
                                  int           storageBytes )
{
    mpEngine = pEngine;
    mStorageLen = (storageBytes + 3) & ~3;
    mpStorageBase = new long[mStorageLen >> 2];
    Empty();
}

ForthVocabulary::~ForthVocabulary()
{
    delete [] mpStorageBase;
}

void
ForthVocabulary::Empty( void )
{
    mNumSymbols = 0;
    mpStorageTop = mpStorageBase;
    mpNext = NULL;
    mpNewestSymbol = NULL;
}

void
ForthVocabulary::SetNextVocabulary( ForthVocabulary *pNextVocab )
{
    mpNext = pNextVocab;
}

ForthVocabulary *
ForthVocabulary::GetNextVocabulary( void )
{
    return mpNext;
}


bool
ForthVocabulary::IsExecutableType( forthOpType      symType )
{
    switch( symType ) {

    case kOpBuiltIn:
    case kOpUserDef:
    case kOpBuiltInPrec:
    case kOpUserDefPrec:
        return true;

    default:
        return false;

    }
}


long
ForthVocabulary::AddSymbol( char             *pSymName,
                            forthOpType      symType,
                            long             symValue )
{
    char *pVC;
    int i, nameLen;

    if ( IsExecutableType( symType ) ){        
        // for executable ops, add the IP of symbol to op table
        // value of symbol is index into op table, not IP
        symValue = mpEngine->AddOp( (long *) symValue );
    }
    
    TRACE( "Adding symbol %s type %d value 0x%x\n",
        pSymName, (int) symType, symValue );
    nameLen = (pSymName == NULL) ? 0 : strlen( pSymName );
    if ( nameLen > 255 ) {
        nameLen = 255;
    }
    symValue += ((int) symType << 24);
    mpNewestSymbol = mpStorageTop;
    // TBD: check for storage overflow
    *mpStorageTop++ = symValue;
    pVC = (char *) mpStorageTop;
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
    
    mpStorageTop = (long *) pVC;
    assert( (((ulong) mpStorageTop) & 3) == 0 );
    
    mNumSymbols++;
    
    return symValue;
}


// return ptr to vocabulary entry for symbol
void *
ForthVocabulary::FindSymbol( char   *pSymName )
{
    int i, j, symLen, nextLen;
    ulong *pNextEntry, *pTmp;
    
    symLen = strlen( pSymName );
    if ( symLen > 255 ) {
        symLen = 255;
    }
    
    // make copy of symbol in mTmpSym
    mTmpSym[0] = symLen;
    memcpy( mTmpSym + 1, pSymName, symLen );
    symLen++;
    if ( symLen < 255 ) {
        // pad length out to even number of longwords
        while ( (symLen & 3) != 0 ) {
            mTmpSym[symLen] = 0;
            symLen++;
        }
    }
    // convert from char count to longword count
    symLen >>= 2;
    
    // go through the vocabulary looking for match with name
    pNextEntry = (ulong *) mpStorageBase;
    for ( i = 0; i < (int) mNumSymbols; i++ ) {
        // skip value field
        pTmp = pNextEntry + 1;
        // add in 1 for length, 4 for the value field, and 3 for longword rounding
        nextLen = ((*((char *) pTmp)) + 8) >> 2;
        for ( j = 0; j < symLen; j++ ) {
            if ( pTmp[j] != ((ulong *) mTmpSym)[j] ) {
                // not a match
                break;
            }
        }
        if ( j == symLen ) {
            // found it
            return pNextEntry;
        }
        pNextEntry += nextLen;
    }
    
    // symbol isn't in vocabulary
    return NULL;
}


void
ForthVocabulary::SmudgeNewestSymbol( void )
{
    // smudge by setting highest bit of 1st character of name
    assert( mpNewestSymbol != NULL );
    ((char *) mpNewestSymbol)[5] |= 0x80;
}


void
ForthVocabulary::UnSmudgeNewestSymbol( void )
{
    // unsmudge by clearing highest bit of 1st character of name
    assert( mpNewestSymbol != NULL );
    ((char *) mpNewestSymbol)[5] &= 0x7F;
}



