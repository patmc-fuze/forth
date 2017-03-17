#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthEngine.h: interface for the ForthEngine class.
//
//////////////////////////////////////////////////////////////////////


#include <sys/types.h>
#include <sys/timeb.h>
#include <map>

#include "Forth.h"
#include "ForthThread.h"
#include "ForthShell.h"
#include "ForthInner.h"
#include "ForthVocabulary.h"
#include "ForthStructs.h"

class ForthThread;
class ForthShell;
class ForthExtension;
class ForthOpcodeCompiler;
class ForthBlockFileManager;

#define DEFAULT_USER_STORAGE 16384

#define MAIN_THREAD_PSTACK_LONGS   8192
#define MAIN_THREAD_RSTACK_LONGS   8192

// this is the size of the buffer returned by GetTmpStringBuffer()
//  which is the buffer used by word and blword
#define TMP_STRING_BUFFER_LEN MAX_STRING_SIZE

typedef enum {
    //kEngineFlagHasLocalVars              = 0x01,
    kEngineFlagInStructDefinition        = 0x02,
    kEngineFlagIsPointer                 = 0x04,
    kEngineFlagInEnumDefinition          = 0x08,
    kEngineFlagIsMethod                  = 0x10,
    kEngineFlagInClassDefinition         = 0x20,
    //kEngineFlagAnsiMode                = 0x40,
    kEngineFlagNoNameDefinition          = 0x80,
} FECompileFlags;

    //long                *DP;            // dictionary pointer
    //long                *DBase;         // base of dictionary
    //ulong               DLen;           // max size of dictionary memory segment

class ForthEngineTokenStack
{
public:
	ForthEngineTokenStack();
	~ForthEngineTokenStack();
	void Initialize(ulong numBytes);
	inline bool IsEmpty() const { return mpCurrent == mpLimit; };
	void Push(const char* pToken);
	char* Pop();
	char* Peek();
	void Clear();

private:
	char*               mpCurrent;
	char*               mpBase;
	char*				mpLimit;
	ulong               mNumBytes;
};

class ForthEngine
{
public:
                    ForthEngine();
    virtual         ~ForthEngine();

    void            Initialize( ForthShell* pShell,
                                int storageLongs=DEFAULT_USER_STORAGE,
                                bool bAddBaseOps=true,
                                ForthExtension* pExtension=NULL );
    void            Reset( void );
    void            ErrorReset( void );

    void            SetFastMode( bool goFast );
    void            ToggleFastMode( void );
    bool            GetFastMode( void );

    //
    // FullyExecuteOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
    // execute forth ops, and is also how systems external to forth execute ops
    //
	eForthResult        FullyExecuteOp(ForthCoreState* pCore, long opCode);
	// ExecuteOp will start execution of an op, but will not finish user defs or methods
	eForthResult        ExecuteOp(ForthCoreState* pCore, long opCode);
	// ExecuteOps executes a sequence of forth ops
    // The sequence must be terminated with an OP_DONE
	eForthResult        ExecuteOps(ForthCoreState* pCore, long* pOps);

    eForthResult        ExecuteOneMethod( ForthCoreState* pCore, ForthObject& obj, long methodNum );

    // add an op to the operator dispatch table. returns the assigned opcode (without type field)
    long            AddOp( const long *pOp );
    long            AddUserOp( const char *pSymbol, long** pEntryOut=NULL, bool smudgeIt=false );
    long*           AddBuiltinOp( const char* name, ulong flags, ulong value );
    void            AddBuiltinOps( baseDictionaryEntry *pEntries );

	ForthClassVocabulary*   StartClassDefinition(const char* pClassName, eBuiltinClassIndex classIndex = kNumBuiltinClasses);
	void					EndClassDefinition();
	ForthClassVocabulary*   AddBuiltinClass(const char* pClassName, eBuiltinClassIndex classIndex, eBuiltinClassIndex parentClassIndex, baseMethodEntry *pEntries);

    // forget the specified op and all higher numbered ops, and free the memory where those ops were stored
    void            ForgetOp( ulong opNumber, bool quietMode=true );
    // forget the named symbol - return false if symbol not found
    bool            ForgetSymbol( const char *pSym, bool quietMode=true );

    // create a thread which will be managed by the engine - the engine destructor will delete all threads
    //  which were created with CreateThread 
    ForthAsyncThread * CreateAsyncThread( long threadLoopOp = OP_DONE, int paramStackSize = DEFAULT_PSTACK_SIZE, int returnStackSize = DEFAULT_RSTACK_SIZE );
	void               DestroyAsyncThread(ForthAsyncThread *pThread);

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

    // returns true IFF file opened successfully
    bool            PushInputFile( const char *pInFileName );
    void            PushInputBuffer( const char *pDataBuffer, int dataBufferLen );
    void            PushInputBlocks( unsigned int firstBlock, unsigned int lastBlock );
    void            PopInputStream( void );

