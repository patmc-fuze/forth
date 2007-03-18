//////////////////////////////////////////////////////////////////////
//
// ForthCore.h: definitions shared by C++ and assembler code
//
//////////////////////////////////////////////////////////////////////

#if !defined(_FORTH_CORE_H_INCLUDED_)
#define _FORTH_CORE_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


typedef enum {
    kVarFetch = 0,
    kVarRef,
    kVarStore,
    kVarPlusStore,
    kVarMinusStore
} varOperation;

typedef enum {
    kResultOk,          // no need to exit
    kResultDone,        // exit because of "done" opcode
    kResultExitShell,   // exit because of a "bye" opcode
    kResultError,       // exit because of error
    kResultFatalError,  // exit because of fatal error
    kResultException,   // exit because of uncaught exception
} eForthResult;

typedef enum {
    kForthErrorNone,
    kForthErrorBadOpcode,
    kForthErrorBadOpcodeType,
    kForthErrorParamStackUnderflow,
    kForthErrorParamStackOverflow,
    kForthErrorReturnStackUnderflow,
    kForthErrorReturnStackOverflow,
    kForthErrorUnknownSymbol,
    kForthErrorFileOpen,
    kForthErrorAbort,
    kForthErrorForgetBuiltin,
    kForthErrorBadMethod,
    kForthErrorException,
    kForthErrorMissingSize,
    kForthErrorBadSyntax,
    // NOTE: if you add errors, make sure that you update ForthEngine::GetErrorString
    kForthNumErrors
} eForthError;

// how sign should be handled while printing integers
typedef enum {
    kPrintSignedDecimal,
    kPrintAllSigned,
    kPrintAllUnsigned
} ePrintSignedMode;

typedef enum
{
    kOpBuiltIn = 0,
    kOpUserDef,

    kOpBranch,          // low 24 bits is signed branch offset
    kOpBranchNZ,
    kOpBranchZ,
    kOpCaseBranch,

    kOpConstant,        // low 24 bits is signed symbol value
    kOpOffset,          // low 24 bits is value to add to TOS

    kOpConstantString,

    kOpAllocLocals,     // low 24 bits is frame size in longs
    kOpInitLocalString,     // bits 0..11 are string length in bytes, bits 12..23 are frame offset

    kOpLocalInt,
    kOpLocalFloat,
    kOpLocalDouble,
    kOpLocalString,

    kOpFieldInt,
    kOpFieldFloat,
    kOpFieldDouble,
    kOpFieldString,

    kOpMethodWithThis,  // low 24 bits is method number

    kOpMemberInt,
    kOpMemberFloat,
    kOpMemberDouble,
    kOpMemberString,

    kOpLocalUserDefined,             // user can add more optypes starting with this one
    kOpMaxLocalUserDefined = 127,    // maximum user defined optype

    kOpUserMethods  = 128
    // optypes from 128...255 are used to select class methods    
} forthOpType;

#define ulong  unsigned long
#define ForthEngine void
#define ForthThreadState void

// there is an action routine with this signature for each forthOpType
// user can add new optypes with ForthEngine::AddOpType
typedef void (*optypeActionRoutine)( void *pCore, ulong theData );

typedef void  (*ForthOp)(void *);

struct ForthCoreState
{
    optypeActionRoutine  optypeAction[ 256 ];

    ForthOp             *builtinOps;
    ulong               numBuiltinOps;

    long                **userOps;
    ulong               numUserOps;
    ulong               maxUserOps;     // current size of table at pUserOps

    ForthEngine         *pEngine;

    // *** beginning of stuff which is per thread ***
    ForthThreadState    *pThread;       // pointer to current thread state

    long                *IPtr;          // interpreter pointer

    long                *SPtr;          // parameter stack pointer
    
    long                *RPtr;          // return stack pointer

    long                *FPtr;           // frame pointer
    
    long                *TPtr;           // this pointer

    varOperation        varMode;        // operation to perform on variables

    eForthResult        state;          // inner loop state - ok/done/error

    eForthError         error;

    long                *STPtr;         // empty parameter stack pointer

    ulong               SLen;           // size of param stack in longwords

    long                *RTPtr;          // empty return stack pointer

    ulong               RLen;           // size of return stack in longwords

    // *** end of stuff which is per thread ***

    // user dictionary stuff
    long                *DP;            // dictionary pointer
    long                *DBase;         // base of dictionary
    ulong               DLen;           // max size of dictionary memory segment
};

#endif
