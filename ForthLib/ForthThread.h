//////////////////////////////////////////////////////////////////////
//
// ForthThread.h: interface for the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_FORTH_THREAD_H_INCLUDED_)
#define _FORTH_THREAD_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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
    friend ForthEngine;

public:
    ForthThread( ForthEngine *pEngine, int paramStackLongs=DEFAULT_PSTACK_SIZE, int returnStackLongs=DEFAULT_PSTACK_SIZE );
    virtual ~ForthThread();

#ifdef CHECK_GAURD_AREAS
    bool CheckGaurdAreas();
#endif

    virtual void Activate( ForthCoreState* pCore );
    virtual void Deactivate( ForthCoreState* pCore );

    void                Reset( void );

    inline void         SetIP( long *pNewIP ) { mState.IP = pNewIP; };

protected:
    ForthEngine         *mpEngine;
    ForthThread         *mpNext;
    
    ForthThreadState    mState;
};

#endif
