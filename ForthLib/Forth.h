//////////////////////////////////////////////////////////////////////
//
// Forth engine definitions
//   Pat McElhatton   September '00
//
//////////////////////////////////////////////////////////////////////

#ifndef _FORTH_H_INCLUDED_
#define _FORTH_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//#include "stdafx.h"

class ForthEngine;
class ForthShell;
class ForthThread;
class ForthVocabulary;
class ForthInputStack;
class ForthInputStream;
class ForthForgettable;
struct ForthCoreState;

#ifndef ulong
#define ulong   unsigned long
#endif

// these are opcode types, they are held in the top byte of an opcode, and in
// a vocabulary entry value field
typedef enum
{
    kOpBuiltIn = 0,
    kOpUserDef,

    kOpBranch,          // low 24 bits is signed branch offset
    kOpBranchNZ,
    kOpBranchZ,
    kOpCaseBranch,

    kOpConstant,        // low 24 bits is signed symbol value
    kOpOffset,          // low 24 bits is signed offset value

    kOpString,

    kOpAllocLocals,     // low 24 bits is frame size in longs

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

// there is an action routine with this signature for each forthOpType
// user can add new optypes with ForthEngine::AddOpType
typedef void (*optypeActionRoutine)( ForthCoreState *pCore, ulong theData );

typedef void  (*ForthOp)( ForthCoreState * );

// user will also have to add an external interpreter with ForthEngine::SetInterpreterExtension
// to compile/interpret these new optypes
// return true if the extension has recognized and processed the symbol
typedef bool (*interpreterExtensionRoutine)( ForthEngine *pEngine, char *pToken );


// the varMode state makes variables do something other
//  than their default behaviour (fetch)
typedef enum {
    kVarFetch = 0,
    kVarRef,
    kVarStore,
    kVarPlusStore,
    kVarMinusStore
} varOperation;

#define DEFAULT_INPUT_BUFFER_LEN   256

// these are the results of running the inner interpreter
typedef enum {
    kResultOk,          // no need to exit
    kResultDone,        // exit because of "done" opcode
    kResultExitShell,   // exit because of a "bye" opcode
    kResultError,       // exit because of error
    kResultFatalError,  // exit because of fatal error
    kResultException,   // exit because of uncaught exception
} eForthResult;

#define FLAG_DONE           1
#define FLAG_BYE            (1 << 1)
#define FLAG_ERROR          (1 << 2)
#define FLAG_FATAL_ERROR    (1 << 3)

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


// the bottom 24 bits of a forth opcode is a value field
// the top 8 bits is the type field
#define OPCODE_VALUE_MASK   0xFFFFFF
#define FORTH_OP_TYPE( OP )  ( (forthOpType) (((OP) >> 24) & 0xFF) )
#define FORTH_OP_VALUE( OP ) ( (OP) & OPCODE_VALUE_MASK )


#define NEEDS(A)
#define RNEEDS(A)

class ForthThread;

#define COMPILED_OP( OP_TYPE, VALUE ) ((OP_TYPE << 24) | (VALUE & OPCODE_VALUE_MASK))
#define BUILTIN_OP( INDEX )   COMPILED_OP( kOpBuiltIn, INDEX )
// These are opcodes that built-in ops must compile directly
// NOTE: the index field of these opcodes must agree with the
//  order of builtin dictionary entries in the ForthOps.cpp file
#define OP_ABORT        BUILTIN_OP(0)
#define OP_DROP         BUILTIN_OP(1)
#define OP_DO_DOES      BUILTIN_OP(2)
#define OP_INT_VAL      BUILTIN_OP(3)
#define OP_FLOAT_VAL    BUILTIN_OP(4)
#define OP_DOUBLE_VAL   BUILTIN_OP(5)
#define OP_DO_VAR       BUILTIN_OP(6)
#define OP_DO_CONSTANT  BUILTIN_OP(7)
#define OP_DO_DCONSTANT BUILTIN_OP(8)
#define OP_END_BUILDS   BUILTIN_OP(9)
#define OP_DONE         BUILTIN_OP(10)
#define OP_DO_INT       BUILTIN_OP(11)
#define OP_DO_FLOAT     BUILTIN_OP(12)
#define OP_DO_DOUBLE    BUILTIN_OP(13)
#define OP_DO_STRING    BUILTIN_OP(14)
#define OP_INTO         BUILTIN_OP(15)
#define OP_DO_DO        BUILTIN_OP(16)
#define OP_DO_LOOP      BUILTIN_OP(17)
#define OP_DO_LOOPN     BUILTIN_OP(18)
#define OP_DO_EXIT      BUILTIN_OP(19)
#define OP_DO_EXIT_L    BUILTIN_OP(20)
#define OP_DO_EXIT_M    BUILTIN_OP(21)
#define OP_DO_EXIT_ML   BUILTIN_OP(22)
#define OP_DO_VOCAB     BUILTIN_OP(23)
#define OP_PLUS         BUILTIN_OP(24)

#define BASE_DICT_PRECEDENCE_FLAG 0x100
typedef struct {
   char             *name;
   ulong            flags;       // forthOpType is bits 0:7, bit 8 is precedence
   ulong            value;
   int              precedence;
} baseDictEntry;

//
// this is the descriptor for a class defined in forth
//
// pOpTable points to a table holding numMethods opcodes
// pBaseClass points to this class's base class, or NULL if there is none
// opcode is the forth op for the class's defining word
//
typedef struct _ForthClassDescriptor {
    ulong                           numMethods;
    ulong                           opcode;
    ulong                           *pOpTable;
    struct _ForthClassDescriptor    *pBaseClass;
} ForthClassDescriptor;


// trace output flags
//#define TRACE_PRINTS
#define TRACE_OUTER_INTERPRETER
#define TRACE_INNER_INTERPRETER
#define TRACE_SHELL
#define TRACE_VOCABULARY

#ifdef TRACE_PRINTS
#define SPEW_PRINTS TRACE
#else
#define SPEW_PRINTS TRACE
//#define SPEW_PRINTS(...)
#endif

#ifdef TRACE_OUTER_INTERPRETER
#define SPEW_OUTER_INTERPRETER TRACE
#else
#define SPEW_OUTER_INTERPRETER(...)
#endif

#ifdef TRACE_INNER_INTERPRETER
#define SPEW_INNER_INTERPRETER TRACE
#else
#define SPEW_INNER_INTERPRETER(...)
#endif

#ifdef TRACE_SHELL
#define SPEW_SHELL TRACE
#else
#define SPEW_SHELL(...)
#endif

#ifdef TRACE_VOCABULARY
#define SPEW_VOCABULARY TRACE
#else
#define SPEW_VOCABULARY(...)
#endif


//////////////////////////////////////////////////////////////////////
////
///     built-in forth ops are implemented with static C-style routines
//      which take a pointer to the ForthThread they are being run in
//      the thread is accesed through "g->" in the code

#define FORTHOP(NAME) void NAME( ForthCoreState *pCore )
// GFORTHOP is used for forthops which are defined outside of the dictionary source module
#define GFORTHOP(NAME) void NAME( ForthCoreState *pCore )

#endif
