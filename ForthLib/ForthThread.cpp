// ForthThread.cpp: implementation of the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthThread.h"
#include "ForthEngine.h"

// this is the number of extra longs to allocate at top and
//    bottom of stacks
#define GAURD_AREA 4

extern "C" {
    extern void consoleOutToFile( ForthCoreState   *pCore,  const char       *pMessage );
};

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthThread
// 

ForthThread::ForthThread( ForthEngine *pEngine, int paramStackLongs, int returnStackLongs )
: mpEngine( pEngine )
{
    mState.SLen = paramStackLongs;
    mState.RLen = returnStackLongs;
    mState.pPrivate = NULL;
    // leave a few extra words above top of stacks, so that underflows don't
    //   tromp on the memory allocator info
    mState.SB = new long[mState.SLen + (GAURD_AREA * 2)];
    mState.SB += GAURD_AREA;
    mState.ST = mState.SB + mState.SLen;

    mState.RB = new long[mState.RLen + (GAURD_AREA * 2)];
    mState.RB += GAURD_AREA;
    mState.RT = mState.RB + mState.RLen;

	pEngine->ResetConsoleOut( &mState );

    Reset();
}

ForthThread::~ForthThread()
{
    delete [] (mState.SB - GAURD_AREA);
    delete [] (mState.RB - GAURD_AREA);
}

void
ForthThread::Reset( void )
{
    mState.SP = mState.ST;
    mState.RP = mState.RT;
    mState.FP = NULL;
    mState.TPV = NULL;
    mState.TPD = NULL;

    mState.error = kForthErrorNone;
    mState.state = kResultDone;
    mState.varMode = kVarFetch;
    mState.base = 10;
    mState.signedPrintMode = kPrintSignedDecimal;
}


void
ForthThread::Activate( ForthCoreState* pCore )
{
    pCore->IP       = mState.IP;
    pCore->SP       = mState.SP;
    pCore->ST       = mState.ST;
    pCore->SLen     = mState.SLen;
    pCore->RP       = mState.RP;
    pCore->RT       = mState.RT;
    pCore->RLen     = mState.RLen;
    pCore->FP       = mState.FP;
    pCore->TPV      = mState.TPV;
    pCore->TPD      = mState.TPD;
    pCore->varMode  = mState.varMode;
    pCore->state    = mState.state;
    pCore->error    = mState.error;
    pCore->pThread  = &mState;
}


void ForthThread::Deactivate( ForthCoreState* pCore )
{
    mState.IP = pCore->IP;
    mState.SP = pCore->SP;
    mState.RP = pCore->RP;
    mState.FP = pCore->FP;
    mState.TPV = pCore->TPV;
    mState.TPD = pCore->TPD;
    // NOTE: we don't copy ST, SLen, RT & RLen back into thread - they should not change
    mState.varMode = (varOperation) (pCore->varMode);
    mState.state = (eForthResult) (pCore->state);
    mState.error = (eForthError) (pCore->error);
}


