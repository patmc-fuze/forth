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

struct ForthCoreState;

#ifndef ulong
#define ulong   unsigned long
#endif

// these are opcode types, they are held in the top byte of an opcode, and in
// a vocabulary entry value field
// NOTE: if you add or reorder op types, make sure that you update ForthEngine::opTypeNames
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
    kOpArrayOffset,     // low 24 bits is array element size
    kOpLocalStructArray,   // bits 0..11 are padded struct size in bytes, bits 12..23 are frame offset in longs

    kOpConstantString,

    kOpAllocLocals,     // low 24 bits is frame size in longs
    kOpInitLocalString,     // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
    kOpLocalRef,

    kOpLocalByte,
    kOpLocalShort,
    kOpLocalInt,
    kOpLocalFloat,
    kOpLocalDouble,
    kOpLocalString,
    kOpLocalOp,

    kOpFieldByte,
    kOpFieldShort,
    kOpFieldInt,
    kOpFieldFloat,
    kOpFieldDouble,
    kOpFieldString,
    kOpFieldOp,

    kOpLocalByteArray,
    kOpLocalShortArray,
    kOpLocalIntArray,
    kOpLocalFloatArray,
    kOpLocalDoubleArray,
    kOpLocalStringArray,
    kOpLocalOpArray,

    kOpFieldByteArray,
    kOpFieldShortArray,
    kOpFieldIntArray,
    kOpFieldFloatArray,
    kOpFieldDoubleArray,
    kOpFieldStringArray,
    kOpFieldOpArray,

    kOpMethodWithThis,  // low 24 bits is method number

    kOpMemberByte,
    kOpMemberShort,
    kOpMemberInt,
    kOpMemberFloat,
    kOpMemberDouble,
    kOpMemberString,
    kOpMemberOp,

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
typedef bool (*interpreterExtensionRoutine)( char *pToken );

// consoleOutRoutine is used to pass all console output
typedef void (*consoleOutRoutine) ( ForthCoreState *pCore, const char *pBuff );

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
    kForthErrorStruct,
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
#define OP_ABORT                BUILTIN_OP(0)
#define OP_DROP                 BUILTIN_OP(1)
#define OP_DO_DOES              BUILTIN_OP(2)
#define OP_INT_VAL              BUILTIN_OP(3)
#define OP_FLOAT_VAL            BUILTIN_OP(4)
#define OP_DOUBLE_VAL           BUILTIN_OP(5)
#define OP_DO_VAR               BUILTIN_OP(6)
#define OP_DO_CONSTANT          BUILTIN_OP(7)
#define OP_DO_DCONSTANT         BUILTIN_OP(8)
#define OP_END_BUILDS           BUILTIN_OP(9)
#define OP_DONE                 BUILTIN_OP(10)
#define OP_DO_BYTE              BUILTIN_OP(11)
#define OP_DO_SHORT             BUILTIN_OP(12)
#define OP_DO_INT               BUILTIN_OP(13)
#define OP_DO_FLOAT             BUILTIN_OP(14)
#define OP_DO_DOUBLE            BUILTIN_OP(15)
#define OP_DO_STRING            BUILTIN_OP(16)
#define OP_DO_OP                BUILTIN_OP(17)
#define OP_INTO                 BUILTIN_OP(18)
#define OP_DO_DO                BUILTIN_OP(19)
#define OP_DO_LOOP              BUILTIN_OP(20)
#define OP_DO_LOOPN             BUILTIN_OP(21)
#define OP_DO_EXIT              BUILTIN_OP(22)
#define OP_DO_EXIT_L            BUILTIN_OP(23)
#define OP_DO_EXIT_M            BUILTIN_OP(24)
#define OP_DO_EXIT_ML           BUILTIN_OP(25)
#define OP_DO_VOCAB             BUILTIN_OP(26)
#define OP_DO_BYTE_ARRAY        BUILTIN_OP(27)
#define OP_DO_SHORT_ARRAY       BUILTIN_OP(28)
#define OP_DO_INT_ARRAY         BUILTIN_OP(29)
#define OP_DO_FLOAT_ARRAY       BUILTIN_OP(30)
#define OP_DO_DOUBLE_ARRAY      BUILTIN_OP(31)
#define OP_DO_STRING_ARRAY      BUILTIN_OP(32)
#define OP_DO_OP_ARRAY          BUILTIN_OP(33)
#define OP_INIT_STRING          BUILTIN_OP(34)
#define OP_INIT_STRING_ARRAY    BUILTIN_OP(35)
#define OP_PLUS                 BUILTIN_OP(36)
#define OP_FETCH                BUILTIN_OP(37)
#define OP_BAD_OP               BUILTIN_OP(38)
#define OP_DO_STRUCT            BUILTIN_OP(39)
#define OP_DO_STRUCT_ARRAY      BUILTIN_OP(40)
#define OP_DO_STRUCT_TYPE       BUILTIN_OP(41)
#define OP_DO_ENUM              BUILTIN_OP(42)

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
#define TRACE_STRUCTS

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

