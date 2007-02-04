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

class ForthEngine;

#define DEFAULT_PSTACK_SIZE 128
#define DEFAULT_RSTACK_SIZE 128

struct ForthThreadState
{
    optypeActionRoutine  optypeAction[ 256 ];
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

    virtual void Activate( ForthCoreState& core );
    virtual void Deactivate( ForthCoreState& core );

    void                Reset( void );
    void                GetErrorString( char *pString );
    eForthResult        CheckStacks( void );

    inline long *       GetIP( void ) { return mState.IP; };
    inline void         SetIP( long *pNewIP ) { mState.IP = pNewIP; };

    inline long *       GetSP( void ) { return mState.SP; };
    inline void         SetSP( long *pNewSP ) { mState.SP = pNewSP; };
    inline long         GetSSize( void ) { return mState.SLen; };
    inline long         GetSDepth( void ) { return mState.ST - mState.SP; };
    inline void         EmptySStack( void ) { mState.SP = mState.ST; };
    inline void         Push( long v ) { *--mState.SP = v; };
    inline void         FPush( float v ) { --mState.SP; *(float *)mState.SP = v; };
    inline void         DPush( double v ) { mState.SP -= 2; *(double *)mState.SP = v; };
    inline long         Pop( void ) { return *mState.SP++; };
    inline float        FPop( void ) { return *(float*)mState.SP++; };
    inline double       DPop( void ) { double dVal = *(double*)mState.SP; mState.SP += 2;  return dVal; };

    inline long *       GetRP( void ) { return mState.RP; };
    inline void         SetRP( long *pNewRP ) { mState.RP = pNewRP; };
    inline long         GetRSize( void ) { return mState.RLen; };
    inline long         GetRDepth( void ) { return mState.RT - mState.RP; };
    inline void         EmptyRStack( void ) { mState.RP = mState.RT; };
    inline void         RPush( long v ) { *--mState.RP = v; };
    inline void         RFPush( float v ) { --mState.RP; *(float *)mState.RP = v; };
    inline void         RDPush( double v ) { mState.RP -= 2; *(double *)mState.RP = v; };
    inline long         RPop( void ) { return *mState.RP++; };
    inline float        RFPop( void ) { return *(float*)mState.RP++; };
    inline double       RDPop( void ) { double dVal = *(double*)mState.RP; mState.RP += 2;  return dVal; };

    inline long *       GetFP( void ) { return mState.FP; };
    inline void         SetFP( long *pNewFP ) { mState.FP = pNewFP; };

    inline long *       GetTP( void ) { return mState.TP; };
    inline void         SetTP( long *pNewTP ) { mState.TP = pNewTP; };

    inline varOperation GetVarOperation( void ) { return mState.varMode; };
    inline void         SetVarOperation( varOperation op ) { mState.varMode = op; };
    inline void         ClearVarOperation( void ) { mState.varMode = kVarFetch; };

    inline void *       GetPrivate( void ) { return mState.pPrivate; };
    inline void         SetPrivate( void *pPrivate ) { mState.pPrivate = pPrivate; };

    inline long         GetCurrentOp( void ) { return mState.currentOp; }
    inline void         SetCurrentOp( long op ) { mState.currentOp = op; }

    inline ForthEngine *GetEngine( void ) { return mpEngine; };
    
    void                SetError( eForthError e, const char *pString = NULL );
    void                SetFatalError( eForthError e, const char *pString = NULL );
    inline eForthError  GetError( void ) { return mState.error; };

    inline void         SetState( eForthResult s ) { mState.state = s; };
    inline eForthResult GetState( void ) { return mState.state; };

    inline FILE *       GetConOutFile( void ) { return mState.pConOutFile; };
    inline void         SetConOutFile( FILE *pFile ) { mState.pConOutFile = pFile; };
    inline void         SetConOutString( char *pStr ) { mState.pConOutStr = pStr; };
    inline char *       GetConOutString( void ) { return mState.pConOutStr; };
    inline long *       GetBaseRef( void ) { return &mState.base; };
    inline void         SetPrintSignedNumMode( ePrintSignedMode mode ) { mState.signedPrintMode = mode; };
    inline ePrintSignedMode GetPrintSignedNumMode( void ) { return mState.signedPrintMode; };

    inline void         SetDefaultOutFile( FILE *pFile ) { mState.pDefaultOutFile = pFile; };
    inline void         SetDefaultInFile( FILE *pFile ) { mState.pDefaultInFile = pFile; };
    inline FILE *       GetDefaultOutFile( void ) { return mState.pDefaultOutFile; };
    inline FILE *       GetDefaultInFile( void ) { return mState.pDefaultInFile; };

protected:
    ForthEngine         *mpEngine;
    ForthThread         *mpNext;
    
    ForthThreadState    mState;
};

#endif