    // returns pointer to new vocabulary entry
    long *          StartOpDefinition( const char *pName=NULL, bool smudgeIt=false, forthOpType opType=kOpUserDef );
    void            EndOpDefinition( bool unsmudgeIt=false );
    // return pointer to symbol entry, NULL if not found
    long *          FindSymbol( const char *pSymName );
    void            DescribeSymbol( const char *pSymName );
    void            DescribeOp( const char *pSymName, long op, long auxData );

    void            StartStructDefinition( void );
    void            EndStructDefinition( void );
    // returns size of local stack frame in bytes after adding local var
    long            AddLocalVar( const char *pName, long typeCode, long varSize );
    long            AddLocalArray( const char *pName, long typeCode, long varSize );
	bool			HasLocalVariables();

    eForthResult    ProcessToken( ForthParseInfo *pInfo );
    char *          GetLastInputToken( void );

    const char *            GetOpTypeName( long opType );
	void                    TraceOp(ForthCoreState* pCore, long op);
	void                    TraceOp(ForthCoreState* pCore);
	void                    TraceStack(ForthCoreState* pCore);
    void                    DescribeOp( long *pOp, char *pBuffer, int buffSize, bool lookupUserDefs=false );
    long *                  NextOp( long *pOp );

    inline long *           GetDP() { return mDictionary.pCurrent; };
    inline void             SetDP( long *pNewDP ) { mDictionary.pCurrent = pNewDP; };
    inline void             CompileLong( long v ) { *mDictionary.pCurrent++ = v; };
    inline void             CompileDouble( double v ) { *((double *) mDictionary.pCurrent) = v; mDictionary.pCurrent += 2; };
	void					CompileOpcode( forthOpType opType, long opVal );
	void					CompileOpcode( long op );
    void                    CompileBuiltinOpcode( long v );
    void                    UncompileLastOpcode( void );
    long *					GetLastCompiledOpcodePtr( void );
    long *					GetLastCompiledIntoPtr( void );
    void                    ProcessConstant( long value, bool isOffset=false );
    void                    ProcessLongConstant( long long value );
    inline void             AllotLongs( int n ) { mDictionary.pCurrent += n; };
	inline void             AllotBytes( int n )	{ mDictionary.pCurrent = reinterpret_cast<long *>(reinterpret_cast<int>(mDictionary.pCurrent) + n); };
    inline void             AlignDP( void ) { mDictionary.pCurrent = (long *)(( ((int)mDictionary.pCurrent) + 3 ) & ~3); };
    inline ForthMemorySection* GetDictionaryMemorySection() { return &mDictionary; };

	inline ForthEngineTokenStack* GetTokenStack() { return &mTokenStack; };

    inline ForthVocabulary  *GetSearchVocabulary( void )   { return mpVocabStack->GetTop(); };
    inline void             SetSearchVocabulary( ForthVocabulary* pVocab )  { mpVocabStack->SetTop( pVocab ); };
    inline ForthVocabulary  *GetDefinitionVocabulary( void )   { return mpDefinitionVocab; };
    inline void             SetDefinitionVocabulary( ForthVocabulary* pVocab )  { mpDefinitionVocab = pVocab; };
    inline ForthLocalVocabulary  *GetLocalVocabulary( void )   { return mpLocalVocab; };
	void					ShowSearchInfo();
    inline ForthShell       *GetShell( void ) { return mpShell; };
	inline void				SetShell( ForthShell *pShell ) { mpShell = pShell; };
    inline ForthVocabulary  *GetForthVocabulary( void )   { return mpForthVocab; };
    inline ForthThread      *GetMainThread( void )  { return mpMainThread->GetThread(0); };

    inline long             *GetCompileStatePtr( void ) { return &mCompileState; };
    inline void             SetCompileState( long v ) { mCompileState = v; };
    inline long             IsCompiling( void ) { return mCompileState; };
    inline bool             InStructDefinition( void ) { return ((mCompileFlags & kEngineFlagInStructDefinition) != 0); };
    inline bool             HasLocalVars( void ) { return (mpLocalAllocOp != NULL); };
    inline long             GetFlags( void ) { return mCompileFlags; };
    inline void             SetFlags( long flags ) { mCompileFlags = flags; };
    inline void             SetFlag( long flags ) { mCompileFlags |= flags; };
    inline void             ClearFlag( long flags ) { mCompileFlags &= (~flags); };
    inline long             CheckFlag( long flags ) { return mCompileFlags & flags; };
    inline long&            GetFeatures( void ) { return mFeatures; };
    inline void             SetFeatures( long features ) { mFeatures = features; };
    inline void             SetFeature( long features ) { mFeatures |= features; };
    inline void             ClearFeature( long features ) { mFeatures &= (~features); };
    inline long             CheckFeature( long features ) { return mFeatures & features; };
    inline char *           GetTmpStringBuffer( void ) { return mpStringBufferB; };
	inline int				GetTmpStringBufferSize( void ) { return MAX_STRING_SIZE; };
    inline void             SetArraySize( long numElements )        { mNumElements = numElements; };
    inline long             GetArraySize( void )                    { return mNumElements; };

