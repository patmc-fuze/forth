// ForthThread.cpp: implementation of the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthThread.h"

// this is the number of extra longs to allocate at top and
//    bottom of stacks
#define GAURD_AREA 4

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ForthThread::ForthThread( ForthEngine *pEngine, int paramStackSize, int returnStackSize )
{
    mpEngine = pEngine;

    mSLen = paramStackSize;
    // leave a few extra words above top of stacks, so that underflows don't
    //   tromp on the memory allocator info
    mSB = new long[mSLen + (GAURD_AREA * 2)];
    mSB += GAURD_AREA;
    mST = mSB + mSLen;

    mRLen = returnStackSize;
    mRB = new long[mRLen + (GAURD_AREA * 2)];
    mRB += GAURD_AREA;
    mRT = mRB + mRLen;
    Reset();
}

ForthThread::~ForthThread()
{
    delete [] (mSB - GAURD_AREA);
    delete [] (mRB - GAURD_AREA);
}

void
ForthThread::Reset( void )
{
    EmptySStack();
    EmptyRStack();
    mFP = NULL;

    mError = kForthErrorNone;
    mState = kResultDone;
    mVarMode = kVarFetch;
    mBase = 10;
    mSignedPrintMode = kPrintSignedDecimal;
    mpConOutFile = stdout;
    mpConOutStr = NULL;
}
