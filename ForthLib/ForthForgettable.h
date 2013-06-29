#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthForgettable.h: interface for the ForthForgettable class.
//
//////////////////////////////////////////////////////////////////////

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


