// ForthEngine.h: interface for the ForthEngine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORTHENGINE_H__C43FADC2_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
#define AFX_FORTHENGINE_H__C43FADC2_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Forth.h"

class ForthThread;
class ForthShell;

class ForthEngine
{
public:
                    ForthEngine();
    virtual         ~ForthEngine();

    void            Initialize( int storageBytes, int vocabularyBytes, int vocabularyEntries, bool bAddBaseOps );
    // add an op to the operator dispatch table. returns the assigned opcode (without type field)
    long            AddOp( long *pOp );
    void            AddBuiltinOps( baseDictEntry *pEntries );

    ForthThread *   CreateThread( int paramStackSize, int returnStackSize, long *pInitialIP );

    // interpret named file, interpret from standard in if pFileName is NULL
    // return 0 for normal exit
    //int             ForthShell( ForthThread *g, char *pFileName );
    char *          GetNextSimpleToken( void );

    void            PushInputStream( FILE *pInFile );
    void            PopInputStream( void );

    void            StartOpDefinition( void );
    void            EndOpDefinition( void );
    void            StartVarsDefinition( void );
    void            EndVarsDefinition( void );
    void            AddLocalVar( char *pName, forthOpType varType, long varSize );

    eForthResult    ProcessToken( ForthThread *g, char *pToken, int parseFlags );
    char *          GetLastInputToken( void );

    // action routines for different op types
    static void     SimpleOp( ForthThread *g, ulong loOp );
    static void     UserOp( ForthThread *g, ulong loOp );
    static void     ImmediateLong( ForthThread *g, ulong loOp );
    static void     ImmediateString( ForthThread *g, ulong loOp );
    static void     RelBranch( ForthThread *g, ulong loOp );
    static void     RelBranchTrue( ForthThread *g, ulong loOp );
    static void     RelBranchFalse( ForthThread *g, ulong loOp );
    static void     UserImmediateOp( ForthThread *g, ulong loOp );
    static void     AllocateLocals( ForthThread *g, ulong loOp );
    static void     LocalStore32( ForthThread *g, ulong loOp );
    static void     LocalStore64( ForthThread *g, ulong loOp );
    static void     LocalIntAction( ForthThread *g, ulong loOp );
    static void     LocalFloatAction( ForthThread *g, ulong loOp );
    static void     LocalDoubleAction( ForthThread *g, ulong loOp );
    static void     LocalStringAction( ForthThread *g, ulong loOp );
    static void     BadOpcodeTypeError( ForthThread *g, ulong loOp );

    void                    TraceOp( ForthThread *g );
    eForthResult            InnerInterpreter( ForthThread *g );

    inline long *           GetDP() { return mpDP; };
    inline void             SetDP( long *pNewDP ) { mpDP = pNewDP; };
    inline void             CompileLong( long v ) { *mpDP++ = v; };
    inline void             AllotLongs( int n ) { mpDP += n; };
    inline void             AlignDP( void ) { mpDP = (long *)(( ((int)mpDP) + 3 ) & ~3); };
    inline ForthVocabulary  *GetCurrentVocabulary( void )   { return mpCurrentVocab; };
    inline ForthVocabulary  *GetLocalVocabulary( void )   { return mpLocalVocab; };
    inline void             SetShell( ForthShell *pShell ) { mpShell = pShell; };

    inline long             *GetCompileStatePtr( void ) { return &mCompileState; };
    inline void             SetCompileState( long v ) { mCompileState = v; };
    inline long             IsCompiling( void ) { return mCompileState; };
    inline bool             InVarsDefinition( void ) { return mbInVarsDefinition; };


protected:
    long        *mpDP;                  // dictionary pointer
    long        *mpDBase;               // base of dictionary
    ulong       mDLen;                  // max size of dictionary memory segment

    ulong       mNumBuiltinOps;         // number of built-in ops

    ulong       mNumOps;                // gNumOps is current number of entries in userDict

    ulong       mMaxOps;                // gMaxOps is max entries in userDict

    ForthVocabulary *mpMainVocab;       // main forth vocabulary
    ForthVocabulary *mpLocalVocab;      // local variable vocabulary

    ForthVocabulary *mpCurrentVocab;    // vocabulary which new definitions are added to
    
    char        *mpStringBufferA;       // string buffer A is used for quoted strings when in interpreted mode
    char        *mpStringBufferB;       // string buffer B is the buffer which string IO ops append to

    long        **mpOpTable;            // ptr to table of ForthOp adresses for built-in ops, and IP
                                        //   addresses for user-defined ops

    long        mCompileState;          // true iff compiling

    ForthThread *mpThreads;
    ForthShell  *mpShell;
    long        *mpEngineScratch;
    char        *mpLastToken;
    int         mNextStringNum;
    long        mLastCompiledOpcode;
    long        mLocalFrameSize;
    long        *mpLocalAllocOp;
    bool        mbInVarsDefinition;
};




#endif // !defined(AFX_FORTHENGINE_H__C43FADC2_9009_11D4_A3C4_FD0788C5AC51__INCLUDED_)
