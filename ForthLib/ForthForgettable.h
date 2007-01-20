//////////////////////////////////////////////////////////////////////
//
// ForthVocabulary.h: interface for the ForthVocabulary class.
//
//////////////////////////////////////////////////////////////////////

#if !defined( FORTH_FORGETTABLE_H )
#define FORTH_FORGETTABLE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"

class ForthForgettable
{
public:
    ForthForgettable( void *pOpAddress, long op );
    virtual ~ForthForgettable();

    static void     ForgetPropagate( void *pForgetLimit, long op );

protected:
    virtual void    ForgetCleanup( void *pForgetLimit, long op ) = 0;

    void*                       mpOpAddress;
    long                        mOp;
    ForthForgettable*           mpNext;
private:
    static ForthForgettable*    mpChainHead;
};

#endif

