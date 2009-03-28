// ForthThread.cpp: implementation of the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthThread.h"
#include "ForthEngine.h"

// this is the number of extra longs to allocate at top and
//    bottom of stacks
#ifdef CHECK_GAURD_AREAS
#define GAURD_AREA 64
#else
#define GAURD_AREA 4
#endif

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

#ifdef CHECK_GAURD_AREAS
    long checkVal = 0x03020100;
    for ( int i = 0; i < 64; i++ )
    {
        mState.SB[i - GAURD_AREA] = checkVal;
        mState.RB[i - GAURD_AREA] = checkVal;
        mState.ST[i] = checkVal;
        mState.RT[i] = checkVal;
        checkVal += 0x04040404;
    }
#endif
    pEngine->ResetConsoleOut( &mState );

    Reset();
}

ForthThread::~ForthThread()
{
    delete [] (mState.SB - GAURD_AREA);
    delete [] (mState.RB - GAURD_AREA);
}

#ifdef CHECK_GAURD_AREAS
bool
ForthThread::CheckGaurdAreas( void )
{
    long checkVal = 0x03020100;
    bool retVal = false;
    for ( int i = 0; i < 64; i++ )
    {
        if ( mState.SB[i - GAURD_AREA] != checkVal )
        {
            return true;
        }
        if ( mState.RB[i - GAURD_AREA] != checkVal )
        {
            return true;
        }
        if ( mState.ST[i] != checkVal )
        {
            return true;
        }
        if ( mState.RT[i] != checkVal )
        {
            return true;
        }
        checkVal += 0x04040404;
    }
    return false;
}
#endif

void
ForthThread::Reset( void )
{
    mState.SP = mState.ST;
    mState.RP = mState.RT;
    mState.FP = NULL;
    mState.TPM = NULL;
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
    pCore->TPM      = mState.TPM;
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
    mState.TPM = pCore->TPM;
    mState.TPD = pCore->TPD;
    // NOTE: we don't copy ST, SLen, RT & RLen back into thread - they should not change
    mState.varMode = (varOperation) (pCore->varMode);
    mState.state = (eForthResult) (pCore->state);
    mState.error = (eForthError) (pCore->error);
}


