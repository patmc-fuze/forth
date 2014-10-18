#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthInner.h: inner interpreter state
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

//class ForthEngine;

extern "C" {

// VAR_ACTIONs are subops of a variable op (store/fetch/incStore/decStore)
#define VAR_ACTION(NAME) static void NAME( ForthCoreState *pCore )
typedef void (*VarAction)( ForthCoreState *pCore );

#define OPTYPE_ACTION(NAME) static void NAME( ForthCoreState *pCore, ulong opVal )

// right now there are about 250 builtin ops, allow for future expansion
#define MAX_BUILTIN_OPS 1024


#if 0
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
    
    long                *TPM;      // this pointer (vtable)
    long                *TPD;      // this pointer (data)

    void                *pPrivate; // pointer to per-thread state
    long                currentOp; // last op dispatched by inner interpreter
    
    ulong               varMode;        // operation to perform on variables

    ulong               state;          // inner loop state - ok/done/error

    ulong               error;

    ForthMemorySection* pDictionary;

    void                *pConOutData;
    consoleOutRoutine   consoleOut;
    long                consoleOutOp;

    FILE                *pDefaultOutFile;
    FILE                *pDefaultInFile;
    long                base;      // output base
    ulong               signedPrintMode;   // if numers are printed as signed/unsigned
};
#endif

struct ForthFileInterface
{
    FILE*               (*fileOpen)( const char* pPath, const char* pAccess );
    int                 (*fileClose)( FILE* pFile );
    size_t              (*fileRead)( void* data, size_t itemSize, size_t numItems, FILE* pFile );
    size_t              (*fileWrite)( const void* data, size_t itemSize, size_t numItems, FILE* pFile );
    int                 (*fileGetChar)( FILE* pFile );
    int                 (*filePutChar)( int val, FILE* pFile );
    int                 (*fileAtEnd)( FILE* pFile );
    int                 (*fileExists)( const char* pPath );
    int                 (*fileSeek)( FILE* pFile, long offset, int ctrl );
    long                (*fileTell) ( FILE* pFile );
    int                 (*fileGetLength)( FILE* pFile );
    char*               (*fileGetString)( char* buffer, int bufferLength, FILE* pFile );
    int                 (*filePutString)( const char* buffer, FILE* pFile );
	int					(*fileRemove)( const char* buffer );
	int					(*fileDup)( int fileHandle );
	int					(*fileDup2)( int srcFileHandle, int dstFileHandle );
	int					(*fileNo)( FILE* pFile );
	int					(*fileFlush)( FILE* pFile );
	char*				(*getTmpnam)( char* path );
	int					(*renameFile)( const char* pOldName, const char* pNewName );
	int					(*runSystem)( const char* pCmdline );
	int					(*changeDir)( const char* pPath );
	int					(*makeDir)( const char* pPath, int mode );
	int					(*removeDir)( const char* pPath );
	FILE*				(*getStdIn)();
	FILE*				(*getStdOut)();
	FILE*				(*getStdErr)();
	void*				(*openDir)( const char* pPath );	// returns DIR*, which is pDir in readDir, closeDir, rewindDir
	void*				(*readDir)( void* pDir );			// return is a struct dirent*
	int					(*closeDir)( void* pDir );
	void				(*rewindDir)( void* pDir );
};

struct ForthCoreState
{
    optypeActionRoutine  *optypeAction;

    ulong               numBuiltinOps;

    long                **ops;
    ulong               numOps;
    ulong               maxOps;     // current size of table at pUserOps

    //ForthEngine         *pEngine;
   void                 *pEngine;

    long                *IP;            // interpreter pointer

    long                *SP;            // parameter stack pointer
    
    long                *RP;            // return stack pointer

    long                *FP;            // frame pointer
    
    long                *TPM;           // this pointer (methods)
    long                *TPD;           // this pointer (data)

    ulong               varMode;        // operation to perform on variables

    ulong               state;          // inner loop state - ok/done/error

    ulong               error;

    long                *SB;            // param stack base
    long                *ST;            // empty parameter stack pointer

    ulong               SLen;           // size of param stack in longwords

    long                *RB;            // return stack base
    long                *RT;            // empty return stack pointer

    ulong               RLen;           // size of return stack in longwords

    void                *pThread;

    ForthMemorySection* pDictionary;
    ForthFileInterface* pFileFuncs;

	void				*innerLoop;		// inner loop reentry point for assembler inner interpreter

