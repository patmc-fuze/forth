//////////////////////////////////////////////////////////////////////
//
// ForthInner.h: inner interpreter state
//
//////////////////////////////////////////////////////////////////////

#if !defined(_FORTH_INNER_H_INCLUDED_)
#define _FORTH_INNER_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"

//class ForthEngine;

extern "C" {

// VAR_ACTIONs are subops of a variable op (store/fetch/incStore/decStore)
#define VAR_ACTION(NAME) static void NAME( ForthCoreState *pCore )
typedef void (*VarAction)( ForthCoreState *pCore );

#define OPTYPE_ACTION(NAME) static void NAME( ForthCoreState *pCore, ulong opVal )

// right now there are about 250 builtin ops, allow for future expansion
#define MAX_BUILTIN_OPS 512


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
    
    long                *TPV;      // this pointer (vtable)
    long                *TPD;      // this pointer (data)

    void                *pPrivate; // pointer to per-thread state
    long                currentOp; // last op dispatched by inner interpreter
    
    ulong               varMode;        // operation to perform on variables

    ulong               state;          // inner loop state - ok/done/error

    ulong               error;

    void                *pConOutData;
    consoleOutRoutine   consoleOut;
    long                consoleOutOp;

    FILE                *pDefaultOutFile;
    FILE                *pDefaultInFile;
    long                base;      // output base
    ulong               signedPrintMode;   // if numers are printed as signed/unsigned
};


struct ForthCoreState
{
    optypeActionRoutine  optypeAction[ 256 ];

    ForthOp             *builtinOps;
    ulong               numBuiltinOps;

    long                **userOps;
    ulong               numUserOps;
    ulong               maxUserOps;     // current size of table at pUserOps

    //ForthEngine         *pEngine;
   void                 *pEngine;

    // *** beginning of stuff which is per thread ***
    ForthThreadState    *pThread;       // pointer to current thread state

    long                *IP;            // interpreter pointer

    long                *SP;            // parameter stack pointer
    
    long                *RP;            // return stack pointer

    long                *FP;            // frame pointer
    
    long                *TPV;           // this pointer (vtable)
    long                *TPD;           // this pointer (data)

    ulong               varMode;        // operation to perform on variables

    ulong               state;          // inner loop state - ok/done/error

    ulong               error;

    long                *ST;            // empty parameter stack pointer

    ulong               SLen;           // size of param stack in longwords

    long                *RT;            // empty return stack pointer

    ulong               RLen;           // size of return stack in longwords
    // *** end of stuff which is per thread ***

    // user dictionary stuff
    long                *DP;            // dictionary pointer
    long                *DBase;         // base of dictionary
    ulong               DLen;           // max size of dictionary memory segment
};


extern eForthResult InnerInterpreter( ForthCoreState *pCore );
extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
extern InitAsmTables( ForthCoreState *pCore );

void InitDispatchTables( ForthCoreState* pCore );
void InitCore( ForthCoreState* pCore );
void CoreSetError( ForthCoreState *pCore, eForthError error, bool isFatal );

// DLLRoutine is used for any external DLL routine - it can take any number of arguments
typedef long (*DLLRoutine)();
// CallDLLRoutine is an assembler routine which:
// 1) moves arguments from the forth parameter stack to the real stack in reverse order
// 2) calls the DLL routine
// 3) leaves the DLL routine result on the forth parameter stack
extern long CallDLLRoutine( DLLRoutine function, long argCount, ForthCoreState *pCore );

inline long GetCurrentOp( ForthCoreState *pCore )
{
    long *pIP = pCore->IP - 1;
    return *pIP;
}


#define GET_IP                          (pCore->IP)
#define SET_IP( A )                     (pCore->IP = A)

#define GET_SP                          (pCore->SP)
#define SET_SP( A )                     (pCore->SP = A)

#define GET_RP                          (pCore->RP)
#define SET_RP( A )                     (pCore->RP = A)

#define GET_FP                          (pCore->FP)
#define SET_FP( A )                     (pCore->FP = A)

#define GET_TPV                         (pCore->TPV)
#define GET_TPD                         (pCore->TPD)
#define SET_TPV( A )                    (pCore->TPV = A)
#define SET_TPD( A )                    (pCore->TPD = A)

#define GET_DP                          (pCore->DP)
#define SET_DP( A )                     (pCore->DP = A)

#define SPOP                            (*pCore->SP++)
#define SPUSH( A )                      (*--pCore->SP = A)

#define FPOP                            (*(float *)pCore->SP++)
#define FPUSH( A )                      --pCore->SP; *(float *)pCore->SP = A

#define DPOP                            *((double *)(pCore->SP)); pCore->SP += 2
#define DPUSH( A )                      pCore->SP -= 2; *((double *)(pCore->SP)) = A

#define RPOP                            (*pCore->RP++)
#define RPUSH( A )                      (*--pCore->RP = A)

#define GET_SDEPTH                      (pCore->ST - pCore->SP)
#define GET_RDEPTH                      (pCore->RT - pCore->RP)

#define GET_STATE                       (eForthResult)(pCore->state)
#define SET_STATE( A )                  (pCore->state = A)

//#define GET_ENGINE                      (pCore->pEngine)
#define GET_ENGINE                      (ForthEngine::GetInstance())

#define GET_VAR_OPERATION               (pCore->varMode)
#define SET_VAR_OPERATION( A )          (pCore->varMode = A)
#define CLEAR_VAR_OPERATION             (pCore->varMode = kVarFetch)

#define GET_NUM_USER_OPS                (pCore->numUserOps)

#define GET_CURRENT_OP                  GetCurrentOp( pCore )

#define USER_OP_TABLE                   (pCore->userOps)

#define SET_ERROR( A )                  CoreSetError( pCore, A, false )
#define SET_FATAL_ERROR( A )            CoreSetError( pCore, A, true )

#define GET_CON_OUT_DATA                (pCore->pThread->pConOutData)
#define SET_CON_OUT_DATA( A )           (pCore->pThread->pConOutData = A)
#define SET_CON_OUT_ROUTINE( A )        (pCore->pThread->consoleOut = A)
#define CONSOLE_STRING_OUT( A )         (pCore->pThread->consoleOut( pCore, A ))

#define GET_CON_OUT_OP                  (pCore->pThread->consoleOutOp)
#define SET_CON_OUT_OP( A )             (pCore->pThread->consoleOutOp = A)

#define GET_BASE_REF                    (&pCore->pThread->base)

#define GET_PRINT_SIGNED_NUM_MODE       (pCore->pThread->signedPrintMode)
#define SET_PRINT_SIGNED_NUM_MODE( A )  (pCore->pThread->signedPrintMode = A)

};      // end extern "C"

#endif
