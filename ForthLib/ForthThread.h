// ForthThread.h: interface for the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORTHTHREAD_H__C43FADC3_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
#define AFX_FORTHTHREAD_H__C43FADC3_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"

class ForthEngine;

class ForthThread  
{
    friend ForthEngine;

public:
	ForthThread( ForthEngine *pEngine, int paramStackSize, int returnStackSize );
	virtual ~ForthThread();

    void                Reset( void );
    void                GetErrorString( char *pString );

    inline long *       GetIP( void ) { return mIP; };
    inline void         SetIP( long *pNewIP ) { mIP = pNewIP; };

    inline long *       GetSP( void ) { return mSP; };
    inline void         SetSP( long *pNewSP ) { mSP = pNewSP; };
    inline long         GetSSize( void ) { return mSLen; };
    inline long         GetSDepth( void ) { return mST - mSP; };
    inline void         EmptySStack( void ) { mSP = mST; };
    inline void         Push( long v ) { *--mSP = v; };
    inline void         FPush( float v ) { --mSP; *(float *)mSP = v; };
    inline void         DPush( double v ) { --mSP; *(double *)mSP = v; };
    inline long         Pop( void ) { return *mSP++; };
    inline float        FPop( void ) { return *(float*)mSP++; };
    inline double       DPop( void ) { return *(double*)mSP++; };

    inline long *       GetRP( void ) { return mRP; };
    inline void         SetRP( long *pNewRP ) { mRP = pNewRP; };
    inline long         GetRSize( void ) { return mRLen; };
    inline long         GetRDepth( void ) { return mRT - mRP; };
    inline void         EmptyRStack( void ) { mRP = mRT; };
    inline void         RPush( long v ) { *--mRP = v; };
    inline void         RFPush( float v ) { --mRP; *(float *)mRP = v; };
    inline void         RDPush( double v ) { --mRP; *(double *)mRP = v; };
    inline long         RPop( void ) { return *mRP++; };
    inline float        RFPop( void ) { return *(float*)mRP++; };
    inline double       RDPop( void ) { return *(double*)mRP++; };

    inline long *       GetFP( void ) { return mFP; };
    inline void         SetFP( long *pNewFP ) { mFP = pNewFP; };

    inline varOperation GetVarOperation( void ) { return mVarMode; };
    inline void         SetVarOperation( varOperation op ) { mVarMode = op; };
    inline void         ClearVarOperation( void ) { mVarMode = kVarFetch; };

    inline void *       GetPrivate( void ) { return mpPrivate; };
    inline void         SetPrivate( void *pPrivate ) { mpPrivate = pPrivate; };

    inline ForthEngine *GetEngine( void ) { return mpEngine; };
    
    inline void         SetError( eForthError e ) { mState = kResultError; mError = e; };
    inline void         SetFatalError( eForthError e ) { mState = kResultFatalError; mError = e; };
    inline eForthError  GetError( void ) { return mError; };

    inline void         SetState( eForthResult s ) { mState = s; };

    inline FILE *       GetConOutFile( void ) { return mpConOutFile; };
    inline void         SetConOutFile( FILE *pFile ) { mpConOutFile = pFile; };
    inline void         SetConOutString( char *pStr ) { mpConOutStr = pStr; };
    inline char *       GetConOutString( void ) { return mpConOutStr; };
    inline long *       GetBaseRef( void ) { return &mBase; };
    inline void         SetPrintSignedNumMode( ePrintSignedMode mode ) { mSignedPrintMode = mode; };
    inline ePrintSignedMode GetPrintSignedNumMode( void ) { return mSignedPrintMode; };

protected:
    ForthEngine         *mpEngine;
    ForthThread         *mpNext;
    
    long                *mIP;       // interpreter pointer
    
    long                *mSP;       // parameter stack pointer
    long                *mST;       // empty parameter stack pointer
    long                *mSB;       // param stack base
    ulong               mSLen;      // size of param stack in longwords
    
    long                *mRP;       // return stack pointer
    long                *mRT;       // empty return stack pointer
    long                *mRB;       // return stack base
    ulong               mRLen;      // size of return stack

    long                *mFP;       // frame pointer
    
    void                *mpPrivate; // pointer to per-thread state
    long                mToken;     // last token dispatched by inner interpreter
    
    varOperation        mVarMode;   // operation to perform on variables

    eForthError         mError;
    eForthResult        mState;     // inner loop state - ok/done/error

    FILE                *mpConOutFile;
    char                *mpConOutStr;

    long                mBase;      // output base
    ePrintSignedMode    mSignedPrintMode;   // if numers are printed as signed/unsigned
};

#endif // !defined(AFX_FORTHTHREAD_H__C43FADC3_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
