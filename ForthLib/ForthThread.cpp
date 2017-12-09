// ForthThread.cpp: implementation of the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#ifdef WIN32
#include <process.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif
#include <deque>
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

struct oAsyncThreadStruct
{
	ulong				refCount;
	int					id;
	ForthAsyncThread	*pThread;
};

struct oThreadStruct
{
	ulong				refCount;
	int					id;
	ForthThread			*pThread;
};

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthThread
// 

ForthThread::ForthThread(ForthEngine *pEngine, ForthAsyncThread *pParentThread, int paramStackLongs, int returnStackLongs)
: mpEngine( pEngine )
, mpParentThread(pParentThread)
, mWakeupTime(0)
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
	oThreadStruct* pThreadStruct = (oThreadStruct *)mObject.pData;
	if (pThreadStruct->pThread != NULL)
	{
		FREE_OBJECT(pThreadStruct);
	}
}

void ForthThread::Destroy()
{
    if (mpParentThread != nullptr)
    {
        mpParentThread->DeleteThread(this);
    }
    else
    {
        delete this;
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

void ForthThread::InitTables(ForthThread* pSourceThread)
{
	ForthCoreState& sourceCore = pSourceThread->mCore;
	mCore.optypeAction = sourceCore.optypeAction;
	mCore.numBuiltinOps = sourceCore.numBuiltinOps;
	mCore.numOps = sourceCore.numOps;
	mCore.maxOps = sourceCore.maxOps;
	mCore.ops = sourceCore.ops;
	mCore.innerLoop = sourceCore.innerLoop;
    mCore.innerExecute = sourceCore.innerExecute;
}

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
    mCore.traceFlags = 0;
	//mCore.IP = nullptr;

	if (mpShowContext != NULL)
	{
		mpShowContext->Reset();
	}

}

void
ForthThread::ResetIP( void )
{
	mCore.IP = &(mOps[0]);
	//mCore.IP = nullptr;
}

void ForthThread::Run()
{
    eForthResult exitStatus;

	Wake();
	mCore.IP = &(mOps[0]);
	//mCore.IP = nullptr;
	// the user defined ops could have changed since this thread was created, update it to match the engine
	ForthCoreState* pEngineState = mpEngine->GetCoreState();
	mCore.ops = pEngineState->ops;
	mCore.numOps = pEngineState->numOps;
#ifdef ASM_INNER_INTERPRETER
    if ( mpEngine->GetFastMode() )
    {
		do
		{
			exitStatus = InnerInterpreterFast(&mCore);
		} while (exitStatus == kResultYield);
    }
    else
#endif
    {
		do
		{
			exitStatus = InnerInterpreter(&mCore);
		} while (exitStatus == kResultYield);
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

void ForthThread::SetRunState(eForthThreadRunState newState)
{
	// TODO!
	mRunState = newState;
}

void ForthThread::Sleep(ulong sleepMilliSeconds)
{
	ulong now = mpEngine->GetElapsedTime();
	mWakeupTime = now + sleepMilliSeconds;
	mRunState = kFTRSSleeping;
}

void ForthThread::Block()
{
	mRunState = kFTRSBlocked;
}

void ForthThread::Wake()
{
	mRunState = kFTRSReady;
	mWakeupTime = 0;
}

void ForthThread::Stop()
{
	mRunState = kFTRSStopped;
}

void ForthThread::Exit()
{
	mRunState = kFTRSExited;
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthAsyncThread
// 

ForthAsyncThread::ForthAsyncThread(ForthEngine *pEngine, int paramStackLongs, int returnStackLongs)
	: mHandle(0)
#ifdef WIN32
	, mThreadId(0)
#endif
	, mpNext(NULL)
	, mActiveThreadIndex(0)
	, mRunState(kFTRSStopped)
{
	ForthThread* pPrimaryThread = new ForthThread(pEngine, this, paramStackLongs, returnStackLongs);
	pPrimaryThread->SetRunState(kFTRSReady);
	mSoftThreads.push_back(pPrimaryThread);
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
	for (ForthThread* pThread : mSoftThreads)
	{
		if (pThread != nullptr)
		{
			delete pThread;
		}
	}
	oAsyncThreadStruct* pThreadStruct = (oAsyncThreadStruct *)mObject.pData;
	if (pThreadStruct->pThread != NULL)
	{
		FREE_OBJECT(pThreadStruct);
	}
}

#ifdef WIN32
unsigned __stdcall ForthAsyncThread::RunLoop(void *pUserData)
#else
void* ForthAsyncThread::RunLoop(void *pUserData)
#endif
{
	ForthAsyncThread* pParentThread = (ForthAsyncThread*)pUserData;
	ForthThread* pActiveThread = pParentThread->GetActiveThread();
	ForthEngine* pEngine = pActiveThread->GetEngine();

	pParentThread->mRunState = kFTRSReady;
	eForthResult exitStatus = kResultOk;
	bool keepRunning = true;
	while (keepRunning)
	{
		bool checkForAllDone = false;
		ForthCoreState* pCore = pActiveThread->GetCore();
#ifdef ASM_INNER_INTERPRETER
		if (pEngine->GetFastMode())
		{
			exitStatus = InnerInterpreterFast(pCore);
		}
		else
#endif
		{
			exitStatus = InnerInterpreter(pCore);
		}

		bool switchActiveThread = false;
		switch (exitStatus)
		{
		case kResultYield:
			switchActiveThread = true;
			SET_STATE(kResultOk);
			break;

		case kResultDone:
			switchActiveThread = true;
			checkForAllDone = true;
			break;

		default:
			break;
		}

		if (switchActiveThread)
		{
			// TODO!
			// - switch to next runnable thread
			// - sleep if all threads are sleeping
			// - deal with all threads stopped
			ulong now = pEngine->GetElapsedTime();

			ForthThread* pNextThread = pParentThread->GetNextReadyThread();
			if (pNextThread != nullptr)
			{
				pActiveThread = pNextThread;
			}
			else
			{
				pNextThread = pParentThread->GetNextSleepingThread();
				if (pNextThread != nullptr)
				{
					ulong wakeupTime = pNextThread->GetWakeupTime();
					if (now >= wakeupTime)
					{
						pNextThread->Wake();
						pActiveThread = pNextThread;
					}
					else
					{
						// TODO: don't always sleep until wakeupTime, since threads which are blocked or stopped
						//  could be unblocked or started by other async threads
						int sleepMilliseconds = wakeupTime - now;
#ifdef WIN32
						::Sleep((DWORD)sleepMilliseconds);
#else
						usleep(sleepMilliseconds * 1000);
#endif
						pActiveThread = pNextThread;
					}
				}
				else
				{
					// there are no ready or sleeping soft threads, should we exit this asyncThread?
					checkForAllDone = true;
				}
			}
		}
		if (checkForAllDone)
		{
			keepRunning = false;
			for (ForthThread* pThread : pParentThread->mSoftThreads)
			{
				if (pThread->GetRunState() != kFTRSExited)
				{
					keepRunning = true;
					break;
				}
			}
		}
	}

	pParentThread->mRunState = kFTRSExited;
#ifdef WIN32
	return 0;
#else
	return nullptr;
#endif
}

ForthThread* ForthAsyncThread::GetThread(int threadIndex)
{
	if (threadIndex < (int)mSoftThreads.size())
	{
		return mSoftThreads[threadIndex];
	}
	return nullptr;
}

ForthThread* ForthAsyncThread::GetActiveThread()
{
	if (mActiveThreadIndex < (int)mSoftThreads.size())
	{
		return mSoftThreads[mActiveThreadIndex];
	}
	return nullptr;
}

void
ForthAsyncThread::Reset(void)
{ 
	for (ForthThread* pThread : mSoftThreads)
	{
		if (pThread != nullptr)
		{
			pThread->Reset();
		}
	}
}

ForthThread* ForthAsyncThread::GetNextReadyThread()
{
	int originalThreadIndex = mActiveThreadIndex;

	do	
	{
		mActiveThreadIndex++;
		if (mActiveThreadIndex >= (int)mSoftThreads.size())
		{
			mActiveThreadIndex = 0;
		}
		ForthThread* pNextThread = mSoftThreads[mActiveThreadIndex];
		if (pNextThread->GetRunState() == kFTRSReady)
		{
			return pNextThread;
		}
	} while (originalThreadIndex != mActiveThreadIndex);

	return nullptr;
}

ForthThread* ForthAsyncThread::GetNextSleepingThread()
{
	ForthThread* pThreadToWake = nullptr;
	int originalThreadIndex = mActiveThreadIndex;
	ulong minWakeupTime = (ulong)(~0);
	do
	{
		if (mActiveThreadIndex >= (int)mSoftThreads.size())
		{
			mActiveThreadIndex = 0;
		}
		ForthThread* pNextThread = mSoftThreads[mActiveThreadIndex];
		if (pNextThread->GetRunState() == kFTRSSleeping)
		{
			ulong wakeupTime = pNextThread->GetWakeupTime();
			if (wakeupTime < minWakeupTime)
			{
				minWakeupTime = wakeupTime;
				pThreadToWake = pNextThread;
			}
		}
		mActiveThreadIndex++;
	} while (originalThreadIndex != mActiveThreadIndex);
	return pThreadToWake;
}

long ForthAsyncThread::Start()
{
#ifdef WIN32
	// securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
	if (mHandle != 0)
	{
		::CloseHandle(mHandle);
	}
	mHandle = (HANDLE)_beginthreadex(NULL, 0, ForthAsyncThread::RunLoop, this, 0, (unsigned *)&mThreadId);
#else
	// securityAttribPtr, stackSize, threadCodeAddr, threadUserData, flags, pThreadIdReturn
	if (mHandle != 0)
	{
		// TODO
		//::CloseHandle( mHandle );
	}
	mHandle = pthread_create(&mThread, NULL, ForthAsyncThread::RunLoop, this);

#endif
	return (long)mHandle;
}

void ForthAsyncThread::Exit()
{
	// TBD: make sure this isn't the main thread
	if (mpNext != NULL)
	{
		mRunState = kFTRSExited;
#ifdef WIN32
		_endthreadex(0);
#else
		pthread_exit(&mExitStatus);
#endif
	}
}

ForthThread* ForthAsyncThread::CreateThread(ForthEngine *pEngine, long threadOp, int paramStackLongs, int returnStackLongs)
{
	ForthThread* pThread = new ForthThread(pEngine, this, paramStackLongs, returnStackLongs);
	pThread->SetOp(threadOp);
	pThread->SetRunState(kFTRSStopped);
	ForthThread* pPrimaryThread = GetThread(0);
	pThread->InitTables(pPrimaryThread);
	mSoftThreads.push_back(pThread);

	return pThread;
}

void ForthAsyncThread::DeleteThread(ForthThread* pInThread)
{
	// TODO: do something when erasing last thread?  what about thread 0?
	int lastIndex = mSoftThreads.size() - 1;
	for (int i = 0; i <= lastIndex; ++i)
	{
		ForthThread* pThread = mSoftThreads[i];
		if (pThread == pInThread)
		{
			delete pThread;
			mSoftThreads.erase(mSoftThreads.begin() + i);
            break;
        }
	}
	if (mActiveThreadIndex == lastIndex)
	{
		mActiveThreadIndex = 0;
	}
}

namespace OThread
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 oAsyncThread
	//

	static ForthClassVocabulary* gpAsyncThreadVocabulary;
	static ForthClassVocabulary* gpThreadVocabulary;

	void CreateAsyncThreadObject(ForthObject& outAsyncThread, ForthEngine *pEngine, long threadOp, int paramStackLongs, int returnStackLongs)
	{
		ForthInterface* pPrimaryInterface = gpAsyncThreadVocabulary->GetInterface(0);

		MALLOCATE_OBJECT(oAsyncThreadStruct, pThreadStruct, gpAsyncThreadVocabulary);
		pThreadStruct->refCount = 0;
		ForthAsyncThread* pAsyncThread = pEngine->CreateAsyncThread(threadOp, paramStackLongs, returnStackLongs);
		pThreadStruct->pThread = pAsyncThread;
		pAsyncThread->Reset();
		outAsyncThread.pMethodOps = pPrimaryInterface->GetMethods();
		outAsyncThread.pData = (long *) pThreadStruct;
		pAsyncThread->SetAsyncThreadObject(outAsyncThread);
		FixupThread(pAsyncThread->GetThread(0));
	}

	FORTHOP(oAsyncThreadNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create an AsyncThread object");
	}

	FORTHOP(oAsyncThreadDeleteMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		if (pThreadStruct->pThread != NULL)
		{
			GET_ENGINE->DestroyAsyncThread(pThreadStruct->pThread);
		}
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadStartMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		ForthAsyncThread* pAsyncThread = pThreadStruct->pThread;
		ForthThread* pThread = pAsyncThread->GetThread(0);
		pThread->Reset();
		long result = pAsyncThread->Start();
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadStartWithArgsMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		ForthAsyncThread* pAsyncThread = pThreadStruct->pThread;
		ForthThread* pThread = pAsyncThread->GetThread(0);
		pThread->Reset();
		ForthCoreState* pDstCore = pThread->GetCore();
		int numArgs = SPOP;
		if (numArgs > 0)
		{
			pDstCore->SP -= numArgs;
			memcpy(pDstCore->SP, pCore->SP, numArgs << 2);
			pCore->SP += numArgs;
		}
		long result = pAsyncThread->Start();
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadExitMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Exit();
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadResetMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		pThreadStruct->pThread->Reset();
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadCreateThreadMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		ForthEngine* pEngine = GET_ENGINE;
		ForthObject thread;

		long threadOp = SPOP;
		int returnStackLongs = (int)(SPOP);
		int paramStackLongs = (int)(SPOP);
		CreateThreadObject(thread, pThreadStruct->pThread, pEngine, threadOp, paramStackLongs, returnStackLongs);

		PUSH_OBJECT(thread);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncThreadGetRunStateMethod)
	{
		GET_THIS(oAsyncThreadStruct, pThreadStruct);
		SPUSH((long)(pThreadStruct->pThread->GetRunState()));
		METHOD_RETURN;
	}

	baseMethodEntry oAsyncThreadMembers[] =
	{
		METHOD("__newOp", oAsyncThreadNew),
		METHOD("delete", oAsyncThreadDeleteMethod),
		METHOD_RET("start", oAsyncThreadStartMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("startWithArgs", oAsyncThreadStartWithArgsMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("exit", oAsyncThreadExitMethod),
		METHOD("reset", oAsyncThreadResetMethod),
		METHOD_RET("createThread", oAsyncThreadCreateThreadMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod, kBCIThread)),
		METHOD_RET("getRunState", oAsyncThreadGetRunStateMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("__thread", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};
	

	//////////////////////////////////////////////////////////////////////
	///
	//                 oThread
	//

	void CreateThreadObject(ForthObject& outThread, ForthAsyncThread *pParentAsyncThread, ForthEngine *pEngine, long threadOp, int paramStackLongs, int returnStackLongs)
	{
		ForthInterface* pPrimaryInterface = gpThreadVocabulary->GetInterface(0);

		MALLOCATE_OBJECT(oThreadStruct, pThreadStruct, gpThreadVocabulary);
		pThreadStruct->refCount = 0;
		pThreadStruct->pThread = pParentAsyncThread->CreateThread(pEngine, threadOp, paramStackLongs, returnStackLongs);
		pThreadStruct->pThread->Reset();

		outThread.pMethodOps = pPrimaryInterface->GetMethods();
		outThread.pData = (long *)pThreadStruct;
		pThreadStruct->pThread->SetThreadObject(outThread);
	}

	void FixupThread(ForthThread* pThread)
	{
		ForthInterface* pPrimaryInterface = gpThreadVocabulary->GetInterface(0);

		MALLOCATE_OBJECT(oThreadStruct, pThreadStruct, gpThreadVocabulary);
		ForthObject& primaryThread = pThread->GetThreadObject();
		pThreadStruct->refCount = 0;
		pThreadStruct->pThread = pThread;
		primaryThread.pMethodOps = pPrimaryInterface->GetMethods();
		primaryThread.pData = (long *)pThreadStruct;
	}

	void FixupAsyncThread(ForthAsyncThread* pAsyncThread)
	{
		ForthInterface* pPrimaryInterface = gpAsyncThreadVocabulary->GetInterface(0);

		MALLOCATE_OBJECT(oAsyncThreadStruct, pAsyncThreadStruct, gpAsyncThreadVocabulary);
		pAsyncThreadStruct->refCount = 0;
		pAsyncThreadStruct->pThread = pAsyncThread;
		ForthObject& primaryAsyncThread = pAsyncThread->GetAsyncThreadObject();
		primaryAsyncThread.pMethodOps = pPrimaryInterface->GetMethods();
		primaryAsyncThread.pData = (long *)pAsyncThreadStruct;

		FixupThread(pAsyncThread->GetThread(0));
	}

	FORTHOP(oThreadNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a Thread object");
	}

	FORTHOP(oThreadDeleteMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		if (pThreadStruct->pThread != NULL)
		{
            pThreadStruct->pThread->Destroy();
        }
		METHOD_RETURN;
	}

	FORTHOP(oThreadStartMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->SetRunState(kFTRSReady);
		METHOD_RETURN;
	}

	FORTHOP(oThreadStartWithArgsMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		ForthThread* pThread = pThreadStruct->pThread;
		ForthCoreState* pDstCore = pThread->GetCore();
		int numArgs = SPOP;
		if (numArgs > 0)
		{
			pDstCore->SP -= numArgs;
			memcpy(pDstCore->SP, pCore->SP, numArgs << 2);
			pCore->SP += numArgs;
		}
		pThread->SetRunState(kFTRSReady);
		METHOD_RETURN;
	}

	FORTHOP(oThreadStopMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		pThreadStruct->pThread->SetRunState(kFTRSStopped);
		METHOD_RETURN;
	}

	FORTHOP(oThreadSleepMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		ulong sleepMilliseconds = SPOP;
		pThreadStruct->pThread->Sleep(sleepMilliseconds);
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

	FORTHOP(oThreadGetRunStateMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		SPUSH((long)(pThreadStruct->pThread->GetRunState()));
		METHOD_RETURN;
	}

	FORTHOP(oThreadStepMethod)
	{
		GET_THIS(oThreadStruct, pThreadStruct);
		ForthThread* pThread = pThreadStruct->pThread;
		ForthCoreState* pThreadCore = pThread->GetCore();
		long op = *(pThreadCore->IP)++;
		long result;
#ifdef ASM_INNER_INTERPRETER
        ForthEngine *pEngine = GET_ENGINE;
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

    FORTHOP(oThreadGetCoreMethod)
    {
        GET_THIS(oThreadStruct, pThreadStruct);
        SPUSH((long)(pThreadStruct->pThread->GetCore()));
        METHOD_RETURN;
    }

    baseMethodEntry oThreadMembers[] =
	{
		METHOD("__newOp", oThreadNew),
		METHOD("delete", oThreadDeleteMethod),
		METHOD("start", oThreadStartMethod),
		METHOD("startWithArgs", oThreadStartWithArgsMethod),
		METHOD("stop", oThreadStopMethod),
		METHOD("sleep", oThreadSleepMethod),
		METHOD("push", oThreadPushMethod),
		METHOD_RET("pop", oThreadPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("rpush", oThreadRPushMethod),
		METHOD_RET("rpop", oThreadRPopMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("getRunState", oThreadGetRunStateMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD_RET("step", oThreadStepMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("reset", oThreadResetMethod),
		METHOD("resetIP", oThreadResetIPMethod),
        METHOD("getCore", oThreadGetCoreMethod),
		//METHOD_RET("getParent", oThreadGetParentMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseType)),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("__thread", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		gpThreadVocabulary = pEngine->AddBuiltinClass("Thread", kBCIThread, kBCIObject, oThreadMembers);
		gpAsyncThreadVocabulary = pEngine->AddBuiltinClass("AsyncThread", kBCIAsyncThread, kBCIObject, oAsyncThreadMembers);
	}

} // namespace OThread

namespace OLock
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 OAsyncLock
	//

	struct oAsyncLockStruct
	{
		ulong			refCount;
		int				id;
#ifdef WIN32
		CRITICAL_SECTION* pLock;
#else
        pthread_mutex_t* pLock;
#endif
	};

	static ForthClassVocabulary* gpAsyncLockVocabulary;

	void CreateAsyncLockObject(ForthObject& outAsyncLock, ForthEngine *pEngine)
	{
		ForthInterface* pPrimaryInterface = gpAsyncLockVocabulary->GetInterface(0);

		MALLOCATE_OBJECT(oAsyncLockStruct, pLockStruct, gpAsyncLockVocabulary);
		pLockStruct->refCount = 0;
#ifdef WIN32
        pLockStruct->pLock = new CRITICAL_SECTION();
        InitializeCriticalSection(pLockStruct->pLock);
#else
		pLockStruct->pLock = new pthread_mutex_t;
		pthread_mutexattr_t mutexAttr;
		pthread_mutexattr_init(&mutexAttr);
		pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);

		pthread_mutex_init(pLockStruct->pLock, &mutexAttr);

		pthread_mutexattr_destroy(&mutexAttr);
#endif

		outAsyncLock.pMethodOps = pPrimaryInterface->GetMethods();
		outAsyncLock.pData = (long *)pLockStruct;
	}

	FORTHOP(oAsyncLockNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create an AsyncLock object");
	}

	FORTHOP(oAsyncLockDeleteMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
		if (pLockStruct->pLock != NULL)
		{
#ifdef WIN32
			DeleteCriticalSection(pLockStruct->pLock);
#else
			pthread_mutex_destroy(pLockStruct->pLock);
			delete pLockStruct->pLock;
#endif
			pLockStruct->pLock = NULL;
		}
		FREE_OBJECT(pLockStruct);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncLockGrabMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
#ifdef WIN32
        EnterCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_lock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	FORTHOP(oAsyncLockTryGrabMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
#ifdef WIN32
        BOOL result = TryEnterCriticalSection(pLockStruct->pLock);
#else
		int lockResult = pthread_mutex_trylock(pLockStruct->pLock);
		bool result = (lockResult == 0);
#endif
		SPUSH((int)result);
		METHOD_RETURN;
	}

	FORTHOP(oAsyncLockUngrabMethod)
	{
		GET_THIS(oAsyncLockStruct, pLockStruct);
#ifdef WIN32
        LeaveCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_unlock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	baseMethodEntry oAsyncLockMembers[] =
	{
		METHOD("__newOp", oAsyncLockNew),
		METHOD("delete", oAsyncLockDeleteMethod),
		METHOD("grab", oAsyncLockGrabMethod),
		METHOD_RET("tryGrab", oAsyncLockTryGrabMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("ungrab", oAsyncLockUngrabMethod),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("__lock", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 OLock
	//

	struct oLockStruct
	{
		ulong			refCount;
		int				id;
		int				lockDepth;
#ifdef WIN32
		CRITICAL_SECTION* pLock;
#else
		pthread_mutex_t* pLock;
#endif
		ForthThread* pLockHolder;
		std::deque<ForthThread*> *pBlockedThreads;
	};

	FORTHOP(oLockNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		ForthInterface* pPrimaryInterface = pClassVocab->GetInterface(0);
		MALLOCATE_OBJECT(oLockStruct, pLockStruct, pClassVocab);

		pLockStruct->refCount = 0;
#ifdef WIN32
        pLockStruct->pLock = new CRITICAL_SECTION();
        InitializeCriticalSection(pLockStruct->pLock);
#else
		pLockStruct->pLock = new pthread_mutex_t;
		pthread_mutexattr_t mutexAttr;
		pthread_mutexattr_init(&mutexAttr);
		pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);

		pthread_mutex_init(pLockStruct->pLock, &mutexAttr);

		pthread_mutexattr_destroy(&mutexAttr);
#endif
		pLockStruct->pLockHolder = nullptr;
		pLockStruct->pBlockedThreads = new std::deque<ForthThread*>;
		pLockStruct->lockDepth = 0;

		PUSH_PAIR(pPrimaryInterface->GetMethods(), pLockStruct);
	}

	FORTHOP(oLockDeleteMethod)
	{
		GET_THIS(oLockStruct, pLockStruct);
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.delete called with threads blocked on lock");

#ifdef WIN32
        DeleteCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_destroy(pLockStruct->pLock);
#endif
        delete pLockStruct->pLock;
        delete pLockStruct->pBlockedThreads;
		FREE_OBJECT(pLockStruct);
		METHOD_RETURN;
	}

	FORTHOP(oLockGrabMethod)
	{
		GET_THIS(oLockStruct, pLockStruct);
#ifdef WIN32
        EnterCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_lock(pLockStruct->pLock);
#endif

		ForthThread* pThread = (ForthThread*)(pCore->pThread);
		if (pLockStruct->pLockHolder == nullptr)
		{
			if (pLockStruct->lockDepth != 0)
			{
				GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.grab called with no lock holder and lock depth not 0");
			}
			else
			{
				pLockStruct->pLockHolder = pThread;
				pLockStruct->lockDepth++;
			}
		}
		else
		{
			if (pLockStruct->pLockHolder == pThread)
			{
				pLockStruct->lockDepth++;
			}
			else
			{
				pThread->Block();
				SET_STATE(kResultYield);
				pLockStruct->pBlockedThreads->push_back(pThread);
			}
		}

#ifdef WIN32
        LeaveCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_unlock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	FORTHOP(oLockTryGrabMethod)
	{
		GET_THIS(oLockStruct, pLockStruct);
#ifdef WIN32
        EnterCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_lock(pLockStruct->pLock);
#endif

		int result = (int)false;
		ForthThread* pThread = (ForthThread*)(pCore->pThread);
		if (pLockStruct->pLockHolder == nullptr)
		{
			if (pLockStruct->lockDepth != 0)
			{
				GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.tryGrab called with no lock holder and lock depth not 0");
			}
			else
			{
				pLockStruct->pLockHolder = pThread;
				pLockStruct->lockDepth++;
				result = true;
			}
		}
		else
		{
			if (pLockStruct->pLockHolder == pThread)
			{
				pLockStruct->lockDepth++;
				result = true;
			}
		}
		SPUSH(result);

#ifdef WIN32
        LeaveCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_unlock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	FORTHOP(oLockUngrabMethod)
	{
		GET_THIS(oLockStruct, pLockStruct);
#ifdef WIN32
        EnterCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_lock(pLockStruct->pLock);
#endif

		if (pLockStruct->pLockHolder == nullptr)
		{
			GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.ungrab called on ungrabbed lock");
		}
		else
		{
			if (pLockStruct->pLockHolder != (ForthThread*)(pCore->pThread))
			{
				GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.ungrab called by thread which does not have lock");
			}
			else
			{
				if (pLockStruct->lockDepth <= 0)
				{
					GET_ENGINE->SetError(kForthErrorIllegalOperation, " OLock.ungrab called with lock depth <= 0");
				}
				else
				{
					pLockStruct->lockDepth--;
					if (pLockStruct->lockDepth == 0)
					{
						if (pLockStruct->pBlockedThreads->size() == 0)
						{
							pLockStruct->pLockHolder = nullptr;
						}
						else
						{
							ForthThread* pThread = pLockStruct->pBlockedThreads->front();
							pLockStruct->pBlockedThreads->pop_front();
							pLockStruct->pLockHolder = pThread;
							pLockStruct->lockDepth++;
							pThread->Wake();
							SET_STATE(kResultYield);
						}
					}
				}
			}
		}

#ifdef WIN32
        LeaveCriticalSection(pLockStruct->pLock);
#else
		pthread_mutex_unlock(pLockStruct->pLock);
#endif
		METHOD_RETURN;
	}

	baseMethodEntry oLockMembers[] =
	{
		METHOD("__newOp", oLockNew),
		METHOD("delete", oLockDeleteMethod),
		METHOD("grab", oLockGrabMethod),
		METHOD_RET("tryGrab", oLockTryGrabMethod, NATIVE_TYPE_TO_CODE(kDTIsMethod, kBaseTypeInt)),
		METHOD("ungrab", oLockUngrabMethod),

		MEMBER_VAR("id", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("lockDepth", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("__lock", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("__lockHolder", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("__blockedThreads", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		gpAsyncLockVocabulary = pEngine->AddBuiltinClass("AsyncLock", kBCIAsyncLock, kBCIObject, oAsyncLockMembers);
		pEngine->AddBuiltinClass("Lock", kBCILock, kBCIObject, oLockMembers);
	}

} // namespace OLock

