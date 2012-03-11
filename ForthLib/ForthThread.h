#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthThread.h: interface for the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"
#include "ForthInner.h"

class ForthEngine;

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

    inline void         SetOp( long op ) { mOps[0] = op; };

    inline void         Push( long value ) { *--mCore.SP = value; };
    inline long         Pop() { return *mCore.SP++; };
    inline void         RPush( long value ) { *--mCore.RP = value; };
    inline long         RPop() { return *mCore.RP++; };

    long                Start();
    void                Exit();

	void				Run();

    inline ulong        WakeupTime() { return mWakeupTime; };

	inline void			SetIP( long* newIP ) { mCore.IP = newIP; };

    friend class ForthEngine;

    static unsigned __stdcall RunLoop( void *pThis );

	inline ForthCoreState* GetCoreState() { return &mCore; };

protected:
    ForthEngine         *mpEngine;
    ForthThread         *mpNext;
    void                *mpPrivate;
    
    //ForthThreadState    mState;
    ForthCoreState      mCore;
    long                mOps[2];
    unsigned long       mWakeupTime;
    HANDLE              mHandle;
    ulong               mThreadId;
};

class ForthThreadQueue
{
public:
    ForthThreadQueue( int initialSize=16 );
    ~ForthThreadQueue();

    void                AddThread( ForthThread* pThread );

    // how many threads are in queue
    int                 Count();

    // returns NULL if queue is empty
    ForthThread*        RemoveThread();

    // returns NULL if index is out of range
    ForthThread*        RemoveThread( int index );

    // returns NULL if index is out of range
    ForthThread*        GetThread( int index );

    // returns -1 if queue is empty
    int                 FindEarliest();

protected:
    ForthThread**   mQueue;
    int             mFirst;
    int             mCount;
    int             mSize;
};

