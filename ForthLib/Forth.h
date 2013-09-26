#pragma once
//////////////////////////////////////////////////////////////////////
//
// Forth engine definitions
//   Pat McElhatton   September '00
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"

struct ForthCoreState;

#ifndef ulong
#define ulong   unsigned long
#endif

#define MAX_STRING_SIZE (8 * 1024)

// these are opcode types, they are held in the top byte of an opcode, and in
// a vocabulary entry value field
// NOTE: if you add or reorder op types, make sure that you update ForthEngine::opTypeNames
typedef enum
{
    kOpBuiltIn = 0,
    kOpBuiltInImmediate,
    kOpUserDef,         // low 24 bits is op number (index into ForthCoreState userOps table)
    kOpUserDefImmediate,
    kOpUserCode,         // low 24 bits is op number (index into ForthCoreState userOps table)
    kOpUserCodeImmediate,
    kOpDLLEntryPoint,   // bits 0:18 are index into ForthCoreState userOps table, 19:23 are arg count
    // 7 - 9 are unused

    kOpBranch = 10,          // low 24 bits is signed branch offset
    kOpBranchNZ,
    kOpBranchZ,
    kOpCaseBranch,
    // 14 - 19 are unused

    kOpConstant = 20,   // low 24 bits is signed symbol value
    kOpConstantString,  // low 24 bits is number of longwords to skip over
    kOpOffset,          // low 24 bits is signed offset value, TOS is number to add it to
    kOpArrayOffset,     // low 24 bits is array element size, TOS is array base, NTOS is index
    kOpAllocLocals,     // low 24 bits is frame size in longs
    kOpLocalRef,        // low 24 bits is offset in bytes
    kOpLocalStringInit,     // bits 0:11 are string length in bytes, bits 12:23 are frame offset in longs
    kOpLocalStructArray,   // bits 0:11 are padded struct size in bytes, bits 12:23 are frame offset in longs
    kOpOffsetFetch,          // low 24 bits is signed offset in longs, TOS is long ptr
    kOpMemberRef,		// low 24 bits is offset in bytes

    kOpLocalByte = 30,	// low 24 bits is offset in bytes
    kOpLocalUByte,
    kOpLocalShort,
    kOpLocalUShort,
    kOpLocalInt,
    kOpLocalUInt,
    kOpLocalLong,
    kOpLocalULong,
    kOpLocalFloat,
    kOpLocalDouble,
    kOpLocalString,
    kOpLocalOp,
    kOpLocalObject,

    kOpLocalByteArray = 43,	// low 24 bits is offset in bytes, TOS is index
    kOpLocalUByteArray,
    kOpLocalShortArray,
    kOpLocalUShortArray,
    kOpLocalIntArray,
    kOpLocalUIntArray,
    kOpLocalLongArray,
    kOpLocalULongArray,
    kOpLocalFloatArray,
    kOpLocalDoubleArray,
    kOpLocalStringArray,
    kOpLocalOpArray,
    kOpLocalObjectArray,

    kOpFieldByte = 56,
    kOpFieldUByte,
    kOpFieldShort,
    kOpFieldUShort,
    kOpFieldInt,
    kOpFieldUInt,
    kOpFieldLong,
    kOpFieldULong,
    kOpFieldFloat,
    kOpFieldDouble,
    kOpFieldString,
    kOpFieldOp,
    kOpFieldObject,

    kOpFieldByteArray = 69,
    kOpFieldUByteArray,
    kOpFieldShortArray,
    kOpFieldUShortArray,
    kOpFieldIntArray,
    kOpFieldUIntArray,
    kOpFieldLongArray,
    kOpFieldULongArray,
    kOpFieldFloatArray,
    kOpFieldDoubleArray,
    kOpFieldStringArray,
    kOpFieldOpArray,
    kOpFieldObjectArray,

    kOpMemberByte = 82,
    kOpMemberUByte,
    kOpMemberShort,
    kOpMemberUShort,
    kOpMemberInt,
    kOpMemberUInt,
    kOpMemberLong,
    kOpMemberULong,
    kOpMemberFloat,
    kOpMemberDouble,
    kOpMemberString,
    kOpMemberOp,
    kOpMemberObject,

    kOpMemberByteArray = 95,
    kOpMemberUByteArray,
    kOpMemberShortArray,
    kOpMemberUShortArray,
    kOpMemberIntArray,
    kOpMemberUIntArray,
    kOpMemberLongArray,
    kOpMemberULongArray,
    kOpMemberFloatArray,
    kOpMemberDoubleArray,
    kOpMemberStringArray,
    kOpMemberOpArray,
    kOpMemberObjectArray,

    kOpMethodWithThis = 108,                 // low 24 bits is method number
    kOpMethodWithTOS,                       // low 24 bits is method number
    kOpMemberStringInit,                    // bits 0:11 are string length in bytes, bits 12:23 are memeber offset in longs
	kOpNVOCombo,							// NUM VAROP OP combo - bits 0:10 are signed integer, bits 11:12 are varop-2, bit 13 is builtin/userdef, bits 14-23 are opcode
	kOpNVCombo,								// NUM VAROP combo - bits 0:21 are signed integer, bits 22:23 are varop-2
	kOpNOCombo,								// NUM OP combo - bits 0:12 are signed integer, bit 13 is builtin/userdef, bits 14:23 are opcode
	kOpVOCombo,								// VAROP OP combo - bits 0:1 are varop-2, bit 2 is builtin/userdef, bits 3:23 are opcode
	
    // 115 is unused
    kOpLocalUserDefined = 116,             // user can add more optypes starting with this one
    kOpMaxLocalUserDefined = 127,    // maximum user defined optype

    kOpUserMethods  = 128
    // optypes from 128:.255 are used to select class methods    
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

// traceOutRoutine is used when overriding builtin trace routines
typedef void (*traceOutRoutine) ( void *pData, const char *pBuff );

// the varMode state makes variables do something other
//  than their default behaviour (fetch)
typedef enum {
    kVarDefaultOp = 0,
    kVarFetch,
    kVarRef,
    kVarStore,
    kVarPlusStore,
    kVarMinusStore,

} varOperation;

typedef enum {
    kVocabSetCurrent = 0,
    kVocabNewestEntry,
	kVocabRef,				// must be same index as kVarRef
    kVocabFindEntry,
    kVocabFindEntryValue,
    kVocabAddEntry,
    kVocabRemoveEntry,
    kVocabEntryLength,
    kVocabNumEntries,
    kVocabGetClass,
} vocabOperation;

#define DEFAULT_INPUT_BUFFER_LEN   (16 * 1024)

// these are the results of running the inner interpreter
typedef enum {
    kResultOk,          // no need to exit
    kResultDone,        // exit because of "done" opcode
    kResultExitShell,   // exit because of a "bye" opcode
    kResultError,       // exit because of error
    kResultFatalError,  // exit because of fatal error
    kResultException,   // exit because of uncaught exception
    kResultShutdown,    // exit because of a "shutdown" opcode
} eForthResult;

// Nothing uses these
/*
#define FLAG_DONE           1
#define FLAG_BYE            (1 << 1)
#define FLAG_ERROR          (1 << 2)
#define FLAG_FATAL_ERROR    (1 << 3)
*/

typedef enum {
    kForthErrorNone,
    kForthErrorBadOpcode,
    kForthErrorBadOpcodeType,
    kForthErrorBadParameter,
    kForthErrorBadVarOperation,
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
    kForthErrorUserDefined,
    kForthErrorBadSyntax,
    kForthErrorBadPreprocessorDirective,
    kForthErrorUnimplementedMethod,
    // NOTE: if you add errors, make sure that you update ForthEngine::GetErrorString
    kForthNumErrors
} eForthError;

// how sign should be handled while printing integers
typedef enum {
    kPrintSignedDecimal,
    kPrintAllSigned,
    kPrintAllUnsigned
} ePrintSignedMode;


typedef struct {
    // user dictionary stuff
    long*               pCurrent;
    long*               pBase;
    ulong               len;
} ForthMemorySection;

// this is what is placed on the stack to represent a forth object
//  usually the 'data' field is actually a pointer to the data, but that is an
//  implementation detail and not true for all classes
typedef struct
{
    long*       pMethodOps;
    long*       pData;      // actually this isn't always a pointer
} ForthObject;


// the bottom 24 bits of a forth opcode is a value field
// the top 8 bits is the type field
#define OPCODE_VALUE_MASK   0xFFFFFF
#define FORTH_OP_TYPE( OP )  ( (forthOpType) (((OP) >> 24) & 0xFF) )
#define FORTH_OP_VALUE( OP ) ( (OP) & OPCODE_VALUE_MASK )

#define DISPATCH_FORTH_OP( _pCore, _op ) 	_pCore->optypeAction[ (int) FORTH_OP_TYPE( _op ) ]( _pCore, FORTH_OP_VALUE( _op ) )


#define NEEDS(A)
#define RNEEDS(A)

class ForthThread;

#define COMPILED_OP( OP_TYPE, VALUE ) (((OP_TYPE) << 24) | ((VALUE) & OPCODE_VALUE_MASK))
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
#define OP_DO_UBYTE             BUILTIN_OP(12)
#define OP_DO_SHORT             BUILTIN_OP(13)
#define OP_DO_USHORT            BUILTIN_OP(14)
#define OP_DO_INT               BUILTIN_OP(15)
#define OP_DO_UINT              BUILTIN_OP(16)
#define OP_DO_LONG              BUILTIN_OP(17)
#define OP_DO_ULONG             BUILTIN_OP(18)
#define OP_DO_FLOAT             BUILTIN_OP(19)
#define OP_DO_DOUBLE            BUILTIN_OP(20)
#define OP_DO_STRING            BUILTIN_OP(21)
#define OP_DO_OP                BUILTIN_OP(22)
#define OP_DO_OBJECT            BUILTIN_OP(23)
#define OP_DO_EXIT              BUILTIN_OP(24)
#define OP_DO_EXIT_L            BUILTIN_OP(25)
#define OP_DO_EXIT_M            BUILTIN_OP(26)
#define OP_DO_EXIT_ML           BUILTIN_OP(27)
#define OP_DO_VOCAB             BUILTIN_OP(28)
#define OP_DO_BYTE_ARRAY        BUILTIN_OP(29)
#define OP_DO_UBYTE_ARRAY       BUILTIN_OP(30)
#define OP_DO_SHORT_ARRAY       BUILTIN_OP(31)
#define OP_DO_USHORT_ARRAY      BUILTIN_OP(32)
#define OP_DO_INT_ARRAY         BUILTIN_OP(33)
#define OP_DO_UINT_ARRAY        BUILTIN_OP(34)
#define OP_DO_LONG_ARRAY        BUILTIN_OP(35)
#define OP_DO_ULONG_ARRAY       BUILTIN_OP(36)
#define OP_DO_FLOAT_ARRAY       BUILTIN_OP(37)
#define OP_DO_DOUBLE_ARRAY      BUILTIN_OP(38)
#define OP_DO_STRING_ARRAY      BUILTIN_OP(39)
#define OP_DO_OP_ARRAY          BUILTIN_OP(40)
#define OP_DO_OBJECT_ARRAY      BUILTIN_OP(41)
#define OP_INIT_STRING          BUILTIN_OP(42)
#define OP_INIT_STRING_ARRAY    BUILTIN_OP(43)
#define OP_PLUS                 BUILTIN_OP(44)
#define OP_FETCH                BUILTIN_OP(45)
#define OP_BAD_OP               BUILTIN_OP(46)
#define OP_DO_STRUCT            BUILTIN_OP(47)
#define OP_DO_STRUCT_ARRAY      BUILTIN_OP(48)
#define OP_DO_STRUCT_TYPE       BUILTIN_OP(49)
#define OP_DO_CLASS_TYPE        BUILTIN_OP(50)
#define OP_DO_ENUM              BUILTIN_OP(51)
#define OP_DO_DO                BUILTIN_OP(52)
#define OP_DO_LOOP              BUILTIN_OP(53)
#define OP_DO_LOOPN             BUILTIN_OP(54)
#define OP_DO_NEW               BUILTIN_OP(55)
#define OP_DFETCH               BUILTIN_OP(56)
#define OP_ALLOC_OBJECT         BUILTIN_OP(57)
#define OP_VOCAB_TO_CLASS       BUILTIN_OP(58)
#define OP_ADDRESS_OF           BUILTIN_OP(59)
#define OP_INTO                 BUILTIN_OP(60)
#define OP_INTO_PLUS            BUILTIN_OP(61)
#define OP_INTO_MINUS           BUILTIN_OP(62)
#define OP_SUPER				BUILTIN_OP(63)
#define OP_DO_CHECKDO           BUILTIN_OP(64)

#define BASE_DICT_PRECEDENCE_FLAG 0x100
typedef struct
{
   const char       *name;
   ulong            flags;
   ulong            value;
} baseDictionaryEntry;

// helper macro for built-in op entries in baseDictionary
#define OP_DEF( func, funcName )  { funcName, kOpBuiltIn, (ulong) func }

// helper macro for ops which have precedence (execute at compile time)
#define PRECOP_DEF( func, funcName )  { funcName, kOpBuiltInImmediate, (ulong) func }


typedef struct
{
    const char      *name;
    ulong           value;
    ulong           returnType;
} baseMethodEntry;


// trace output flags
#ifdef INCLUDE_TRACE
//#define TRACE_PRINTS
#define TRACE_OUTER_INTERPRETER
#define TRACE_INNER_INTERPRETER
#define TRACE_SHELL
#define TRACE_VOCABULARY
#define TRACE_STRUCTS
#endif

#ifdef TRACE_PRINTS
#define SPEW_PRINTS TRACE
#else
#define SPEW_PRINTS TRACE
//#define SPEW_PRINTS(...)
#endif

#ifdef TRACE_OUTER_INTERPRETER
#define SPEW_OUTER_INTERPRETER TRACE
#else
#define SPEW_OUTER_INTERPRETER TRACE
//#define SPEW_OUTER_INTERPRETER(...)
#endif

#ifdef TRACE_INNER_INTERPRETER
#define SPEW_INNER_INTERPRETER TRACE
#else
#define SPEW_INNER_INTERPRETER TRACE
//#define SPEW_INNER_INTERPRETER(...)
#endif

#ifdef TRACE_SHELL
#define SPEW_SHELL TRACE
#else
#define SPEW_SHELL TRACE
//#define SPEW_SHELL(...)
#endif

#ifdef TRACE_VOCABULARY
#define SPEW_VOCABULARY TRACE
#else
#define SPEW_VOCABULARY TRACE
//#define SPEW_VOCABULARY(...)
#endif

#ifdef TRACE_STRUCTS
#define SPEW_STRUCTS TRACE
#else
#define SPEW_STRUCTS TRACE
//#define SPEW_STRUCTS(...)
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
//  as well as the order of actual opcodes used to implement native types (, OP_DO_FLOAT, OP_DO_INT_ARRAY, ...)
typedef enum
{
    kBaseTypeByte,          // 0 - byte
    kBaseTypeUByte,         // 1 - ubyte
    kBaseTypeShort,         // 2 - short
    kBaseTypeUShort,        // 3 - ushort
    kBaseTypeInt,           // 4 - int
    kBaseTypeUInt,          // 5 - uint
    kBaseTypeLong,          // 6 - long
    kBaseTypeULong,         // 7 - ulong
    kBaseTypeFloat,         // 8 - float
    kBaseTypeDouble,        // 9 - double
    kBaseTypeString,        // 10 - string
    kBaseTypeOp,            // 11 - op
    kNumNativeTypes,
    kBaseTypeObject = kNumNativeTypes,      // 12 - object
    kBaseTypeStruct,                        // 13 - struct
    kBaseTypeUserDefinition,                // 14 - user defined forthop
    kBaseTypeVoid,							// 15 - void
    kNumBaseTypes,
    kBaseTypeUnknown = kNumBaseTypes,
} forthBaseType;

typedef enum
{
    // kDTIsPtr and kDTIsNative can be combined with anything
    kDTIsPtr        = 16,
    kDTIsArray      = 32,
    kDTIsMethod     = 64,
} storageDescriptor;

// user-defined structure fields have a 32-bit descriptor with the following format:
// 3...0        base type
//   4          is field a pointer
//   5          is field an array
//   6          is this a method
//   7          unused
// 31...8       depends on base type:
//      string      length
//      struct      structIndex
//      object      classId

// when kDTArray and kDTIsPtr are both set, it means the field is an array of pointers
#define NATIVE_TYPE_TO_CODE( ARRAY_FLAG, NATIVE_TYPE )      ((ARRAY_FLAG) | (NATIVE_TYPE))
#define STRING_TYPE_TO_CODE( ARRAY_FLAG, MAX_BYTES )        ((ARRAY_FLAG) | kBaseTypeString | ((MAX_BYTES) << 8))
#define STRUCT_TYPE_TO_CODE( ARRAY_FLAG, STRUCT_INDEX )     ((ARRAY_FLAG) | kBaseTypeStruct | ((STRUCT_INDEX) << 8))
#define OBJECT_TYPE_TO_CODE( ARRAY_FLAG, STRUCT_INDEX )     ((ARRAY_FLAG) | kBaseTypeObject | ((STRUCT_INDEX) << 8))

#define BASE_TYPE_TO_CODE( BASE_TYPE )                      (BASE_TYPE)

#define CODE_IS_SIMPLE( CODE )                              (((CODE) & (kDTIsArray | kDTIsPtr)) == 0)
#define CODE_IS_ARRAY( CODE )                               (((CODE) & kDTIsArray) != 0)
#define CODE_IS_PTR( CODE )                                 (((CODE) & kDTIsPtr) != 0)
#define CODE_IS_NATIVE( CODE )                              (((CODE) & 0xF) < kNumNativeTypes)
#define CODE_IS_METHOD( CODE )                              (((CODE) & kDTIsMethod) != 0)
#define CODE_TO_BASE_TYPE( CODE )                           ((CODE) & 0x0F)
#define CODE_TO_STRUCT_INDEX( CODE )                        ((CODE) >> 8)
#define CODE_TO_STRING_BYTES( CODE )                        ((CODE) >> 8)
#define CODE_IS_USER_DEFINITION( CODE )                     (((CODE) & 0x0F) == kBaseTypeUserDefinition)

// bit fields for kOpDLLEntryPoint
#define DLL_ENTRY_TO_CODE( INDEX, NUM_ARGS, FLAGS )    (((NUM_ARGS) << 19) | FLAGS | INDEX )
#define CODE_TO_DLL_ENTRY_INDEX( VAL )          (VAL & 0x0000FFFF)
#define CODE_TO_DLL_ENTRY_NUM_ARGS( VAL)        (((VAL) & 0x00F80000) >> 19)
#define CODE_TO_DLL_ENTRY_FLAGS( VAL)        (((VAL) & 0x00070000) >> 16)
#define DLL_ENTRY_FLAG_RETURN_VOID		0x10000
#define DLL_ENTRY_FLAG_RETURN_64BIT		0x20000
#define DLL_ENTRY_FLAG_STDCALL			0x40000