#ifdef TRACE_STRUCTS
#define SPEW_STRUCTS TRACE
#else
#define SPEW_STRUCTS(...)
#endif


// user-defined ops vocab entries have the following value fields:
// - opcode
// - struct type
#define NUM_FORTH_VOCAB_VALUE_LONGS 2

// a locals vocab entry has the following value fields:
// - opcode (which contains frame offset)
// - struct type
#define NUM_LOCALS_VOCAB_VALUE_LONGS 2

// a struct vocab entry has the following value fields:
// - field offset in bytes
// - field type
// - element count (valid only for array fields)
#define NUM_STRUCT_VOCAB_VALUE_LONGS 3

//////////////////////////////////////////////////////////////////////
////
///     built-in forth ops are implemented with static C-style routines
//      which take a pointer to the ForthThread they are being run in
//      the thread is accesed through "g->" in the code

#define FORTHOP(NAME) void NAME( ForthCoreState *pCore )
// GFORTHOP is used for forthops which are defined outside of the dictionary source module
#define GFORTHOP(NAME) void NAME( ForthCoreState *pCore )

//////////////////////////////////////////////////////////////////////
////
///     user-defined structure support
//      
//      

// forth native data types
// NOTE: the order of these have to match the order of forthOpType definitions above which
//  are related to native types (kOpLocalByte, kOpMemberFloat, kOpLocalIntArray, ...)
//  as well as the order of actual opcodes used to implement native types (OP_DO_INT, OP_DO_FLOAT, OP_DO_INT_ARRAY, ...)
typedef enum
{
    kNativeByte,
    kNativeShort,
    kNativeInt,
    kNativeFloat,
    kNativeDouble,
    kNativeString,
    kNativeOp,
} forthNativeType;

typedef enum
{
    // kDTNone, kDTSingle and kDTArray are mutually exclusive
    kDTNone         = 0,
    kDTSingle       = 1,
    kDTArray        = 2,
    kDTIllegal      = 3,
    // kDTIsPtr and kDTIsNative can be combined with anything
    kDTIsPtr        = 4,
    kDTIsNative     = 8,
} storageDescriptor;

// user-defined structure fields have a 32-bit descriptor with the following format:
// 1...0        select none, single or array
//   2          is field a pointer
//   3          is field native

// for types with kDTIsNative set:
// 7...4        forthNativeType
// 31...8       string length (if forthNativeType == kNativeString)

// for types with kDTIsNative clear:
// 31...4     struct index

// when kDTArray and kDTIsPtr are both set, it means the field is an array of pointers
#define NATIVE_TYPE_TO_CODE( STORAGE_TYPE, NATIVE_TYPE )    (kDTIsNative | ((NATIVE_TYPE << 4) | STORAGE_TYPE))
#define STRING_TYPE_TO_CODE( STORAGE_TYPE, MAX_BYTES )      (kDTIsNative | ((MAX_BYTES << 8) | (kNativeString << 4) | STORAGE_TYPE))
#define STRUCT_TYPE_TO_CODE( STORAGE_TYPE, STRUCT_INDEX )    ((STRUCT_INDEX << 4) | STORAGE_TYPE)
#define CODE_IS_DATA( CODE )                (((CODE) & 3) != 0)
#define CODE_IS_VARIABLE( CODE )            (((CODE) & 3) == kDTSingle)
#define CODE_IS_ARRAY( CODE )               (((CODE) & 3) == kDTArray)
#define CODE_IS_PTR( CODE )                 (((CODE) & kDTIsPtr) != 0)
#define CODE_IS_NATIVE( CODE )              (((CODE) & kDTIsNative) != 0)
#define CODE_TO_STORAGE_TYPE( CODE )        ((CODE) & 0x0F)
#define CODE_TO_NATIVE_TYPE( CODE )         (((CODE) >> 4) & 0x0F)
#define CODE_TO_STRUCT_INDEX( CODE )        (((CODE) >> 4) & 0x0FFFFFFF)
#define CODE_TO_STRING_BYTES( CODE )        ((CODE) >> 8)

#endif
