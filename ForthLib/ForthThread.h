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

#define DEFAULT_PSTACK_SIZE 128
#define DEFAULT_RSTACK_SIZE 128

struct ForthThreadState
{
    long                *IP;       // interpreter pointer

    long                *SP;       // parameter stack pointer
    long                *ST;       // empty parameter stack pointer
    long                *SB;       // param stack base
    ulong               SLen;      // size of param stack in longwords
    
    long                *RP;       // return stack pointer
    long                *RT;       // empty return stack pointer
    long                *RB;       // return stack base
    ulong               RLen;      // size of return stack

    long                *FP;       // frame pointer
    
    long                *TP;       // this pointer

    void                *pPrivate; // pointer to per-thread state
    long                currentOp; // last op dispatched by inner interpreter
    
    varOperation        varMode;   // operation to perform on variables

    eForthError         error;
    eForthResult        state;     // inner loop state - ok/done/error
    const char *        pErrorString;  // optional error information from shell

    FILE                *pConOutFile;
    char                *pConOutStr;

    FILE                *pDefaultOutFile;
    FILE                *pDefaultInFile;
    long                base;      // output base
    ePrintSignedMode    signedPrintMode;   // if numers are printed as signed/unsigned
};

class ForthThread  
{
    friend ForthEngine;

public:
    ForthThread( ForthEngine *pEngine, int paramStackLongs=DEFAULT_PSTACK_SIZE, int returnStackLongs=DEFAULT_PSTACK_SIZE );
    virtual ~ForthThread();

    //
    // ExecuteOneOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
    // execute forth ops, and is also how systems external to forth execute ops
    //
    eForthResult        ExecuteOneOp( long opCode );

    void                Reset( void );
    void                GetErrorString( char *pString );
    eForthResult        CheckStacks( void );

    inline long *       GetIP( void ) { return mIP; };
    inline void         SetIP( long *pNewIP ) { mIP = pNewIP; };

    inline long *       GetSP( void ) { return mSP; };
    inline void         SetSP( long *pNewSP ) { mSP = pNewSP; };
    inline long         GetSSize( void ) { return mSLen; };
    inline long         GetSDepth( void ) { return mST - mSP; };
    inline void         EmptySStack( void ) { mSP = mST; };
    inline void         Push( long v ) { *--mSP = v; };
    inline void         FPush( float v ) { --mSP; *(float *)mSP = v; };
    inline void         DPush( double v ) { mSP -= 2; *(double *)mSP = v; };
    inline long         Pop( void ) { return *mSP++; };
    inline float        FPop( void ) { return *(float*)mSP++; };
    inline double       DPop( void ) { double dVal = *(double*)mSP; mSP += 2;  return dVal; };

    inline long *       GetRP( void ) { return mRP; };
    inline void         SetRP( long *pNewRP ) { mRP = pNewRP; };
    inline long         GetRSize( void ) { return mRLen; };
    inline long         GetRDepth( void ) { return mRT - mRP; };
    inline void         EmptyRStack( void ) { mRP = mRT; };
    inline void         RPush( long v ) { *--mRP = v; };
    inline void         RFPush( float v ) { --mRP; *(float *)mRP = v; };
    inline void         RDPush( double v ) { mRP -= 2; *(double *)mRP = v; };
    inline long         RPop( void ) { return *mRP++; };
    inline float        RFPop( void ) { return *(float*)mRP++; };
    inline double       RDPop( void ) { double dVal = *(double*)mRP; mRP += 2;  return dVal; };

    inline long *       GetFP( void ) { return mFP; };
    inline void         SetFP( long *pNewFP ) { mFP = pNewFP; };

    inline long *       GetTP( void ) { return mTP; };
    inline void         SetTP( long *pNewTP ) { mTP = pNewTP; };

    inline varOperation GetVarOperation( void ) { return mVarMode; };
    inline void         SetVarOperation( varOperation op ) { mVarMode = op; };
    inline void         ClearVarOperation( void ) { mVarMode = kVarFetch; };

    inline void *       GetPrivate( void ) { return mpPrivate; };
    inline void         SetPrivate( void *pPrivate ) { mpPrivate = pPrivate; };

    inline ForthEngine *GetEngine( void ) { return mpEngine; };
    
    void                SetError( eForthError e, const char *pString = NULL );
    void                SetFatalError( eForthError e, const char *pString = NULL );
    inline eForthError  GetError( void ) { return mError; };

    inline void         SetState( eForthResult s ) { mState = s; };

    inline FILE *       GetConOutFile( void ) { return mpConOutFile; };
    inline void         SetConOutFile( FILE *pFile ) { mpConOutFile = pFile; };
    inline void         SetConOutString( char *pStr ) { mpConOutStr = pStr; };
    inline char *       GetConOutString( void ) { return mpConOutStr; };
    inline long *       GetBaseRef( void ) { return &mBase; };
    inline void         SetPrintSignedNumMode( ePrintSignedMode mode ) { mSignedPrintMode = mode; };
    inline ePrintSignedMode GetPrintSignedNumMode( void ) { return mSignedPrintMode; };

    inline void         SetDefaultOutFile( FILE *pFile ) { mpDefaultOutFile = pFile; };
    inline void         SetDefaultInFile( FILE *pFile ) { mpDefaultInFile = pFile; };
    inline FILE *       GetDefaultOutFile( void ) { return mpDefaultOutFile; };
    inline FILE *       GetDefaultInFile( void ) { return mpDefaultInFile; };

protected:
    ForthEngine         *mpEngine;
    ForthThread         *mpNext;
    
    ForthThreadState    mState;
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
    
    long                *mTP;       // this pointer

    void                *mpPrivate; // pointer to per-thread state
    long                mCurrentOp; // last op dispatched by inner interpreter
    
    varOperation        mVarMode;   // operation to perform on variables

    eForthError         mError;
    eForthResult        mState;     // inner loop state - ok/done/error
    const char *        mpErrorString;  // optional error information from shell

    FILE                *mpConOutFile;
    char                *mpConOutStr;

    FILE                *mpDefaultOutFile;
    FILE                *mpDefaultInFile;
    long                mBase;      // output base
    ePrintSignedMode    mSignedPrintMode;   // if numers are printed as signed/unsigned
};

#endif // !defined(AFX_FORTHTHREAD_H__C43FADC3_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
