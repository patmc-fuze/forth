#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthThread.h: interface for the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"
#include "ForthInner.h"
#ifdef LINUX
#include <pthread.h>
#endif

class ForthEngine;
class ForthShowContext;

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
    ForthThread( ForthEngine *pEngine, int paramStackLongs=DEFAULT_PSTACK_SIZE, int returnStackLongs=DEFAULT_PSTACK_SIZE );
    virtual ~ForthThread();

#ifdef CHECK_GAURD_AREAS
    bool CheckGaurdAreas();
#endif

    void                Reset( void );
    void                ResetIP( void );

    inline void         SetOp( long op ) { mOps[0] = op; };

    inline void         Push( long value ) { *--mCore.SP = value; };
    inline long         Pop() { return *mCore.SP++; };
    inline void         RPush( long value ) { *--mCore.RP = value; };
    inline long         RPop() { return *mCore.RP++; };

	void				Run();

    inline ulong        WakeupTime() { return mWakeupTime; };

	inline void			SetIP( long* newIP ) { mCore.IP = newIP; };
	
	ForthShowContext*	GetShowContext();

    friend class ForthEngine;

#ifdef LINUX
    static void* RunLoop( void *pThis );
#else
    static unsigned __stdcall RunLoop( void *pThis );
#endif

	inline ForthCoreState* GetCore() { return &mCore; };

protected:
    ForthEngine         *mpEngine;
    ForthThread         *mpNext;
    void                *mpPrivate;
	ForthShowContext	*mpShowContext;

    //ForthThreadState    mState;
    ForthCoreState      mCore;
    long                mOps[2];
    unsigned long       mWakeupTime;
    ulong               mThreadId;
};

class ForthAsyncThread : public ForthThread
{
public:
	ForthAsyncThread(ForthEngine *pEngine, int paramStackLongs = DEFAULT_PSTACK_SIZE, int returnStackLongs = DEFAULT_PSTACK_SIZE);
	virtual ~ForthAsyncThread();

	long                Start();
	void                Exit();

protected:
#ifdef LINUX
	int                 mHandle;
	pthread_t           mThread;
	int					mExitStatus;
#else
	HANDLE              mHandle;
#endif
};

namespace OThread
{
	void AddClasses(ForthEngine* pEngine);
}

namespace OLock
{
	void AddClasses(ForthEngine* pEngine);
}