    void                    GetErrorString( char *pBuffer, int bufferSize );
    eForthResult            CheckStacks( void );
    void                    SetError( eForthError e, const char *pString = NULL );
    void                    AddErrorText( const char *pString );
    void                    SetFatalError( eForthError e, const char *pString = NULL );
    inline eForthError      GetError( void ) { return (eForthError) (mpCore->error); };
    // for some reason, inlining this makes it return bogus data in some cases - WTF?
	//inline ForthCoreState*  GetCoreState( void ) { return mpCore; };
    ForthCoreState*			GetCoreState( void );

    void                    StartEnumDefinition( void );
    void                    EndEnumDefinition( void );

    ForthVocabularyStack*   GetVocabularyStack( void )              { return mpVocabStack; };

    static ForthEngine*     GetInstance( void );

	void					SetDefaultConsoleOut( ForthObject& newOutStream );
	void					SetConsoleOut( ForthCoreState* pCore, ForthObject& newOutStream );
	void					PushConsoleOut( ForthCoreState* pCore );
	void					PushDefaultConsoleOut( ForthCoreState* pCore );
	void					ResetConsoleOut( ForthCoreState* pCore );

    // return milliseconds since engine was created
    unsigned long           GetElapsedTime( void );

	void                    ConsoleOut( const char* pBuffer );
	void                    TraceOut( const char* pFormat, ... );

	long					GetTraceFlags( void );
	void					SetTraceFlags( long flags );

	void					SetTraceOutRoutine(traceOutRoutine traceRoutine, void* pTraceData);
	void					GetTraceOutRoutine(traceOutRoutine& traceRoutine, void*& pTraceData) const;

	void					DumpCrashState();

	// squish float/double down to 24-bits, returns true IFF number can be represented exactly OR approximateOkay==true and number is within range of squished float
	bool					SquishFloat( float fvalue, bool approximateOkay, ulong& squishedFloat );
	float					UnsquishFloat( ulong squishedFloat );

	bool					SquishDouble( double dvalue, bool approximateOkay, ulong& squishedDouble );
	double					UnsquishDouble( ulong squishedDouble );
	// squish 64-bit int to 24 bits, returns true IFF number can be represented in 24 bits
	bool					SquishLong( long long lvalue, ulong& squishedLong );
	long long				UnsquishLong( ulong squishedLong );

    inline long*            GetBlockPtr() { return &mBlockNumber; };
    ForthBlockFileManager*  GetBlockFileManager();

	bool					IsServer() const;
	void					SetIsServer(bool isServer);

protected:
    // NOTE: temporarily modifies string @pToken
    bool                    ScanIntegerToken( char* pToken, long& value, long long& lvalue, int base, bool& isOffset, bool& isSingle );
    // NOTE: temporarily modifies string @pToken
    bool                    ScanFloatToken( char *pToken, float& fvalue, double& dvalue, bool& isSingle, bool& isApproximate );


	long*					FindUserDefinition( ForthVocabulary* pVocab, long*& pClosestIP, long* pIP, long*& pBase );
	void					DisplayUserDefCrash( long *pRVal, char* buff, int buffSize );

protected:
    ForthCoreState*  mpCore;             // core inner interpreter state

    ForthMemorySection mDictionary;

	ForthEngineTokenStack mTokenStack;		// contains tokens which will be gotten by GetNextSimpleToken instead of from input stream

	ForthVocabulary * mpForthVocab;              // main forth vocabulary
    ForthLocalVocabulary * mpLocalVocab;         // local variable vocabulary

    ForthVocabulary * mpDefinitionVocab;    // vocabulary which new definitions are added to
    ForthVocabularyStack * mpVocabStack;

	ForthOpcodeCompiler* mpOpcodeCompiler;
    ForthBlockFileManager* mBlockFileManager;

    char        *mpStringBufferA;       // string buffer A is used for quoted strings when in interpreted mode
    char        *mpStringBufferANext;   // one char past last used in A
    int         mStringBufferASize;
    char        *mpStringBufferB;       // string buffer B is the buffer which string IO ops append to

    long        mCompileState;          // true iff compiling

	ForthAsyncThread * mpThreads;
	ForthAsyncThread * mpMainThread;
    ForthShell  *   mpShell;
    long *          mpEngineScratch;
    char *          mpLastToken;
    long            mLocalFrameSize;
    long *          mpLocalAllocOp;
    char *          mpErrorString;  // optional error information from shell

    ForthTypesManager *mpTypesManager;

    interpreterExtensionRoutine *mpInterpreterExtension;

    long            mCompileFlags;
    long            mFeatures;
    long            mNumElements;       // number of elements in next array declared
	long			mTraceFlags;

	traceOutRoutine	mTraceOutRoutine;
	void*			mpTraceOutData;

    long *          mpEnumStackBase;
    long            mNextEnum;

    long            mBlockNumber;       // number returned by 'blk'

	ForthObject		mDefaultConsoleOutStream;

#ifdef WIN32
#ifdef MSDEV
    //struct _timeb   mStartTime;
	struct __timeb32	mStartTime;
#else
    struct _timeb    mStartTime;
#endif
#else
    struct timeb    mStartTime;
#endif
    ForthExtension* mpExtension;

    static ForthEngine* mpInstance;
    bool            mFastMode;
	bool			mIsServer;
};

