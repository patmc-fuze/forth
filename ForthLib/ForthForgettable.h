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

    static inline ForthForgettable *GetForgettableChainHead( void ) {
        return mpChainHead;
    };

    inline ForthForgettable *GetNextForgettable( void ) {
        return mpNext;
    };

    // type of forgettable, like 'vocabulary' or 'globalObject'
    virtual const char* GetTypeName();
    virtual const char* GetName();

    virtual void AfterStart();
    virtual int Save( FILE* pOutFile );
    // return false
    virtual bool Restore( const char* pBuffer, unsigned int numBytes );

protected:
    virtual void    ForgetCleanup( void *pForgetLimit, long op ) = 0;

    void*                       mpOpAddress;
    long                        mOp;
    ForthForgettable*           mpNext;
private:
    static ForthForgettable*    mpChainHead;
};


