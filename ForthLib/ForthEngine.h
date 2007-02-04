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


#define NEW_INNER_INTERP


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
    // forget the named symbol
    void            ForgetSymbol( const char *pSym );

    // create a thread which will be managed by the engine - the engine destructor will delete all threads
    //  which were created with CreateThread 
    ForthThread *   CreateThread( int paramStackSize = DEFAULT_PSTACK_SIZE, int returnStackSize = DEFAULT_RSTACK_SIZE, long *pInitialIP = NULL );
    void            DestroyThread( ForthThread *pThread );

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

    void            PushInputStream( FILE *pInFile );
    void            PopInputStream( void );

    void            StartOpDefinition( bool smudgeIt=false );
    void            EndOpDefinition( bool unsmudgeIt=false );
    // return pointer to symbol entry, NULL if not found
    void *          FindSymbol( const char *pSymName );
    void            StartVarsDefinition( void );
    void            EndVarsDefinition( void );
    void            AddLocalVar( const char *pName, forthOpType varType, long varSize );

    eForthResult    ProcessToken( ForthParseInfo *pInfo );
    char *          GetLastInputToken( void );

    void                    TraceOp( void );

    inline long *           GetDP() { return mCore.DP; };
    inline void             SetDP( long *pNewDP ) { mCore.DP = pNewDP; };
    inline void             CompileLong( long v ) { *mCore.DP++ = v; };
    inline void             CompileDouble( double v ) { *((double *) mCore.DP) = v; mCore.DP += 2; };
    void                    CompileOpcode( long v );
    inline void             AllotLongs( int n ) { mCore.DP += n; };
    inline void             AlignDP( void ) { mCore.DP = (long *)(( ((int)mCore.DP) + 3 ) & ~3); };
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
    inline bool             InVarsDefinition( void ) { return ((mCompileFlags & kFECompileFlagInVarsDefinition) != 0); };
    inline long             GetCompileFlags( void ) { return mCompileFlags; };
    inline void             SetCompileFlags( long flags ) { mCompileFlags = flags; };
    inline void             SetCompileFlag( long flags ) { mCompileFlags |= flags; };
    inline void             ClearCompileFlag( long flags ) { mCompileFlags &= (~flags); };

    void                    SetCurrentThread( ForthThread* pThread );

protected:
    bool                    ScanIntegerToken( const char *pToken, long *pValue, int base );

protected:
    ForthCoreState   mCore;             // core inner interpreter state

    ForthVocabulary * mpMainVocab;          // main forth vocabulary
    ForthVocabulary * mpLocalVocab;         // local variable vocabulary
    ForthVocabulary * mpPrecedenceVocab;    // vocabulary for symbols with precedence

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
    long            mLastCompiledOpcode;
    long            mLocalFrameSize;
    long *          mpLocalAllocOp;

    interpreterExtensionRoutine *mpInterpreterExtension;

    long        mCompileFlags;
};




#endif