    void                *pConOutData;
    consoleOutRoutine   consoleOut;
    long                consoleOutOp;

    FILE                *pDefaultOutFile;
    FILE                *pDefaultInFile;
    long                base;               // output base
    ulong               signedPrintMode;   // if numers are printed as signed/unsigned
};


extern eForthResult InnerInterpreter( ForthCoreState *pCore );
extern eForthResult InterpretOneOp( ForthCoreState *pCore, long op );
#ifdef ASM_INNER_INTERPRETER
extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
extern void InitAsmTables( ForthCoreState *pCore );
extern eForthResult InterpretOneOpFast( ForthCoreState *pCore, long op );
#endif

void InitDispatchTables( ForthCoreState* pCore );
void CoreSetError( ForthCoreState *pCore, eForthError error, bool isFatal );

// DLLRoutine is used for any external DLL routine - it can take any number of arguments
typedef long (*DLLRoutine)();
// CallDLLRoutine is an assembler routine which:
// 1) moves arguments from the forth parameter stack to the real stack in reverse order
// 2) calls the DLL routine
// 3) leaves the DLL routine result on the forth parameter stack
extern void CallDLLRoutine( DLLRoutine function, long argCount, unsigned long flags, ForthCoreState *pCore );

inline long GetCurrentOp( ForthCoreState *pCore )
{
    long *pIP = pCore->IP - 1;
    return *pIP;
}


#define GET_IP                          (pCore->IP)
#define SET_IP( A )                     (pCore->IP = (A))

#define GET_SP                          (pCore->SP)
#define SET_SP( A )                     (pCore->SP = (A))

#define GET_RP                          (pCore->RP)
#define SET_RP( A )                     (pCore->RP = (A))

#define GET_FP                          (pCore->FP)
#define SET_FP( A )                     (pCore->FP = (A))

#define GET_TPM                         (pCore->TPM)
#define GET_TPD                         (pCore->TPD)
#define SET_TPM( A )                    (pCore->TPM = (A))
#define SET_TPD( A )                    (pCore->TPD = (A))

#define GET_DP                          (pCore->pDictionary->pCurrent)
#define SET_DP( A )                     (pCore->pDictionary->pCurrent = (A))

#define SPOP                            (*pCore->SP++)
#define SPUSH( A )                      (*--pCore->SP = (A))
#define SDROP                           (pCore->SP++)

#define FPOP                            (*(float *)pCore->SP++)
#define FPUSH( A )                      --pCore->SP; *(float *)pCore->SP = A

#define DPOP                            *((double *)(pCore->SP)); pCore->SP += 2
#define DPUSH( A )                      pCore->SP -= 2; *((double *)(pCore->SP)) = A

#define LPOP                            *((long long *)(pCore->SP)); pCore->SP += 2
#define LPUSH( A )                      pCore->SP -= 2; *((long long *)(pCore->SP)) = A

#define RPOP                            (*pCore->RP++)
#define RPUSH( A )                      (*--pCore->RP = (A))

#define GET_SDEPTH                      (pCore->ST - pCore->SP)
#define GET_RDEPTH                      (pCore->RT - pCore->RP)

#define GET_STATE                       (eForthResult)(pCore->state)
#define SET_STATE( A )                  (pCore->state = (A))

#define GET_ENGINE                      ((ForthEngine *) (pCore->pEngine))
//#define GET_ENGINE                      (ForthEngine::GetInstance())

#define GET_VAR_OPERATION               (pCore->varMode)
#define SET_VAR_OPERATION( A )          (pCore->varMode = (A))
#define CLEAR_VAR_OPERATION             (pCore->varMode = kVarDefaultOp)

#define GET_NUM_OPS		                (pCore->numOps)

#define GET_CURRENT_OP                  GetCurrentOp( pCore )

#define OP_TABLE                        (pCore->ops)

#define SET_ERROR( A )                  CoreSetError( pCore, A, false )
#define SET_FATAL_ERROR( A )            CoreSetError( pCore, A, true )

#define CONSOLE_STRING_OUT( A )         (pCore->consoleOut( pCore, A ))

#define GET_BASE_REF                    (&pCore->base)

#define GET_PRINT_SIGNED_NUM_MODE       (pCore->signedPrintMode)
#define SET_PRINT_SIGNED_NUM_MODE( A )  (pCore->signedPrintMode = (A))

};      // end extern "C"

