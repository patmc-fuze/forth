//
// Forth engine definitions
//   Pat McElhatton   September '00
//

#ifndef __FORTH_H
#define __FORTH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//#include "stdafx.h"

#ifndef ulong
#define ulong unsigned long
#endif

class ForthEngine;
class ForthShell;
class ForthThread;
class ForthVocabulary;

// these are symbol types, they are held in the top byte of a symbols value field
// the highest bit of the symbol type field is reserved for the precedence bit
// which if set makes operations execute even when in compile mode
typedef enum {
   kOpBuiltIn = 0,
   kOpUserDef,      // user operation defined with colon

   kOpBranch,
   kOpBranchNZ,
   kOpBranchZ,

   kOpConstant,
   kOpString,

   kOpAllocLocals,
   kOpLocalInt,
   kOpLocalFloat,
   kOpLocalDouble,
   kOpLocalString,
   kOpLocalUserDefined,

   kOpBuiltInPrec    = 128,
   kOpUserDefPrec    = 129,      // user operation defined with colon

} forthOpType;


// the varMode state makes variables do something other
//  than their default behaviour (fetch)
typedef enum {
    kVarFetch = 0,
    kVarRef,
    kVarStore,
    kVarPlusStore,
    kVarMinusStore
} varOperation;

typedef struct _ForthInputStream {
    struct _ForthInputStream    *pNext;
    char    *pBuffer;
    int     bufferLen;
    int     bufferOffset;
    FILE    *pInFile;
} ForthInputStream;

#define DEFAULT_INPUT_BUFFER_LEN   256

// these are the results of running the inner interpreter
typedef enum {
    kResultOk,          // no need to exit
    kResultDone,        // exit because of "done" opcode
    kResultExitShell,   // exit because of a "bye" opcode
    kResultError,       // exit because of error
    kResultFatalError,  // exit because of fatal error
} eForthResult;

#define FLAG_DONE           1
#define FLAG_BYE            (1 << 1)
#define FLAG_ERROR          (1 << 2)
#define FLAG_FATAL_ERROR    (1 << 3)

typedef enum {
    kForthErrorNone,
    kForthErrorBadOpcode,
    kForthErrorParamStackUnderflow,
    kForthErrorParamStackOverflow,
    kForthErrorReturnStackUnderflow,
    kForthErrorReturnStackOverflow,
    kForthErrorUnknownSymbol,
    kForthErrorFileOpen,
    kForthErrorAbort
} eForthError;

// how sign should be handled while printing integers
typedef enum {
    kPrintSignedDecimal,
    kPrintAllSigned,
    kPrintAllUnsigned
} ePrintSignedMode;


#define SET_ERROR( F )  SET_FLAG( (F) | FLAG_DONE | FLAG_ERROR )
#define SET_DONE( F )   SET_FLAG( (F) | FLAG_DONE )
#define CLR_FLAG( F )   g->flags &= ~(F)
#define SET_FLAG( F )   g->flags |= (F)
#define CHK_FLAG( F )   (g->flags & (F))


// the bottom 24 bits of a forth opcode is a value field
#define OPCODE_VALUE_MASK   0xFFFFFF

#define NEEDS(A)
#define RNEEDS(A)

#define POP       (*(g->sp)++)
#define FPOP      (*(float *)(g->sp)++)
#define DPOP      (*(double *)(g->sp)++)
#define PUSH(A)   *--(g->sp) = (long) (A)
#define FPUSH(A)  *(float *)--(g->sp) = (A)
#define DPUSH(A)  *(double *)--(g->sp) = (A)
#define RPOP      (*(g->rp)++)
#define RPUSH(A)  *--(g->rp) = (ulong) (A)

class ForthThread;

#define FORTHOP(NAME) static void NAME(ForthThread *g)

typedef void  (*ForthOp)(ForthThread *);

#define COMPILED_OP( OP_TYPE, VALUE ) ((OP_TYPE << 24) | (VALUE & OPCODE_VALUE_MASK))
#define BUILTIN_OP( INDEX )   COMPILED_OP( kOpBuiltIn, INDEX )
// These are opcodes that built-in ops must compile directly
// NOTE: the index field of these opcodes must agree with the
//  order of builtin dictionary entries at the end of this file
#define OP_ABORT        BUILTIN_OP(0)
#define OP_UNRAVEL      BUILTIN_OP(1)
#define OP_DO_DOES      BUILTIN_OP(2)
#define OP_EXIT         BUILTIN_OP(3)
#define OP_INT_VAL      BUILTIN_OP(4)
#define OP_FLOAT_VAL    BUILTIN_OP(5)
#define OP_DOUBLE_VAL   BUILTIN_OP(6)
#define OP_DO_VAR       BUILTIN_OP(7)
#define OP_DO_CONSTANT  BUILTIN_OP(8)
#define OP_END_BUILDS   BUILTIN_OP(9)
#define OP_DONE         BUILTIN_OP(10)
#define OP_DO_INT       BUILTIN_OP(11)
#define OP_DO_FLOAT     BUILTIN_OP(12)
#define OP_DO_DOUBLE    BUILTIN_OP(13)
#define OP_DO_STRING    BUILTIN_OP(14)
#define OP_INTO         BUILTIN_OP(15)

typedef struct {
   char             *name;
   forthOpType      flags;       // nameLen is 0:7,
   ulong            value;
} baseDictEntry;

// These are the flags that can be passed to InterpretForthToken
#define PARSE_FLAG_QUOTED_STRING        1
#define PARSE_FLAG_QUOTED_CHARACTER     2



#endif
