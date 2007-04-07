//////////////////////////////////////////////////////////////////////
//
// ForthEngine.h: interface for the ForthEngine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_FORTH_ENGINE_H_INCLUDED_)
#define _FORTH_ENGINE_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"
#include "ForthThread.h"
#include "ForthShell.h"
#include "ForthInner.h"
#include "ForthVocabulary.h"
#include "ForthStructs.h"

#define NEW_INNER_INTERP


class ForthThread;
class ForthShell;

#define DEFAULT_USER_STORAGE 16384

// this is the size of the buffer returned by GetTmpStringBuffer()
//  which is the buffer used by word and blword
#define TMP_STRING_BUFFER_LEN 256

typedef enum {
    kFECompileFlagHasLocalVars              = 0x01,
    kFECompileFlagInStructDefinition        = 0x02,
    kFECompileFlagIsPointer                 = 0x04,
    kFECompileFlagIsMethod                  = 0x08,
    kFECompileFlagInClassDefinition         = 0x10,
} FECompileFlags;

class ForthEngine
{
public:
                    ForthEngine();
    virtual         ~ForthEngine();

    void            Initialize( int storageLongs=DEFAULT_USER_STORAGE,
                                bool bAddBaseOps=true,
                                baseDictEntry *pUserBuiltinOps=NULL );
    void            Reset( void );
    void            ErrorReset( void );

    void            SetFastMode( bool goFast );
    void            ToggleFastMode( void );

    //
    // ExecuteOneOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
    // execute forth ops, and is also how systems external to forth execute ops
    //
    eForthResult        ExecuteOneOp( long opCode );

    // add an op to the operator dispatch table. returns the assigned opcode (without type field)
    long            AddOp( const long *pOp, forthOpType symType );
    long            AddUserOp( const char *pSymbol, bool smudgeIt=false );
    void            AddBuiltinOps( baseDictEntry *pEntries );

    // forget the specified op and all higher numbered ops, and free the memory where those ops were stored
    void            ForgetOp( ulong opNumber );
    // forget the named symbol - return false if symbol not found
    bool            ForgetSymbol( const char *pSym );

    // create a thread which will be managed by the engine - the engine destructor will delete all threads
    //  which were created with CreateThread 
    ForthThread *   CreateThread( int paramStackSize = DEFAULT_PSTACK_SIZE, int returnStackSize = DEFAULT_RSTACK_SIZE, long *pInitialIP = NULL );
    void            DestroyThread( ForthThread *pThread );

    // return true IFF the last compiled opcode was an integer literal
    bool            GetLastConstant( long& constantValue );

    // add a user-defined extension to the outer interpreter
    inline void     SetInterpreterExtension( interpreterExtensionRoutine *pRoutine )
    {
        mpInterpreterExtension = pRoutine;
    };

    // add a user-defined forthop type
    // opType should be in range kOpLocalUserDefined ... kOpMaxLocalUserDefined
    // return false IFF opType is out of range
    bool            AddOpType( forthOpType opType, optypeActionRoutine opAction );

    char *          GetNextSimpleToken( void );

    void            PushInputFile( FILE *pInFile );
    void            PushInputBuffer( char *pDataBuffer, int dataBufferLen );
    void            PopInputStream( void );

    // returns pointer to new vocabulary entry
    long *          StartOpDefinition( const char *pName=NULL, bool smudgeIt=false );
    void            EndOpDefinition( bool unsmudgeIt=false );
    // return pointer to symbol entry, NULL if not found
    long *          FindSymbol( const char *pSymName );
    void            StartStructDefinition( void );
    void            EndStructDefinition( void );
    // returns size of local stack frame in bytes after adding local var
    long            AddLocalVar( const char *pName, forthNativeType varType, long varSize );
    long            AddLocalArray( const char *pName, forthNativeType elementType, long varSize );

    eForthResult    ProcessToken( ForthParseInfo *pInfo );
    char *          GetLastInputToken( void );

    void                    TraceOp( void );

