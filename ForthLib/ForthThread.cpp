// ForthThread.cpp: implementation of the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#endif
#include "ForthThread.h"
#include "ForthEngine.h"
#include "ForthShowContext.h"
#include "ForthBuiltinClasses.h"

// this is the number of extra longs to allocate at top and
//    bottom of stacks
#ifdef CHECK_GAURD_AREAS
#define GAURD_AREA 64
#else
#define GAURD_AREA 4
#endif

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthThread
// 

ForthThread::ForthThread( ForthEngine *pEngine, int paramStackLongs, int returnStackLongs )
: mpEngine( pEngine )
, mWakeupTime( 0 )
, mpNext( NULL )
, mHandle( 0 )
, mThreadId( 0 )
, mpPrivate( NULL )
, mpShowContext(NULL)
{
    mCore.pThread = this;
    mCore.SLen = paramStackLongs;
    mCore.RLen = returnStackLongs;
    // leave a few extra words above top of stacks, so that underflows don't
    //   tromp on the memory allocator info
    mCore.SB = new long[mCore.SLen + (GAURD_AREA * 2)];
    mCore.SB += GAURD_AREA;
    mCore.ST = mCore.SB + mCore.SLen;

    mCore.RB = new long[mCore.RLen + (GAURD_AREA * 2)];
    mCore.RB += GAURD_AREA;
    mCore.RT = mCore.RB + mCore.RLen;

#ifdef CHECK_GAURD_AREAS
    long checkVal = 0x03020100;
    for ( int i = 0; i < 64; i++ )
    {
        mCore.SB[i - GAURD_AREA] = checkVal;
        mCore.RB[i - GAURD_AREA] = checkVal;
        mCore.ST[i] = checkVal;
        mCore.RT[i] = checkVal;
        checkVal += 0x04040404;
    }
#endif
    mCore.optypeAction = NULL;
    mCore.numBuiltinOps = 0;
    mCore.ops = NULL;
    mCore.numOps = 0;
    mCore.maxOps = 0;
    mCore.IP = NULL;
    mCore.pEngine = pEngine;

    mCore.pDictionary = NULL;

	SPEW_SHELL("ForthThread pCore=%p NULLing consoleOutStream\n", &mCore);
	mCore.consoleOutStream.pData = NULL;
	mCore.consoleOutStream.pMethodOps = NULL;

    pEngine->ResetConsoleOut( &mCore );

    mOps[1] = gCompiledOps[OP_DONE];

    Reset();
}

ForthThread::~ForthThread()
{
    mCore.SB -= GAURD_AREA;
    delete [] mCore.SB;
    mCore.RB -= GAURD_AREA;
    delete [] mCore.RB;

	if (mpShowContext != NULL)
	{
		delete mpShowContext;
	}
#ifdef WIN32
    if ( mHandle != 0 )
    {
        CloseHandle( mHandle );
    }
#else
    if ( mHandle != 0 )
    {
    	// TODO
        //CloseHandle( mHandle );
    }
#endif
}

