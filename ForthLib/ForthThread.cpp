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
//                     ForthAsyncThread
// 

ForthAsyncThread::ForthAsyncThread(ForthEngine *pEngine, int paramStackLongs, int returnStackLongs)
	: ForthThread(pEngine, paramStackLongs, returnStackLongs)
	, mHandle(0)
{
}

ForthAsyncThread::~ForthAsyncThread()
{
#ifdef WIN32
	if (mHandle != 0)
	{
		CloseHandle(mHandle);
	}
#else
	if (mHandle != 0)
	{
		// TODO
		//CloseHandle( mHandle );
	}
#endif
}

long ForthAsyncThread::Start()
{
#ifdef WIN32
	// securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
	if (mHandle != 0)
	{
		::CloseHandle(mHandle);
	}
	mHandle = (HANDLE)_beginthreadex(NULL, 0, ForthThread::RunLoop, this, 0, (unsigned *)&mThreadId);
#else
	// securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
	if (mHandle != 0)
	{
		// TODO
		//::CloseHandle( mHandle );
	}
	mHandle = pthread_create(&mThread, NULL, ForthThread::RunLoop, this);

#endif
	return (long)mHandle;
}

void ForthAsyncThread::Exit()
{
	// TBD: make sure this isn't the main thread
	if (mpNext != NULL)
	{
#ifdef WIN32
		_endthreadex(0);
#else
		pthread_exit(&mExitStatus);
#endif
	}
}

namespace OThread
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 oAsyncThread
	//

	struct oAsyncThreadStruct
	{
		ulong				refCount;
		ForthAsyncThread	*pThread;
	};

	// TODO: add tracking of run state of thread - this should be done inside ForthThread, not here
	FORTHOP(oAsyncThreadNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oAsyncThreadStruct, pThreadStruct);
		pThreadStruct->refCount = 0;
		pThreadStruct->pThread = NULL;
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pThreadStruct);
	}

	FORTHOP(oAsyncThreadDeleteMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		if (pThreadStruct->pThread != NULL)
		{
			GET_ENGINE->DestroyThread(pThreadStruct->pThread);
			pThreadStruct->pThread = NULL;
		}
		FREE_OBJECT(pThreadStruct);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadInitMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
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

	FORTHOP(oAsyncThreadStartMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		long result = pThreadStruct->pThread->Start();
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadExitMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Exit();
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadPushMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Push(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadPopMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		long val = pThreadStruct->pThread->Pop();
		SPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadRPushMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		pThreadStruct->pThread->RPush(SPOP);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadRPopMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		long val = pThreadStruct->pThread->RPop();
		SPUSH(val);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadGetStateMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		SPUSH((long)(pThreadStruct->pThread->GetCore()));
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadStepMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
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

	FORTHOP(oAsyncThreadResetMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Reset();
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadResetIPMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		pThreadStruct->pThread->ResetIP();
		METHOD_RETURN;
	}

	baseMethodEntry oAsyncThreadMembers[] =
	{
		METHOD("__newOp", oAsyncThreadNew),
		METHOD("delete", oAsyncThreadDeleteMethod),
		METHOD("init", oAsyncThreadInitMethod),
		METHOD_RET("start", oAsyncThreadStartMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("exit", oAsyncThreadExitMethod),
		METHOD("push", oAsyncThreadPushMethod),
		METHOD_RET("pop", oAsyncThreadPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("rpush", oAsyncThreadRPushMethod),
		METHOD_RET("rpop", oAsyncThreadRPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("getState", oAsyncThreadGetStateMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("step", oAsyncThreadStepMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("reset", oAsyncThreadResetMethod),
		METHOD("resetIP", oAsyncThreadResetIPMethod),

		MEMBER_VAR("__thread", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		ForthClassVocabulary* pOThreadClass = pEngine->AddBuiltinClass("OAsyncThread", kBCIThread, kBCIObject, oAsyncThreadMembers);
	}

} // namespace OThread

namespace OLock
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 OLock
	//

	struct oAsyncLockStruct
	{
		ulong			refCount;
#ifdef WIN32
		CRITICAL_SECTION* pLock;
#else
#endif
	};

	FORTHOP(oAsyncLockNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oAsyncLockStruct, pLockStruct);
		pLockStruct->refCount = 0;
		pLockStruct->pLock = new CRITICAL_SECTION();
		InitializeCriticalSection(pLockStruct->pLock);
		PUSH_PAIR(pPrimaryInterface->GetMethods(), pLockStruct);
	}

	FORTHOP(oAsyncLockDeleteMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
		if (pLockStruct->pLock != NULL)
		{
			DeleteCriticalSection(pLockStruct->pLock);
			pLockStruct->pLock = NULL;
		}
		FREE_OBJECT(pLockStruct);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncLockGrabMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
		EnterCriticalSection(pLockStruct->pLock);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncLockTryGrabMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
		BOOL result = TryEnterCriticalSection(pLockStruct->pLock);
		SPUSH((int)result);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncLockUngrabMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
		LeaveCriticalSection(pLockStruct->pLock);
		METHOD_RETURN;
	}

	baseMethodEntry oAsyncLockMembers[] =
	{
		METHOD("__newOp", oAsyncLockNew),
		METHOD("delete", oAsyncLockDeleteMethod),
		METHOD("grab", oAsyncLockGrabMethod),
		METHOD_RET("tryGrab", oAsyncLockTryGrabMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("ungrab", oAsyncLockUngrabMethod),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		ForthClassVocabulary* pOLock = pEngine->AddBuiltinClass("OAsyncLock", kBCILock, kBCIObject, oAsyncLockMembers);
	}

} // namespace OLock