    inline long *           GetDP() { return mpCore->DP; };
    inline void             SetDP( long *pNewDP ) { mpCore->DP = pNewDP; };
    inline void             CompileLong( long v ) { *mpCore->DP++ = v; };
    inline void             CompileDouble( double v ) { *((double *) mpCore->DP) = v; mpCore->DP += 2; };
    void                    CompileOpcode( long v );
    void                    UncompileLastOpcode( void );
    inline void             AllotLongs( int n ) { mpCore->DP += n; };
    inline void             AlignDP( void ) { mpCore->DP = (long *)(( ((int)mpCore->DP) + 3 ) & ~3); };
    inline ForthVocabulary  *GetSearchVocabulary( void )   { return mpSearchVocab; };
    inline void             SetSearchVocabulary( ForthVocabulary* pVocab )  { mpSearchVocab = pVocab; };
    inline ForthVocabulary  *GetPrecedenceVocabulary( void )   { return mpPrecedenceVocab; };
    inline ForthVocabulary  *GetDefinitionVocabulary( void )   { return mpDefinitionVocab; };
    inline void             SetDefinitionVocabulary( ForthVocabulary* pVocab )  { mpDefinitionVocab = pVocab; };
    inline ForthVocabulary  *GetLocalVocabulary( void )   { return mpLocalVocab; };
    inline void             SetShell( ForthShell *pShell ) { mpShell = pShell; };
    inline ForthShell       *GetShell( void ) { return mpShell; };
    inline ForthVocabulary  *GetForthVocabulary( void )   { return mpMainVocab; };
    inline ForthThread      *GetCurrentThread( void )  { return mpCurrentThread; };

    inline long             *GetCompileStatePtr( void ) { return &mCompileState; };
    inline void             SetCompileState( long v ) { mCompileState = v; };
    inline long             IsCompiling( void ) { return mCompileState; };
    inline bool             InStructDefinition( void ) { return ((mCompileFlags & kFECompileFlagInStructDefinition) != 0); };
    inline bool             HasLocalVars( void ) { return ((mCompileFlags & kFECompileFlagHasLocalVars) != 0); };
    inline long             GetCompileFlags( void ) { return mCompileFlags; };
    inline void             SetCompileFlags( long flags ) { mCompileFlags = flags; };
    inline void             SetCompileFlag( long flags ) { mCompileFlags |= flags; };
    inline void             ClearCompileFlag( long flags ) { mCompileFlags &= (~flags); };
    inline long *           GetLastCompiledOpcodePtr( void ) { return mpLastCompiledOpcode; };
    inline char *           GetTmpStringBuffer( void ) { return mpStringBufferB; };
    void                    SetCurrentThread( ForthThread* pThread );
    inline void             SetArraySize( long numElements )        { mNumElements = numElements; };
    inline long             GetArraySize( void )                    { return mNumElements; };

    void                    GetErrorString( char *pString );
    eForthResult            CheckStacks( void );
    void                    SetError( eForthError e, const char *pString = NULL );
    void                    SetFatalError( eForthError e, const char *pString = NULL );
    inline eForthError      GetError( void ) { return (eForthError) (mpCore->error); };
    inline ForthCoreState*  GetCoreState( void ) { return mpCore; };

    static ForthEngine*     GetInstance( void );

protected:
    // NOTE: temporarily modifies string @pToken
    bool                    ScanIntegerToken( char *pToken, long *pValue, int base, bool& isOffset );
    // NOTE: temporarily modifies string @pToken
    bool                    ScanFloatToken( char *pToken, float& fvalue, double& dvalue, bool& isSingle );

protected:
    ForthCoreState*  mpCore;             // core inner interpreter state

    ForthVocabulary * mpMainVocab;              // main forth vocabulary
    ForthVocabulary * mpLocalVocab;             // local variable vocabulary
    ForthVocabulary * mpPrecedenceVocab;        // vocabulary for symbols with precedence

    ForthVocabulary * mpDefinitionVocab;    // vocabulary which new definitions are added to
    ForthVocabulary * mpSearchVocab;        // vocabulary where symbol lookup begins
    
    char        *mpStringBufferA;       // string buffer A is used for quoted strings when in interpreted mode
    char        *mpStringBufferB;       // string buffer B is the buffer which string IO ops append to

    long        mCompileState;          // true iff compiling

    ForthThread *   mpThreads;
    ForthThread *   mpCurrentThread;
    ForthShell  *   mpShell;
    long *          mpEngineScratch;
    char *          mpLastToken;
    int             mNextStringNum;
    long *          mpLastCompiledOpcode;
    long            mLocalFrameSize;
    long *          mpLocalAllocOp;
    const char *    mpErrorString;  // optional error information from shell

    ForthStructsManager *mpStructsManager;

    interpreterExtensionRoutine *mpInterpreterExtension;

    long            mCompileFlags;
    long            mNumElements;       // number of elements in next array declared
    bool            mFastMode;

    static ForthEngine* mpInstance;
};




#endif
