#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthThread.h: interface for the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>

#include "Forth.h"
#include "ForthInner.h"
#if defined(LINUX) || defined(MACOSX)
#include <pthread.h>
#include <semaphore.h>
#endif

class ForthEngine;
class ForthShowContext;
class ForthAsyncThread;

#define DEFAULT_PSTACK_SIZE 128
#define DEFAULT_RSTACK_SIZE 128


// define CHECK_GAURD_AREAS if there is a bug where memory immediately above or below the stacks is getting trashed
//#define CHECK_GAURD_AREAS

#ifdef CHECK_GAURD_AREAS
#define CHECK_STACKS(THREAD_PTR)    (THREAD_PTR)->CheckGaurdAreas()
#else
#define CHECK_STACKS(THREAD_PTR)
#endif

class ForthThread  
{
public:
	ForthThread(ForthEngine *pEngine, ForthAsyncThread *pParentThread, int threadIndex, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
    virtual ~ForthThread();
    void Destroy();

#ifdef CHECK_GAURD_AREAS
    bool CheckGaurdAreas();
#endif

	void				InitTables(ForthThread* pSourceThread);

    void                Reset( void );
    void                ResetIP( void );

    inline void         SetOp( long op ) { mOps[0] = op; };

    inline void         Push( long value ) { *--mCore.SP = value; };
    inline long         Pop() { return *mCore.SP++; };
    inline void         RPush( long value ) { *--mCore.RP = value; };
    inline long         RPop() { return *mCore.RP++; };

	void				Run();
	void				Block();

	inline ulong        GetWakeupTime() { return mWakeupTime; };
	void				Sleep(ulong milliSeconds);
	void				Wake();
	void				Stop();
	void				Exit();
    void                Join(ForthThread* pJoiningThread);

	inline void			SetIP( long* newIP ) { mCore.IP = newIP; };
	
	ForthShowContext*	GetShowContext();

	inline eForthThreadRunState GetRunState() { return mRunState; }
	void				SetRunState(eForthThreadRunState newState);
	inline ForthAsyncThread* GetParent() { return mpParentThread; }
	inline ForthEngine* GetEngine() { return mpEngine; }

    friend class ForthEngine;

	inline ForthCoreState* GetCore() { return &mCore; };

	inline ForthObject& GetThreadObject() { return mObject; }
	inline void SetThreadObject(ForthObject& inObject) { mObject = inObject; }

    inline int GetThreadIndex() { return mThreadIndex; }

protected:
    void    WakeAllJoiningThreads();

	ForthObject			mObject;
    ForthEngine         *mpEngine;
    void                *mpPrivate;
	ForthShowContext	*mpShowContext;
	ForthAsyncThread	*mpParentThread;
    //ForthThreadState    mState;
    ForthCoreState      mCore;
    long                mOps[2];
    ulong				mWakeupTime;
	eForthThreadRunState mRunState;
    ForthThread*         mpJoinHead;
    ForthThread*         mpNextJoiner;
    int                 mThreadIndex;
};

class ForthAsyncThread
{
public:
	ForthAsyncThread(ForthEngine *pEngine, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	virtual ~ForthAsyncThread();

	void                Reset(void);
	long                Start();
	void                Exit();
	//void                YieldToNext();
	ForthThread*		GetNextReadyThread();
	ForthThread*		GetNextSleepingThread();
	ForthThread*		GetThread(int threadIndex);
	ForthThread*		GetActiveThread();
    void                SetActiveThread(ForthThread *pThread);

	inline eForthThreadRunState GetRunState() { return mRunState; }

    void                Join();

    void                InnerLoop();

	ForthThread*		CreateThread(ForthEngine *pEngine, long threadOp, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	void				DeleteThread(ForthThread* pThread);

	inline ForthObject& GetAsyncThreadObject() { return mObject; }
	inline void SetAsyncThreadObject(ForthObject& inObject) { mObject = inObject; }

#if defined(LINUX) || defined(MACOSX)
	static void* RunLoop(void *pThis);
#else
	static unsigned __stdcall RunLoop(void *pThis);
#endif

	friend class ForthEngine;

protected:
	ForthObject			mObject;
	std::vector<ForthThread*> mSoftThreads;
	ForthAsyncThread*   mpNext;
	int					mActiveThreadIndex;
	eForthThreadRunState mRunState;
#if defined(LINUX) || defined(MACOSX)
	int                 mHandle;
	pthread_t           mThread;
	int					mExitStatus;
    sem_t               mExitedSignal;
#else
    HANDLE              mHandle;
	ulong               mThreadId;
    HANDLE              mExitedSignal;
#endif
};

namespace OThread
{
	void AddClasses(ForthEngine* pEngine);

	void CreateAsyncThreadObject(ForthObject& outAsyncThread, ForthEngine *pEngine, long threadOp, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	void FixupAsyncThread(ForthAsyncThread* pAsyncThread);
	void CreateThreadObject(ForthObject& outThread, ForthAsyncThread *pParentAsyncThread, ForthEngine *pEngine, long threadOp, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_RSTACK_SIZE);
	void FixupThread(ForthThread* pThread);
}

namespace OLock
{
	void AddClasses(ForthEngine* pEngine);

	void CreateAsyncLockObject(ForthObject& outAsyncLock, ForthEngine *pEngine);
    void CreateAsyncSemaphoreObject(ForthObject& outSemaphore, ForthEngine *pEngine);
}
