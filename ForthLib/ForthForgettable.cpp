//////////////////////////////////////////////////////////////////////
//
// ForthForgettable.cpp: implementation of the ForthForgettable abstract base class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthForgettable.h"
#include "ForthEngine.h"
#include "ForthShell.h"

//############################################################################
//
//   allocatable memory blocks that are cleaned up on "forget"
//
//############################################################################

//////////////////////////////////////////////////////////////////////
////
///     ForthForgettable - abstract forgettable base class
//
// a forgettable object is associated with a forth op address, when a "forget"
// causes that forth op to be destroyed, the forgettable chain is walked, and
// all forgettables which are associated with ops that have been destroyed
// are also deleted...

ForthForgettable* ForthForgettable::mpChainHead = NULL;

ForthForgettable::ForthForgettable( void* pOpAddress, long op )
: mpNext( mpChainHead )
, mpOpAddress( pOpAddress )
, mOp( op )
{
    mpChainHead = this;
}

ForthForgettable::~ForthForgettable()
{
    // remove us from the forgettable chain
    if ( this == mpChainHead )
    {
        mpChainHead = mpNext;
    }
    else
    {
        ForthForgettable* pNext = mpChainHead;
        while ( pNext != NULL )
        {
            ForthForgettable* pTmp = pNext->mpNext;

            if ( pTmp == this )
            {
                pNext->mpNext = pTmp->mpNext;
                break;
            }
            pNext = pTmp;
        }
    }
}

void ForthForgettable::ForgetPropagate( void* pForgetLimit, long op )
{
    ForthForgettable *pNext;
    ForthForgettable *pTmp;

    // give each forgettable a chance to do internal cleanup
    pNext = mpChainHead;
    while ( pNext != NULL )
    {
        pNext->ForgetCleanup( pForgetLimit, op );
        pNext = pNext->mpNext;
    }
    // delete all forgettables that are below the forget limit
    pNext = mpChainHead;
    while ( pNext != NULL )
    {
        pTmp = pNext->mpNext;
        if ( (ulong) pNext->mpOpAddress > (ulong) pForgetLimit )
        {
            delete pNext;
        }
        pNext = pTmp;
    }
}