#ifdef CHECK_GAURD_AREAS
bool
ForthThread::CheckGaurdAreas( void )
{
    long checkVal = 0x03020100;
    bool retVal = false;
    for ( int i = 0; i < 64; i++ )
    {
        if ( mCore.SB[i - GAURD_AREA] != checkVal )
        {
            return true;
        }
        if ( mCore.RB[i - GAURD_AREA] != checkVal )
        {
            return true;
        }
        if ( mCore.ST[i] != checkVal )
        {
            return true;
        }
        if ( mCore.RT[i] != checkVal )
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
    mCore.SP = mCore.ST;
    mCore.RP = mCore.RT;
    mCore.FP = NULL;
    mCore.TPM = NULL;
    mCore.TPD = NULL;

    mCore.error = kForthErrorNone;
    mCore.state = kResultDone;
    mCore.varMode = kVarDefaultOp;
    mCore.base = 10;
    mCore.signedPrintMode = kPrintSignedDecimal;
	mCore.IP = &(mOps[0]);

	if (mpShowContext != NULL)
	{
		mpShowContext->Reset();
	}

}

void
ForthThread::ResetIP( void )
{
	mCore.IP = &(mOps[0]);
}

#ifdef WIN32

unsigned __stdcall ForthThread::RunLoop( void *pUserData )
{
    ForthThread* pThis = (ForthThread*) pUserData;
    eForthResult exitStatus = kResultOk;

    pThis->Reset();
    pThis->mCore.IP = &(pThis->mOps[0]);
#ifdef ASM_INNER_INTERPRETER
    if ( pThis->mpEngine->GetFastMode() )
    {
        exitStatus = InnerInterpreterFast( &(pThis->mCore) );
    }
    else
#endif
    {
        exitStatus = InnerInterpreter( &(pThis->mCore) );
    }

    return 0;
}

#else

void* ForthThread::RunLoop( void *pUserData )
{
    ForthThread* pThis = (ForthThread*) pUserData;
    eForthResult exitStatus = kResultOk;

    pThis->Reset();
    pThis->mCore.IP = &(pThis->mOps[0]);
#ifdef ASM_INNER_INTERPRETER
    if ( pThis->mpEngine->GetFastMode() )
    {
        exitStatus = InnerInterpreterFast( &(pThis->mCore) );
    }
    else
#endif
    {
        exitStatus = InnerInterpreter( &(pThis->mCore) );
    }

    return NULL;
}

#endif

void ForthThread::Run()
{
    eForthResult exitStatus;

    mCore.IP = &(mOps[0]);
	// the user defined ops could have changed since this thread was created, update it to match the engine
	ForthCoreState* pEngineState = mpEngine->GetCoreState();
	mCore.ops = pEngineState->ops;
	mCore.numOps = pEngineState->numOps;
#ifdef ASM_INNER_INTERPRETER
    if ( mpEngine->GetFastMode() )
    {
        exitStatus = InnerInterpreterFast( &mCore );
    }
    else
#endif
    {
        exitStatus = InnerInterpreter( &mCore );
    }
}

long ForthThread::Start()
{
#ifdef WIN32
    // securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
    if ( mHandle != 0 )
    {
        ::CloseHandle( mHandle );
    }
    mHandle = (HANDLE) _beginthreadex( NULL, 0, ForthThread::RunLoop, this, 0, (unsigned *) &mThreadId );
#else
    // securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
    if ( mHandle != 0 )
    {
    	// TODO
        //::CloseHandle( mHandle );
    }
    mHandle = pthread_create( &mThread, NULL, ForthThread::RunLoop, this );

#endif
    return (long) mHandle;
}

void ForthThread::Exit()
{
    // TBD: make sure this isn't the main thread
    if ( mpNext != NULL )
    {
#ifdef WIN32
        _endthreadex( 0 );
#else
        pthread_exit( &mExitStatus );
#endif
    }
}

ForthShowContext* ForthThread::GetShowContext()
{
	if (mpShowContext == NULL)
	{
		mpShowContext = new ForthShowContext;
	}
	return mpShowContext;
}
//////////////////////////////////////////////////////////////////////
////
///
//                     ForthThreadQueue
// 

ForthThreadQueue::ForthThreadQueue( int initialSize )
:   mFirst( 0 )
,   mCount( 0 )
,   mSize( initialSize )
{
    mQueue = (ForthThread **) __MALLOC( sizeof(ForthThread*) *  mSize );
}

ForthThreadQueue::~ForthThreadQueue()
{
	__FREE(mQueue);
}

void ForthThreadQueue::AddThread( ForthThread* pThread )
{
    if ( mCount == mSize )
    {
        mSize += 16;
		mQueue = (ForthThread **)__REALLOC(mQueue, sizeof(ForthThread*) *  mSize);
    }
    int ix = mFirst + mCount;
    if ( ix >= mSize )
    {
        ix -= mSize;
    }
    mQueue[ix] = pThread;
    ++mCount;
}

int ForthThreadQueue::Count()
{
    return mCount;
}

// returns NULL if queue is empty
ForthThread* ForthThreadQueue::RemoveThread()
{
    ForthThread* result = NULL;
    if ( mCount != 0 )
    {
        result = mQueue[mFirst++];
        if ( mFirst == mSize )
        {
            mFirst = 0;
        }
        --mCount;
    }
    return result;
}

// returns NULL if index is out of range
ForthThread* ForthThreadQueue::RemoveThread( int index )
{
    ForthThread* result = NULL;
    if ( index < mCount )
    {
        int ix = mFirst + index;
        if ( ix > mSize )
        {
            ix -= mSize;
        }
        result = mQueue[ix];
        if ( index != 0 )
        {
            // move first element into slot where removed thread was
            mQueue[ix] = mQueue[mFirst];
        }
        mFirst++;
        if ( mFirst == mSize )
        {
            mFirst = 0;
        }
        --mCount;
    }
    return result;
}

// returns NULL if index is out of range
ForthThread* ForthThreadQueue::GetThread( int index )
{
    ForthThread* result = NULL;
    if ( index < mCount )
    {
        int ix = mFirst + index;
        if ( ix > mSize )
        {
            ix -= mSize;
        }
        result = mQueue[ix];
    }
    return result;
}

// returns -1 if queue is empty
int ForthThreadQueue::FindEarliest()
{
    int result = -1;
    unsigned long earliest = 0;
    for ( int i = 0; i < mCount; i++ )
    {
        int ix = mFirst + i;
        if ( ix > mSize )
        {
            ix -= mSize;
        }
        unsigned long wakeupTime = mQueue[ix]->WakeupTime();
        if ( wakeupTime <= earliest )
        {
            result = i;
            earliest = wakeupTime;
        }
    }
    return result;
}

namespace OThread
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 oThread
	//

	struct oThreadStruct
	{
		ulong			refCount;
		ForthThread		*pThread;
	};

	// TODO: add tracking of run state of thread - this should be done inside ForthThread, not here
	FORTHOP(oThreadNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oThreadStruct, pThreadStruct);
		pThreadStruct->refCount = 0;
		pThreadStruct->pThread = NULL;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pThreadStruct);
	}

	FORTHOP(oThreadDeleteMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		if (pThreadStruct->pThread != NULL)
		{
			GET_ENGINE->DestroyThread(pThreadStruct->pThread);
			pThreadStruct->pThread = NULL;
		}
		FREE_OBJECT(pThreadStruct);
		METHOD_RETURN;
	}

	FORTHOP(oThreadInitMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		ForthEngine* pEngine = GET_ENGINE;
		if (pThreadStruct->pThread != NULL)
		{
			pEngine->DestroyThread(pThreadStruct->pThread);
		}
		long threadOp = SPOP;
		int returnStackSize = (int)(SPOP);
		int paramStackSize = (int)(SPOP);
		pThreadStruct->pThread = GET_ENGINE->CreateThread(threadOp, paramStackSize, returnStackSize);
		pThreadStruct->pThread->Reset();
		METHOD_RETURN;
	}

	FORTHOP(oThreadStartMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		long result = pThreadStruct->pThread->Start();
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oThreadExitMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Exit();
		METHOD_RETURN;
	}

	FORTHOP(oThreadPushMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Push(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oThreadPopMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		long val = pThreadStruct->pThread->Pop();
		SPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oThreadRPushMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->RPush(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oThreadRPopMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		long val = pThreadStruct->pThread->RPop();
		SPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oThreadGetStateMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		SPUSH((long)(pThreadStruct->pThread->GetCore()));
		METHOD_RETURN;
	}

	FORTHOP(oThreadStepMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		ForthThread* pThread = pThreadStruct->pThread;
		ForthCoreState* pThreadCore = pThread->GetCore();
		long op = *(pThreadCore->IP)++;
		long result;
		ForthEngine *pEngine = GET_ENGINE;
#ifdef ASM_INNER_INTERPRETER
		if (pEngine->GetFastMode())
		{
			result = (long)InterpretOneOpFast(pThreadCore, op);
		}
		else
#endif
		{
			result = (long)InterpretOneOp(pThreadCore, op);
		}
		if (result == kResultDone)
		{
			pThread->ResetIP();
		}
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oThreadResetMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Reset();
		METHOD_RETURN;
	}

	FORTHOP(oThreadResetIPMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->ResetIP();
		METHOD_RETURN;
	}

	baseMethodEntry oThreadMembers[] =
	{
		METHOD("__newOp", oThreadNew),
		METHOD("delete", oThreadDeleteMethod),
		METHOD("init", oThreadInitMethod),
		METHOD_RET("start", oThreadStartMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("exit", oThreadExitMethod),
		METHOD("push", oThreadPushMethod),
		METHOD_RET("pop", oThreadPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("rpush", oThreadRPushMethod),
		METHOD_RET("rpop", oThreadRPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("getState", oThreadGetStateMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("step", oThreadStepMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("reset", oThreadResetMethod),
		METHOD("resetIP", oThreadResetIPMethod),

		MEMBER_VAR("__thread", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		ForthClassVocabulary* pOThreadClass = pEngine->AddBuiltinClass("OThread", gpObjectClassVocab, oThreadMembers);
	}

} // namespace OThread
