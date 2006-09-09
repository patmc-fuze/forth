// ForthEngine.h: interface for the ForthEngine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORTHENGINE_H__C43FADC2_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
#define AFX_FORTHENGINE_H__C43FADC2_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"
#include "ForthThread.h"
#include "ForthShell.h"

class ForthThread;
class ForthShell;
#define DEFAULT_USER_STORAGE 16384

typedef enum {
    kFECompileFlagInVarsDefinition = 1,
    kFECompileFlagHasLocalVars = 2,
    kFECompileFlagIsMethod = 4,
    kFECompileFlagInClassDefinition = 8,
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

    // add an op to the operator dispatch table. returns the assigned opcode (without type field)
    long            AddOp( const long *pOp );
    long            AddUserOp( const char *pSymbol, bool smudgeIt=false );
    void            AddBuiltinOps( baseDictEntry *pEntries );

    // forget the specified op and all higher numbered ops, and free the memory where those ops were stored
    void            ForgetOp( ulong opNumber );
    // forget the named symbol
    void            ForgetSymbol( const char *pSym );

    // create a thread which will be managed by the engine - the engine destructor will delete all threads
    //  which were created with CreateThread 
    ForthThread *   CreateThread( int paramStackSize = DEFAULT_PSTACK_SIZE, int returnStackSize = DEFAULT_RSTACK_SIZE, long *pInitialIP = NULL );
    void            DestroyThread( ForthThread *pThread );

    // add a user-defined extension to the outer interpreter
    inline void             SetInterpreterExtension( interpreterExtensionRoutine *pRoutine )
    {
        mpInterpreterExtension = pRoutine;
    };

    // add a user-defined forthop type
    // opType should be in range kOpLocalUserDefined ... kOpMaxLocalUserDefined
    // return false IFF opType is out of range
    static bool             AddOpType( forthOpType opType, optypeActionRoutine opAction );

    char *          GetNextSimpleToken( void );

    void            PushInputStream( FILE *pInFile );
    void            PopInputStream( void );

    void            StartOpDefinition( bool smudgeIt=false );
    void            EndOpDefinition( bool unsmudgeIt=false );
    // return pointer to symbol entry, NULL if not found
    void *          FindSymbol( const char *pSymName );
    void            StartVarsDefinition( void );
    void            EndVarsDefinition( void );
    void            AddLocalVar( const char *pName, forthOpType varType, long varSize );

    eForthResult    ProcessToken( ForthThread *g, ForthParseInfo *pInfo );
    char *          GetLastInputToken( void );

    // action routines for different op types
    static void     LocalIntAction( ForthThread *g, ulong loOp );
    static void     LocalFloatAction( ForthThread *g, ulong loOp );
    static void     LocalDoubleAction( ForthThread *g, ulong loOp );
    static void     LocalStringAction( ForthThread *g, ulong loOp );

    static void     FieldIntAction( ForthThread *g, ulong loOp );
    static void     FieldFloatAction( ForthThread *g, ulong loOp );
    static void     FieldDoubleAction( ForthThread *g, ulong loOp );
    static void     FieldStringAction( ForthThread *g, ulong loOp );

    static void     InvokeClassMethod( ForthThread *g, ulong loOp );
    static void     BadOpcodeTypeError( ForthThread *g, ulong loOp );
    static bool     IsExecutableType( forthOpType symType );

    static void     MemberIntAction( ForthThread *g, ulong loOp );
    static void     MemberFloatAction( ForthThread *g, ulong loOp );
    static void     MemberDoubleAction( ForthThread *g, ulong loOp );
    static void     MemberStringAction( ForthThread *g, ulong loOp );


    void                    TraceOp( ForthThread *g );
    virtual eForthResult    InnerInterpreter( ForthThread *g );

    inline long *           GetDP() { return mpDP; };
    inline void             SetDP( long *pNewDP ) { mpDP = pNewDP; };
    inline void             CompileLong( long v ) { *mpDP++ = v; };
    inline void             CompileDouble( long v ) { *((double *) mpDP) = v; mpDP += 2; };
    inline void             AllotLongs( int n ) { mpDP += n; };
    inline void             AlignDP( void ) { mpDP = (long *)(( ((int)mpDP) + 3 ) & ~3); };
    inline ForthVocabulary  *GetSearchVocabulary( void )   { return mpSearchVocab; };
    inline ForthVocabulary  *GetPrecedenceVocabulary( void )   { return mpPrecedenceVocab; };
    inline ForthVocabulary  *GetDefinitionVocabulary( void )   { return mpDefinitionVocab; };
    inline ForthVocabulary  *GetLocalVocabulary( void )   { return mpLocalVocab; };
    inline void             SetShell( ForthShell *pShell ) { mpShell = pShell; };
    inline ForthShell *     GetShell( void ) { return mpShell; };

    inline long             *GetCompileStatePtr( void ) { return &mCompileState; };
    inline void             SetCompileState( long v ) { mCompileState = v; };
    inline long             IsCompiling( void ) { return mCompileState; };
    inline bool             InVarsDefinition( void ) { return ((mCompileFlags & kFECompileFlagInVarsDefinition) != 0); };
    inline long             GetCompileFlags( void ) { return mCompileFlags; };
    inline void             SetCompileFlags( long flags ) { mCompileFlags = flags; };
    inline void             SetCompileFlag( long flags ) { mCompileFlags |= flags; };
    inline void             ClearCompileFlag( long flags ) { mCompileFlags &= (~flags); };

protected:
    bool                    ScanIntegerToken( const char *pToken, long *pValue, int base );

protected:
    long *      mpDP;                   // dictionary pointer
    long *      mpDBase;                // base of dictionary
    ulong       mDLen;                  // max size of dictionary memory segment

    ulong       mNumBuiltinOps;         // number of built-in ops

    ulong       mNumOps;                // gNumOps is current number of entries in userDict

    ulong       mMaxOps;                // gMaxOps is max entries in userDict

    ForthVocabulary * mpMainVocab;          // main forth vocabulary
    ForthVocabulary * mpLocalVocab;         // local variable vocabulary
    ForthVocabulary * mpPrecedenceVocab;    // vocabulary for symbols with precedence

    ForthVocabulary * mpDefinitionVocab;    // vocabulary which new definitions are added to
    ForthVocabulary * mpSearchVocab;        // vocabulary where symbol lookup begins
    
    char        *mpStringBufferA;       // string buffer A is used for quoted strings when in interpreted mode
    char        *mpStringBufferB;       // string buffer B is the buffer which string IO ops append to

    long        **mpOpTable;            // ptr to table of ForthOp adresses for built-in ops, and IP
                                        //   addresses for user-defined ops

    long        mCompileState;          // true iff compiling

    ForthThread *   mpThreads;
    ForthShell  *   mpShell;
    long *          mpEngineScratch;
    char *          mpLastToken;
    int             mNextStringNum;
    long            mLastCompiledOpcode;
    long            mLocalFrameSize;
    long *          mpLocalAllocOp;

    interpreterExtensionRoutine *mpInterpreterExtension;

    long        mCompileFlags;
};




#endif // !defined(AFX_FORTHENGINE_H__C43FADC2_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
