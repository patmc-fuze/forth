// ForthVocabulary.h: interface for the ForthVocabulary class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORTHVOCABULARY_H__C43FADC1_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
#define AFX_FORTHVOCABULARY_H__C43FADC1_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"

class ForthVocabulary  
{
public:
	ForthVocabulary( ForthEngine *pEngine, int storageBytes );
	virtual ~ForthVocabulary();

    void                Empty( void );

    void                SetNextVocabulary( ForthVocabulary *pNext );
    ForthVocabulary     *GetNextVocabulary( void );

    static bool         IsExecutableType( forthOpType symType );

    long                AddSymbol( char *pSymName, forthOpType symType, long symValue );
    void                *FindSymbol( char *pSymName );

    void                SmudgeNewestSymbol( void );
    void                UnSmudgeNewestSymbol( void );

private:
    ForthEngine         *mpEngine;
    ForthVocabulary     *mpNext;
    char                mTmpSym[256];
    int                 mNumSymbols;
    ulong               mStorageLen;
    long                *mpStorageBase;
    long                *mpStorageTop;
    long                *mpNewestSymbol;

};

#endif // !defined(AFX_FORTHVOCABULARY_H__C43FADC1_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
