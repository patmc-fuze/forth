//////////////////////////////////////////////////////////////////////
//
// ForthEngine.cpp: implementation of the ForthEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthShell.h"
#include "ForthVocabulary.h"
#include "ForthInner.h"
#include "ForthExtension.h"
#include "ForthStructs.h"

extern "C"
{

    extern baseDictionaryEntry baseDictionary[];
#ifdef _ASM_INNER_INTERPRETER
    extern void InitAsmTables(  ForthCoreState *pCore );
#endif
    extern eForthResult InnerInterp( ForthCoreState *pCore );
    extern void consoleOutToFile( ForthCoreState   *pCore,  const char       *pMessage );
};

//#ifdef TRACE_INNER_INTERPRETER

// provide trace ability for builtin ops
#define NUM_TRACEABLE_OPS MAX_BUILTIN_OPS
static const char *gOpNames[ NUM_TRACEABLE_OPS ];

//#endif

ForthEngine* ForthEngine::mpInstance = NULL;

#define ERROR_STRING_MAX    256

///////////////////////////////////////////////////////////////////////
//
// opTypeNames must be kept in sync with forthOpType enum in forth.h
//

static const char *opTypeNames[] =
{
    "BuiltIn", "BuiltInImmediate", "UserDefined", "UserDefinedImmediate", "UserCode", "UserCodeImmediate", "DLLEntryPoint", 0, 0, 0,
    "Branch", "BranchTrue", "BranchFalse", "CaseBranch", 0, 0, 0, 0, 0, 0,
    "Constant", "ConstantString", "Offset", "ArrayOffset", "AllocLocals", "LocalRef", "InitLocalString", "LocalStructArray", "OffsetFetch", 0,
    "LocalByte", "LocalUByte", "LocalShort", "LocalUShort", "LocalInt", "LocalUInt", "LocalLong", "LocalULong", "LocalFloat", "LocalDouble", "LocalString", "LocalOp", "LocalObject",
    "LocalByteArray", "LocalUByteArray", "LocalShortArray", "LocalUShortArray", "LocalIntArray", "LocalUIntArray", "LocalLongArray", "LocalULongArray", "LocalFloatArray", "LocalDoubleArray", "LocalStringArray", "LocalOpArray", "LocalObjectArray",
    "FieldByte", "FieldUByte", "FieldShort", "FieldUShort", "FieldInt", "FieldUInt", "FieldLong", "FieldULong", "FieldFloat", "FieldDouble", "FieldString", "FieldOp", "FieldObject",
	"FieldByteArray", "FieldUByteArray", "FieldShortArray", "FieldUShortArray", "FieldIntArray", "FieldUIntArray", "FieldLongArray", "FieldULongArray", "FieldFloatArray", "FieldDoubleArray", "FieldStringArray", "FieldOpArray", "FieldObjectArray",
    "MemberByte", "MemberUByte", "MemberShort", "MemberUShort", "MemberInt", "MemberUInt", "MemberLong", "MemberULong", "MemberFloat", "MemberDouble", "MemberString", "MemberOp", "MemberObject",
	"MemberByteArray", "MemberUByteArray", "MemberShortArray", "MemberUShortArray", "MemberIntArray", "MemberUIntArray", "MemberLongArray", "MemberULongArray", "MemberFloatArray", "MemberDoubleArray", "MemberStringArray", "MemberOpArray", "MemberObjectArray",
    "MethodWithThis", "MethodWithTOS", "InitMemberString", 0,
    "LocalUserDefined"
};

///////////////////////////////////////////////////////////////////////
//
// pErrorStrings must be kept in sync with eForthError enum in forth.h
//
static const char *pErrorStrings[] =
{
    "No Error",
    "Bad Opcode",
    "Bad OpcodeType",
    "Bad Parameter",
    "Bad Variable Operation",
    "Parameter Stack Underflow",
    "Parameter Stack Overflow",
    "Return Stack Underflow",
    "Return Stack Overflow",
    "Unknown Symbol",
    "File Open Failed",
    "Aborted",
    "Can't Forget Builtin Op",
    "Bad Method Number",
    "Unhandled Exception",
    "Missing Preceeding Size Constant",
    "Error In Struct Definition",
    "Error In User-Defined Op",
    "Syntax error",
    "Bad Preprocessor Directive",
};

void defaultTraceOutRoutine( void*, const char* pBuff )
{
    TRACE( "%s", pBuff );
}

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthEngine
// 

ForthEngine::ForthEngine()
: mpForthVocab( NULL )
, mpLocalVocab( NULL )
, mpDefinitionVocab( NULL )
, mpStringBufferA( NULL )
, mpStringBufferB( NULL )
, mpThreads( NULL )
, mpInterpreterExtension( NULL )
, mpMainThread( NULL )
, mFastMode( false )
, mpLastCompiledOpcode( NULL )
, mpLastIntoOpcode( NULL )
, mNumElements( 0 )
, mpTypesManager( NULL )
, mpVocabStack( NULL )
, mDefaultConsoleOut( consoleOutToFile )
, mpDefaultConsoleOutData( stdout )
, mpExtension( NULL )
, mpCore( NULL )
, mpShell( NULL )
, mTraceFlags( kTraceShell )
, mTraceOutRoutine( defaultTraceOutRoutine )
, mpTraceOutData( NULL )
{
    // scratch area for temporary definitions
    ASSERT( mpInstance == NULL );
    mpInstance = this;
    mpEngineScratch = new long[70];
    mpErrorString = new char[ ERROR_STRING_MAX + 1 ];

    // remember creation time for elapsed time method
#ifdef _WINDOWS
    _ftime32_s( &mStartTime );
#endif

    // At this point, the main thread does not exist, it will be created later in Initialize, this
    // is fairly screwed up, it is becauses originally ForthEngine was the center of the universe,
    // and it created the shell, but now the shell is created first, and the shell or the main app
    // can create the engine, and then the shell calls ForthEngine::Initialize to hook the two up.
    // The main thread needs to get the file interface from the shell, so it can't be created until
    // after the engine is connected to the shell.  Did I mention its screwed up?
}

ForthEngine::~ForthEngine()
{
    ForthThread *pNextThread;

    if ( mpExtension != NULL )
    {
        mpExtension->Shutdown();
    }

    if ( mDictionary.pBase )
    {
        delete [] mDictionary.pBase;
        delete mpForthVocab;
        delete mpLocalVocab;
        delete mpTypesManager;
        delete [] mpStringBufferA;
        delete [] mpStringBufferB;
    }
    delete [] mpErrorString;

    ForthVocabulary *pVocab = ForthVocabulary::GetVocabularyChainHead();
    while ( pVocab != NULL )
    {
        ForthVocabulary *pNextVocab = pVocab->GetNextChainVocabulary();
        delete pVocab;
        pVocab = pNextVocab;
    }
    if ( mpCore->optypeAction )
    {
        free( mpCore->optypeAction );
    }
    if ( mpCore->builtinOps )
    {
        free( mpCore->builtinOps );
    }
    if ( mpCore->userOps )
    {
        free( mpCore->userOps );
    }

    // delete all threads;
    while ( mpThreads != NULL )
    {
        pNextThread = mpThreads->mpNext;
        delete mpThreads;
        mpThreads = pNextThread;
    }
    delete mpEngineScratch;

    delete mpVocabStack;

	mpInstance = NULL;
}

ForthEngine*
ForthEngine::GetInstance( void )
{
    ASSERT( mpInstance != NULL );
    return mpInstance;
}

//############################################################################
//
//    system initialization
//
//############################################################################

#define NUM_INTERP_STRINGS 32
#define INTERP_STRINGS_LEN  256

void
ForthEngine::Initialize( ForthShell*        pShell,
                         int                totalLongs,
                         bool               bAddBaseOps,
                         ForthExtension*    pExtension )
{
    mpShell = pShell;

    mDictionary.pBase = new long[totalLongs];
    mDictionary.pCurrent = mDictionary.pBase;

    mpForthVocab = new ForthVocabulary( "forth", NUM_FORTH_VOCAB_VALUE_LONGS );
    mpLocalVocab = new ForthLocalVocabulary( "locals", NUM_LOCALS_VOCAB_VALUE_LONGS );
    mpStringBufferA = new char[INTERP_STRINGS_LEN * NUM_INTERP_STRINGS];
    mpStringBufferB = new char[TMP_STRING_BUFFER_LEN];

    mpMainThread = CreateThread( 0, MAIN_THREAD_PSTACK_LONGS, MAIN_THREAD_RSTACK_LONGS );
    mpCore = &(mpMainThread->mCore);
    mpCore->optypeAction = (optypeActionRoutine *) malloc( sizeof(optypeActionRoutine) * 256 );
    mpCore->numBuiltinOps = 0;
    mpCore->builtinOps = (ForthOp *) malloc( sizeof(ForthOp) * MAX_BUILTIN_OPS );
    mpCore->numUserOps = 0;
    mpCore->maxUserOps = 128;
    mpCore->userOps = (long **) malloc( sizeof(long *) * mpCore->maxUserOps );

    mpTypesManager = new ForthTypesManager();

    mpVocabStack = new ForthVocabularyStack;
    mpVocabStack->Initialize();
    mpVocabStack->Clear();
    mpDefinitionVocab = mpForthVocab;


    //
    // build dispatch table for different opcode types
    //
    InitDispatchTables( mpCore );
#ifdef _ASM_INNER_INTERPRETER
    InitAsmTables( mpCore );
#endif

    if ( bAddBaseOps )
    {
        AddBuiltinOps( baseDictionary );
        mpTypesManager->AddBuiltinClasses( this );
    }
    if ( pExtension != NULL )
    {
        mpExtension = pExtension;
        mpExtension->Initialize( this );
    }

    Reset();
}


void
ForthEngine::ToggleFastMode( void )
{
    SetFastMode(!mFastMode );
}

bool
ForthEngine::GetFastMode( void )
{
    return mFastMode;
}

void
ForthEngine::SetFastMode( bool goFast )
{
    mFastMode = goFast;
}

void
ForthEngine::Reset( void )
{
    mpVocabStack->Clear();

    mNextStringNum = 0;

    mpLastCompiledOpcode = NULL;
    mpLastIntoOpcode = NULL;
    mCompileState = 0;
    mCompileFlags = 0;

    if ( mpExtension != NULL )
    {
        mpExtension->Reset();
    }
}

void
ForthEngine::ErrorReset( void )
{
    Reset();
	mpMainThread->Reset();
}


// add an op to engine dispatch table
long
ForthEngine::AddOp( const long *pOp, forthOpType symType )
{
    long newOp;

    if ( (symType == kOpBuiltIn) || (symType == kOpBuiltInImmediate) )
    {
        ASSERT( mpCore->numBuiltinOps < MAX_BUILTIN_OPS );
        mpCore->builtinOps[ mpCore->numBuiltinOps++ ] = (ForthOp) pOp;

        newOp = mpCore->numBuiltinOps - 1;
    }
    else
    {
        if ( mpCore->numUserOps == mpCore->maxUserOps )
        {
            mpCore->maxUserOps += 128;
            mpCore->userOps = (long **) realloc( mpCore->userOps, sizeof(long *) * mpCore->maxUserOps );
        }
        mpCore->userOps[ mpCore->numUserOps++ ] = (long *) pOp;

        newOp = mpCore->numUserOps - 1;
    }
    return newOp;
}


// add an op to dictionary and corresponding symbol to current vocabulary
long
ForthEngine::AddUserOp( const char *pSymbol, bool smudgeIt )
{
    AlignDP();
    mpDefinitionVocab->AddSymbol( pSymbol, kOpUserDef, (long) mDictionary.pCurrent, true );
    if ( smudgeIt )
    {
        mpDefinitionVocab->SmudgeNewestSymbol();
    }

    return mpCore->numUserOps - 1;
}


void
ForthEngine::AddBuiltinOp( const char* name, ulong flags, ulong value )
{
    // AddSymbol will call ForthEngine::AddOp to add the operators to op table
    mpDefinitionVocab->AddSymbol( name, flags, value, true );

//#ifdef TRACE_INNER_INTERPRETER
    // add built-in op names to table for TraceOp
    if ( (flags == kOpBuiltIn) || ((flags == kOpBuiltInImmediate)) )
    {
        if ( (mpCore->numBuiltinOps - 1) < NUM_TRACEABLE_OPS )
        {
            gOpNames[mpCore->numBuiltinOps - 1] = name;
        }
    }
//#endif
}


void
ForthEngine::AddBuiltinOps( baseDictionaryEntry *pEntries )
{
    // I think this assert is a holdover from when userOps and builtinOps were in a single dispatch table
    // assert if this is called after any user ops have been defined
    //ASSERT( mpCore->numUserOps == 0 );

    while ( pEntries->value != NULL )
    {

        AddBuiltinOp( pEntries->name, pEntries->flags, pEntries->value);
        pEntries++;

    }
}


ForthClassVocabulary*
ForthEngine::StartClassDefinition( const char* pClassName )
{
    SetFlag( kEngineFlagInStructDefinition );
    SetFlag( kEngineFlagInClassDefinition );
	
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->StartClassDefinition( pClassName );

	// add new class vocab to top of search order
	mpVocabStack->DupTop();
	mpVocabStack->SetTop( pVocab );

    CompileOpcode( OP_DO_CLASS_TYPE );
    CompileLong( (long) pVocab );

    return pVocab;
}

void
ForthEngine::EndClassDefinition()
{
	ClearFlag( kEngineFlagInStructDefinition );
    ClearFlag( kEngineFlagInClassDefinition );

    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
	pManager->EndClassDefinition();
	mpVocabStack->DropTop();
}

ForthClassVocabulary*
ForthEngine::AddBuiltinClass( const char* pClassName, ForthClassVocabulary* pParentClass, baseMethodEntry *pEntries )
{
    // do "class:" - define class subroutine
    ForthClassVocabulary* pVocab = StartClassDefinition( pClassName );
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();

    if ( pParentClass )
    {
        // do "extends" - tie into parent class
        pManager->GetNewestStruct()->Extends( pParentClass );
    }

    // loop through pEntries, adding ops to builtinOps table and adding methods to class
    while ( pEntries->name != NULL )
    {
        const char* pMemberName = pEntries->name;
        if ( (pEntries->returnType & kDTIsMethod) != 0 )
        {
            if ( !strcmp( pMemberName, "new" ) )
            {
                // this isn't a new method, it is the class constructor op
                AddBuiltinOp( pMemberName, kOpBuiltIn, pEntries->value );
                pVocab->GetClassObject()->newOp = mpCore->numBuiltinOps - 1;
            }
            else
            {
                // this entry is a member method
                // add method routine to builtinOps table
                long methodOp = OP_BAD_OP;
				if ( pEntries->value != NULL )
				{
					methodOp = AddOp( (long *) pEntries->value, kOpBuiltIn );
					if ( (mpCore->numBuiltinOps - 1) < NUM_TRACEABLE_OPS )
					{
						gOpNames[mpCore->numBuiltinOps - 1] = pMemberName;
					}
				}
                // do "method:"
                long methodIndex = pVocab->FindMethod( pMemberName );
                StartOpDefinition( pMemberName, false );
                long* pEntry = pVocab->GetNewestEntry();
                // pEntry[0] is initially the opcode for the method, now we replace it with the method index,
                //  and put the opcode in the method table
				methodIndex = pVocab->AddMethod( pMemberName, methodIndex, methodOp );
                pEntry[0] = methodIndex;
                pEntry[1] = pEntries->returnType;
                TRACE( "Method %s op is 0x%x\n", pMemberName, methodOp );

                // do ";method"
                EndOpDefinition( false );
            }
        }
        else
        {
            // this entry is a member variable
            pManager->GetNewestStruct()->AddField( pMemberName, pEntries->returnType, (int) pEntries->value );

        }

#ifdef TRACE_INNER_INTERPRETER
        // add built-in op names to table for TraceOp
        if ( (mpCore->numBuiltinOps - 1) < NUM_TRACEABLE_OPS )
        {
            gOpNames[mpCore->numBuiltinOps - 1] = pEntries->name;
        }
#endif
        pEntries++;
    }

    // do ";class"
    ClearFlag( kEngineFlagInStructDefinition );
    pManager->EndClassDefinition();

    return pVocab;
}


// forget the specified op and all higher numbered ops, and free the memory where those ops were stored
void
ForthEngine::ForgetOp( ulong opNumber, bool quietMode )
{
    if ( opNumber < mpCore->numUserOps )
    {
        mDictionary.pCurrent = mpCore->userOps[ opNumber ];
        mpCore->numUserOps = opNumber;
    }
    else
    {
        if ( !quietMode )
        {
            TRACE( "ForthEngine::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, mpCore->numUserOps );
            printf( "ForthEngine::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, mpCore->numUserOps );
        }
    }
    if ( mpExtension != NULL )
    {
        mpExtension->ForgetOp( opNumber );
    }
}

// return true if symbol was found
bool
ForthEngine::ForgetSymbol( const char *pSym, bool quietMode )
{
    long *pEntry = NULL;
    long op;
    forthOpType opType;
    bool forgotIt = false;
    char buff[256];
    buff[0] = '\0';

    ForthVocabulary* pFoundVocab = NULL;
    pEntry = GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );

    if ( pFoundVocab != NULL )
    {
        op = ForthVocabulary::GetEntryValue( pEntry );
        opType = ForthVocabulary::GetEntryType( pEntry );
        switch ( opType )
        {
            case kOpBuiltIn:
            case kOpBuiltInImmediate:
                // sym is built-in op - no way
                sprintf_s( buff, sizeof(buff),  "Error - attempt to forget builtin op %s from %s\n", pSym, pFoundVocab->GetName() );
                break;

            case kOpUserDef:
            case kOpUserDefImmediate:
            case kOpUserCode:
            case kOpUserCodeImmediate:
                ForgetOp( op, quietMode );
                ForthForgettable::ForgetPropagate( mDictionary.pCurrent, op );
                forgotIt = true;
                break;

            default:
                const char* pStr = GetOpTypeName( opType );
                sprintf_s( buff, sizeof(buff), "Error - attempt to forget op %s of type %s from %s\n", pSym, pStr, pFoundVocab->GetName() );
                break;

        }
    }
    else
    {
        sprintf_s( buff, sizeof(buff), "Error - attempt to forget unknown op %s from %s\n", pSym, GetSearchVocabulary()->GetName() );
    }
    if ( buff[0] != '\0' )
    {
        if ( !quietMode )
        {
            printf( "%s", buff );
        }
        TRACE( "%s", buff );
    }
    return forgotIt;
}

ForthThread *
ForthEngine::CreateThread( long threadOp, int paramStackSize, int returnStackSize )
{
    ForthThread *pNewThread = new ForthThread( this, paramStackSize, returnStackSize );
    pNewThread->SetOp( threadOp );

    pNewThread->mCore.pEngine = this;
    pNewThread->mCore.pDictionary = &mDictionary;
    pNewThread->mCore.pFileFuncs = mpShell->GetFileInterface();

    if ( mpCore != NULL )
    {
        // fill in optype & opcode action tables from engine thread
        pNewThread->mCore.optypeAction = mpCore->optypeAction;
        pNewThread->mCore.numBuiltinOps = mpCore->numBuiltinOps;
        pNewThread->mCore.builtinOps = mpCore->builtinOps;
        pNewThread->mCore.numUserOps = mpCore->numUserOps;
        pNewThread->mCore.maxUserOps = mpCore->maxUserOps;
        pNewThread->mCore.userOps = mpCore->userOps;
    }

    pNewThread->mpNext = mpThreads;
    mpThreads = pNewThread;

    return pNewThread;
}


void
ForthEngine::DestroyThread( ForthThread *pThread )
{
    ForthThread *pNext, *pCurrent;

    if ( mpThreads == pThread )
    {

        // special case - thread is head of list
        mpThreads = mpThreads->mpNext;
        delete pThread;

    }
    else
    {

        // TBD: this is untested
        pCurrent = mpThreads;
        while ( pCurrent != NULL )
        {
            pNext = pCurrent->mpNext;
            if ( pThread == pNext )
            {
                pCurrent->mpNext = pNext->mpNext;
                delete pThread;
                return;
            }
            pCurrent = pNext;
        }

        TRACE( "ForthEngine::DestroyThread tried to destroy unknown thread 0x%x!\n", pThread );
        // TBD: raise the alarm
    }
}


// interpret named file, interpret from standard in if pFileName is NULL
// return 0 for normal exit
//int             ForthShell( ForthThread *g, char *pFileName );
char *
ForthEngine::GetNextSimpleToken( void )
{
    return mpShell->GetNextSimpleToken();
}


bool
ForthEngine::PushInputFile( const char *pInFileName )
{
    return mpShell->PushInputFile( pInFileName );
}

void
ForthEngine::PushInputBuffer( char *pDataBuffer, int dataBufferLen )
{
    mpShell->PushInputBuffer( pDataBuffer, dataBufferLen );
}


void
ForthEngine::PopInputStream( void )
{
    mpShell->PopInputStream();
}



long *
ForthEngine::StartOpDefinition( const char *pName, bool smudgeIt, forthOpType opType )
{
    mpLocalVocab->Empty();
    mLocalFrameSize = 0;
    mpLocalAllocOp = NULL;
    mpLastCompiledOpcode = NULL;
    mpLastIntoOpcode = NULL;
    AlignDP();
    if ( pName == NULL )
    {
        pName = GetNextSimpleToken();
    }
    long* pEntry = mpDefinitionVocab->AddSymbol( pName, opType, (long) mDictionary.pCurrent, true );
    if ( smudgeIt )
    {
        mpDefinitionVocab->SmudgeNewestSymbol();
    }

    return pEntry;
}


void
ForthEngine::EndOpDefinition( bool unsmudgeIt )
{
    int nLongs = mLocalFrameSize;
    if ( mpLocalAllocOp != NULL )
    {
        *mpLocalAllocOp = COMPILED_OP( kOpAllocLocals, nLongs );
        mpLocalAllocOp = NULL;
        mLocalFrameSize = 0;
    }
    mpLastCompiledOpcode = NULL;
    mpLastIntoOpcode = NULL;
    if ( unsmudgeIt )
    {
        mpDefinitionVocab->UnSmudgeNewestSymbol();
    }
}


long *
ForthEngine::FindSymbol( const char *pSymName )
{
    ForthVocabulary* pFoundVocab = NULL;
    return GetVocabularyStack()->FindSymbol( pSymName, &pFoundVocab );
}

void
ForthEngine::DescribeSymbol( const char *pSymName )
{
    long *pEntry = NULL;
    char buff[256];
    char buff2[128];
    char c;
    int line = 1;
    bool notDone = true;

    ForthVocabulary* pFoundVocab = NULL;
    pEntry = GetVocabularyStack()->FindSymbol( pSymName, &pFoundVocab );
    if ( pEntry )
    {
        long opType = FORTH_OP_TYPE( pEntry[0] );
        long opValue = FORTH_OP_VALUE( pEntry[0] );
        bool isUserOp = (opType == kOpUserDef) || (opType == kOpUserDefImmediate);
        const char* pStr = GetOpTypeName( opType );
        if ( isUserOp )
        {
            ForthStructVocabulary::TypecodeToString( pEntry[1], buff2, sizeof(buff2) );
            sprintf_s( buff, sizeof(buff), "%s: type %s:%x value 0x%x 0x%x (%s) \n", pSymName, pStr, opValue, pEntry[0], pEntry[1], buff2 );
        }
        else
        {
            sprintf_s( buff, sizeof(buff), "%s: type %s:%x value 0x%x 0x%x \n", pSymName, pStr, opValue, pEntry[0], pEntry[1] );
        }
        ConsoleOut( buff );
        if ( isUserOp )
        {
            // disassemble the op until IP reaches next newer op
            long* curIP = mpCore->userOps[ opValue ];
            long* endIP = (opValue == (mpCore->numUserOps - 1)) ? GetDP() : mpCore->userOps[ opValue + 1 ];
            while ( (curIP < endIP) && notDone )
            {
                sprintf_s( buff, sizeof(buff), "  %08x  ", curIP );
                ConsoleOut( buff );
                DescribeOp( curIP, buff, sizeof(buff), true );
                ConsoleOut( buff );
                sprintf_s( buff, sizeof(buff), "\n" );
                ConsoleOut( buff );
                if ( ((line & 31) == 0) && (mpShell != NULL) && mpShell->GetInput()->InputStream()->IsInteractive() )
                {
                    ConsoleOut( "Hit ENTER to continue, 'q' & ENTER to quit\n" );
                    c = mpShell->GetChar();

                    if ( (c == 'q') || (c == 'Q') )
                    {
                        c = mpShell->GetChar();
                        notDone = false;
                    }
                }
                curIP = NextOp( curIP );
                line++;
            }
        }
    }
    else
    {
        sprintf_s( buff, sizeof(buff), "Symbol %s not found\n", pSymName );
        TRACE( buff );
        ConsoleOut( buff );
    }
}

long *
ForthEngine::NextOp( long *pOp )
{
    long op = *pOp++;
    long opType = FORTH_OP_TYPE( op );
    long opVal = FORTH_OP_VALUE( op );

    switch ( opType )
    {
        case kOpBuiltIn:
            switch( opVal )
            {
                case OP_INT_VAL:
                case OP_FLOAT_VAL:
                case OP_DO_DO:
                case OP_DO_STRUCT_ARRAY:
                    pOp++;
                    break;

                case OP_DOUBLE_VAL:
                    pOp += 2;
                    break;

                default:
                    break;
            }
            break;

        case kOpConstantString:
            pOp += opVal;
            break;

        default:
            break;
    }
    return pOp;
}

void
ForthEngine::StartStructDefinition( void )
{
    mCompileFlags |= kEngineFlagInStructDefinition;
}

void
ForthEngine::EndStructDefinition( void )
{
    mCompileFlags &= (~kEngineFlagInStructDefinition);
}

long
ForthEngine::AddLocalVar( const char        *pVarName,
                          long              typeCode,
                          long              varSize )
{
    long *pEntry;
    if ( mpLocalAllocOp == NULL )
    {
        // this is first local var definition, leave space for local alloc op
        mCompileFlags |= kEngineFlagHasLocalVars;
        mpLocalAllocOp = mDictionary.pCurrent;
        CompileLong( 0 );
    }
    varSize = ((varSize + 3) & ~3) >> 2;
    mLocalFrameSize += varSize;
    long baseType = CODE_TO_BASE_TYPE( typeCode );
    long fieldType = kOpLocalInt;
    if ( !CODE_IS_PTR( typeCode ) )
    {
        if ( baseType <= kBaseTypeObject )
        {
            fieldType = kOpLocalByte + baseType;
        }
        else
        {
            fieldType = kOpLocalRef;
        }
    }
    pEntry = mpLocalVocab->AddSymbol( pVarName, fieldType, mLocalFrameSize, false );
    pEntry[1] = typeCode;

    return mLocalFrameSize;
}

long
ForthEngine::AddLocalArray( const char          *pArrayName,
                            long                typeCode,
                            long                elementSize )
{
    long *pEntry;
    if ( mpLocalAllocOp == NULL )
    {
        // this is first local var definition, leave space for local alloc op
        mCompileFlags |= kEngineFlagHasLocalVars;
        mpLocalAllocOp = mDictionary.pCurrent;
        CompileLong( 0 );
    }

#if 0
    long elementType = CODE_TO_BASE_TYPE( typeCode );
    long fieldBytes, alignment, padding, alignMask;
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    pManager->GetFieldInfo( typeCode, fieldBytes, alignment );
    alignMask = alignment - 1;
    padding = fieldBytes & alignMask;
    elementSize = (padding) ? (fieldBytes + (alignment - padding)) : fieldBytes;
    long arraySize = elementSize * mNumElements;
    mLocalFrameSize += arraySize;

    pEntry = mpLocalVocab->AddSymbol( pArrayName, kOpLocalStructArray,
                                        (mLocalFrameSize << 12) + elementSize, false );
#else
    long elementType = CODE_TO_BASE_TYPE( typeCode );
    if ( elementType != kBaseTypeStruct )
    {
        // array of non-struct
        long arraySize = elementSize * mNumElements;
        arraySize = ((arraySize + 3) & ~3) >> 2;
        mLocalFrameSize += arraySize;
        long opcode = CODE_IS_PTR(typeCode) ? kOpLocalIntArray : (kOpLocalByteArray + CODE_TO_BASE_TYPE(typeCode));
        pEntry = mpLocalVocab->AddSymbol( pArrayName, opcode, mLocalFrameSize, false );
    }
    else
    {
        // array of struct
        long fieldBytes, alignment, padding, alignMask;
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        pManager->GetFieldInfo( typeCode, fieldBytes, alignment );
        alignMask = alignment - 1;
        padding = fieldBytes & alignMask;
        long paddedSize = (padding) ? (fieldBytes + (alignment - padding)) : fieldBytes;
        long arraySize = paddedSize * mNumElements;
        mLocalFrameSize += arraySize;
        if ( CODE_IS_PTR(typeCode) )
        {
            pEntry = mpLocalVocab->AddSymbol( pArrayName, kOpLocalIntArray, mLocalFrameSize, false );
        }
        else
        {
            pEntry = mpLocalVocab->AddSymbol( pArrayName, kOpLocalStructArray, (mLocalFrameSize << 12) + paddedSize, false );
        }
    }
#endif
    pEntry[1] = typeCode;

    mNumElements = 0;
    return mLocalFrameSize;
}


// TBD: tracing of built-in ops won't work for user-added builtins...
const char *
ForthEngine::GetOpTypeName( long opType )
{
    return (opType < kOpLocalUserDefined) ? opTypeNames[opType] : "unknown";
}

static bool lookupUserTraces = true;


void
ForthEngine::TraceOut( const char* pBuff )
{
	if ( mTraceFlags & kTraceToConsole )
	{
		ConsoleOut( pBuff );
	}
	else
	{
		if ( mTraceOutRoutine != NULL )
		{
			mTraceOutRoutine( mpTraceOutData, pBuff );
		}
	}
}


void
ForthEngine::SetTraceOutRoutine( traceOutRoutine traceRoutine, void* pTraceData )
{
	mTraceOutRoutine = traceRoutine;
	mpTraceOutData = pTraceData;
}


void
ForthEngine::TraceOp( ForthCoreState* pCore )
{
#ifdef TRACE_INNER_INTERPRETER
    long *pOp = pCore->IP;
    char buff[ 256 ];
    int rDepth = pCore->RT - pCore->RP;
    if ( rDepth > 8 )
    {
        rDepth = 8;
    }
    char* sixteenSpaces = "                ";     // 16 spaces
    char* pIndent = sixteenSpaces + (16 - (rDepth << 1));
    if ( *pOp != OP_DONE )
    {
		sprintf( buff,  "# 0x%08x ", pOp );
		TraceOut( buff );
		TraceOut( pIndent );
        DescribeOp( pOp, buff, sizeof(buff), lookupUserTraces );
		TraceOut( buff );
		TraceOut( " # " );
    }
#endif
}

void
ForthEngine::TraceStack( ForthCoreState* pCore )
{
	long *pSP = GET_SP;
	int nItems = GET_SDEPTH;
	int i;
	char buff[64];

	sprintf( buff, "  stack[%d]:", nItems );
	TraceOut( buff );
	for ( i = 0; i < nItems; i++ )
	{
		sprintf( buff, " %x", *pSP++ );
		TraceOut( buff );
	}
}

void
ForthEngine::DescribeOp( long *pOp, char *pBuffer, int buffSize, bool lookupUserDefs )
{
    long op = *pOp;
    forthOpType opType = FORTH_OP_TYPE( op );
    ulong opVal = FORTH_OP_VALUE( op );
    ForthVocabulary *pFoundVocab = NULL;
    long *pEntry = NULL;

    sprintf_s( pBuffer, buffSize, "%02x:%06x    ", opType, opVal );
    pBuffer += 13;
	buffSize -= 13;
    if ( opType >= (sizeof(opTypeNames) / sizeof(char *)) )
    {
        sprintf_s( pBuffer, buffSize, "BadOpType" );
    }
    else
    {

        switch( opType )
        {

            case kOpBuiltIn:
            case kOpBuiltInImmediate:
                if ( (opVal < NUM_TRACEABLE_OPS) && (gOpNames[opVal] != NULL) )
                {
                    // traceable built-in op
                    switch( opVal )
                    {
                        case OP_INT_VAL:
                            sprintf_s( pBuffer, buffSize, "%s 0x%x", gOpNames[opVal], pOp[1] );
                            break;

                        case OP_FLOAT_VAL:
                            sprintf_s( pBuffer, buffSize, "%s %f", gOpNames[opVal], *((float *)(&(pOp[1]))) );
                            break;

                        case OP_DOUBLE_VAL:
                            sprintf_s( pBuffer, buffSize, "%s %g", gOpNames[opVal], *((double *)(&(pOp[1]))) );
                            break;

                        default:
                            sprintf_s( pBuffer, buffSize, "%s", gOpNames[opVal] );
                            break;
                    }
                }
                else
                {
                    // op we don't have name pointer for
                    sprintf_s( pBuffer, buffSize, "%s(%d)", opTypeNames[opType], opVal );
                }
                break;
            
            case kOpUserDef:
            case kOpUserDefImmediate:
            case kOpUserCode:
            case kOpUserCodeImmediate:
            case kOpDLLEntryPoint:
                if ( lookupUserDefs )
                {
                    pEntry = GetVocabularyStack()->FindSymbolByValue( op, &pFoundVocab );
                }
                if ( pEntry )
                {
                    // the symbol name in the vocabulary doesn't always have a terminating null
                    int len = pFoundVocab->GetEntryNameLength( pEntry );
                    const char* pName = pFoundVocab->GetEntryName( pEntry );
                    for ( int i = 0; i < len; i++ )
                    {
                        *pBuffer++ = *pName++;
                    }
                    *pBuffer = '\0';
                }
                else
                {
                    sprintf_s( pBuffer, buffSize, "%s", opTypeNames[opType] );
                }
                break;

            case kOpLocalByte:          case kOpLocalByteArray:
            case kOpLocalShort:         case kOpLocalShortArray:
            case kOpLocalInt:           case kOpLocalIntArray:
            case kOpLocalFloat:         case kOpLocalFloatArray:
            case kOpLocalDouble:        case kOpLocalDoubleArray:
            case kOpLocalString:        case kOpLocalStringArray:
            case kOpLocalOp:            case kOpLocalOpArray:
            case kOpLocalLong:          case kOpLocalLongArray:
            case kOpLocalObject:        case kOpLocalObjectArray:
            case kOpLocalUByte:         case kOpLocalUByteArray:
            case kOpLocalUShort:        case kOpLocalUShortArray:
            case kOpMemberByte:          case kOpMemberByteArray:
            case kOpMemberShort:         case kOpMemberShortArray:
            case kOpMemberInt:           case kOpMemberIntArray:
            case kOpMemberFloat:         case kOpMemberFloatArray:
            case kOpMemberDouble:        case kOpMemberDoubleArray:
            case kOpMemberString:        case kOpMemberStringArray:
            case kOpMemberOp:            case kOpMemberOpArray:
            case kOpMemberLong:          case kOpMemberLongArray:
            case kOpMemberObject:        case kOpMemberObjectArray:
            case kOpMemberUByte:         case kOpMemberUByteArray:
            case kOpMemberUShort:        case kOpMemberUShortArray:
                sprintf_s( pBuffer, buffSize, "%s_%x", opTypeNames[opType], opVal );
                break;

            case kOpConstantString:
                sprintf_s( pBuffer, buffSize, "\"%s\"", (char *)(pOp + 1) );
                break;
            
            case kOpConstant:
                if ( opVal & 0x800000 )
                {
                    opVal |= 0xFF000000;
                }
                sprintf_s( pBuffer, buffSize, "%s    %d", opTypeNames[opType], opVal );
                break;

            case kOpOffset:
                if ( opVal & 0x800000 )
                {
                    opVal |= 0xFF000000;
                    sprintf_s( pBuffer, buffSize, "%s    %d", opTypeNames[opType], opVal );
                }
                else
                {
                    sprintf_s( pBuffer, buffSize, "%s    +%d", opTypeNames[opType], opVal );
                }
                break;

            case kOpCaseBranch:
            case kOpBranch:   case kOpBranchNZ:  case kOpBranchZ:
                if ( opVal & 0x800000 )
                {
                    opVal |= 0xFF000000;
                }
                sprintf_s( pBuffer, buffSize, "%s    0x%08x", opTypeNames[opType], opVal + 1 + pOp );
                break;

            case kOpInitLocalString:   // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
            case kOpInitMemberString:   // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
                sprintf_s( pBuffer, buffSize, "%s    maxBytes %d offset %d", opTypeNames[opType], opVal & 0xFFF, opVal >> 12 );
                break;
            
            case kOpLocalStructArray:   // bits 0..11 are padded struct size in bytes, bits 12..23 are frame offset in longs
                sprintf_s( pBuffer, buffSize, "%s    elementSize %d offset %d", opTypeNames[opType], opVal & 0xFFF, opVal >> 12 );
                break;
            
            case kOpAllocLocals:
                sprintf_s( pBuffer, buffSize, "%s    longs %d", opTypeNames[opType], opVal );
                break;
            
            case kOpArrayOffset:
                sprintf_s( pBuffer, buffSize, "%s    elementSize %d", opTypeNames[opType], opVal );
                break;
            
            case kOpMethodWithThis:
            case kOpMethodWithTOS:
                sprintf_s( pBuffer, buffSize, "%s    %d", opTypeNames[opType], opVal );
                break;

            default:
                if ( opType >= (unsigned int)(sizeof(opTypeNames) / sizeof(char *)) )
                {
                    sprintf_s( pBuffer, buffSize, "BAD OPTYPE!" );
                }
                else
                {
                    sprintf_s( pBuffer, buffSize, "%s", opTypeNames[opType] );
                }
                break;
        }
    }
}



bool
ForthEngine::AddOpType( forthOpType opType, optypeActionRoutine opAction )
{

    if ( (opType >= kOpLocalUserDefined) && (opType <= kOpMaxLocalUserDefined) )
    {
        mpCore->optypeAction[ opType ] = opAction;
    }
    else
    {
        // opType out of range
        return false;
    }
    return true;
}


char *
ForthEngine::GetLastInputToken( void )
{
    return mpLastToken;
}


// return true IFF token is an integer literal
// sets isOffset if token ended with a + or -
// NOTE: temporarily modifies string @pToken
bool
ForthEngine::ScanIntegerToken( char         *pToken,
                               long         &value,
                               long long    &lvalue,
                               int          base,
                               bool         &isOffset,
                               bool&        isSingle )
{
    long digit;
    bool isNegative;
    char c;
    int digitsFound = 0;
    int len = strlen( pToken );
    char *pLastChar = pToken + (len - 1);
    char lastChar = *pLastChar;
    bool isValid = false;

    isOffset = false;
    isSingle = true;

    // handle leading plus or minus sign
    isNegative = (pToken[0] == '-');
    if ( isNegative || (pToken[0] == '+') )
    {
        // strip leading plus/minus sign
        pToken++;
    }

    // handle trailing plus or minus sign
    if ( lastChar == '+' )
    {
        isOffset = true;
        *pLastChar = 0;
    }
    else if ( lastChar == '-' )
    {
        isOffset = true;
        *pLastChar = 0;
        isNegative = !isNegative;
    }
    else if ( (lastChar == 'L') || (lastChar == 'l') )
    {
        isSingle = false;
        *pLastChar = 0;
    }
    else
    {
        pLastChar = NULL;
    }

    if ( (pToken[0] == '0') && (pToken[1] == 'x') )
    {
        if ( isSingle )
        {
            if ( sscanf_s( pToken + 2, "%x", &value ) == 1 )
            {
                if ( isNegative )
                {
                    value = 0 - value;
                }
                isValid = true;
            }
        }
        else
        {
            if ( sscanf_s( pToken + 2, "%I64x", &lvalue ) == 1 )
            {
                if ( isNegative )
                {
                    lvalue = 0 - lvalue;
                }
                isValid = true;
            }
        }
    }
    else
    {

        isValid = true;
        value = 0;
        lvalue = 0;
        while ( (c = *pToken++) != 0 )
        {

            if ( (c >= '0') && (c <= '9') )
            {
                digit = c - '0';
                digitsFound++;
            }
            else if ( (c >= 'A') && (c <= 'Z') )
            {
                digit = 10 + (c - 'A');
                digitsFound++;
            }
            else if ( (c >= 'a') && (c <= 'z') )
            {
                digit = 10 + (c - 'a');
                digitsFound++;
            }
            else
            {
                // char can't be a digit
                isValid = false;
                break;
            }

            if ( digit >= base )
            {
                // invalid digit for current base
                isValid = false;
                break;
            }
            if ( isSingle )
            {
                value = (value * base) + digit;
            }
            else
            {
                lvalue = (lvalue * base) + digit;
            }
        }

        if ( digitsFound == 0 )
        {
            isValid = false;
        }

        // all chars were valid digits
        if ( isNegative )
        {
            if ( isSingle )
            {
                value = 0 - value;
            }
            else
            {
                lvalue = 0 - lvalue;
            }
        }
    }

    // restore original last char
    if ( pLastChar != NULL )
    {
        *pLastChar = lastChar;
    }
    return isValid;
}


// return true IFF token is a real literal
// sets isSingle to tell if result is a float or double
// NOTE: temporarily modifies string @pToken
bool ForthEngine::ScanFloatToken( char *pToken, float& fvalue, double& dvalue, bool& isSingle )
{
   bool retVal = false;
   int len = strlen( pToken );

   if ( strchr( pToken, '.' ) == NULL )
   {
      return false;
   }
   char *pLastChar = pToken + (len - 1);
   char lastChar = *pLastChar;
   switch ( lastChar )
   {
   case 'd':
   case 'g':
      *pLastChar = 0;
      if ( sscanf_s( pToken, "%lf", &dvalue ) == 1)
      {
         retVal = true;
         isSingle = false;
      }
      *pLastChar = lastChar;
      break;
   case 'f':
      *pLastChar = 0;
      if ( sscanf_s( pToken, "%f", &fvalue ) == 1)
      {
         retVal = true;
         isSingle = true;
      }
      *pLastChar = lastChar;
      break;
   default:
       if ( sscanf_s( pToken, "%f", &fvalue ) == 1)
        {
            retVal = true;
            isSingle = true;
        }
        break;
   }

   return retVal;
}


// compile an opcode
// remember the last opcode compiled so someday we can do optimizations
//   like combining "->" followed by a local var name into one opcode
void
ForthEngine::CompileOpcode( long op )
{
    mpLastCompiledOpcode = mDictionary.pCurrent;
    if ( op == OP_INTO )
    {
       // we need this to support initialization of local string vars (ugh)
       mpLastIntoOpcode = mpLastCompiledOpcode;
       
    }
	if ( mTraceFlags & kTraceCompilation )
	{
		char buff[ 256 ];
		sprintf( buff,  "Compiling 0x%08x @ 0x%08x\n", op, mDictionary.pCurrent );
		TraceOut( buff );
	}
    *mDictionary.pCurrent++ = op;
}

void
ForthEngine::UncompileLastOpcode( void )
{
    if ( mpLastCompiledOpcode != NULL )
    {
        if ( mpLastCompiledOpcode <= mpLastIntoOpcode )
        {
            mpLastIntoOpcode = NULL;

        }
		if ( mTraceFlags & kTraceCompilation )
		{
			char buff[ 256 ];
			sprintf( buff,  "Uncompiling from 0x08x to 0x%08x\n", mDictionary.pCurrent, mpLastCompiledOpcode );
			TraceOut( buff );
		}

        mDictionary.pCurrent = mpLastCompiledOpcode;
        mpLastCompiledOpcode = NULL;
    }
    else
    {
        TRACE( "ForthEngine::UncompileLastOpcode called with no previous opcode\n" );
        SetError( kForthErrorMissingSize, "UncompileLastOpcode called with no previous opcode" );
    }
}

// interpret/compile a constant value/offset
void
ForthEngine::ProcessConstant( long value, bool isOffset )
{
    if ( mCompileState )
    {
        // compile the literal value
        if ( (value < (1 << 23)) && (value >= -(1 << 23)) )
        {
            // value fits in opcode immediate field
            if ( isOffset )
            {
                CompileOpcode( (value & 0xFFFFFF) | (kOpOffset << 24) );
            }
            else
            {
                CompileOpcode( (value & 0xFFFFFF) | (kOpConstant << 24) );
            }
        }
        else
        {
            // value too big, must go in next longword
            if ( isOffset )
            {
                CompileOpcode( OP_INT_VAL );
                *mDictionary.pCurrent++ = value;
                CompileOpcode( OP_PLUS );
            }
            else
            {
                CompileOpcode( OP_INT_VAL );
                *mDictionary.pCurrent++ = value;
            }
        }
    }
    else
    {
        if ( isOffset )
        {
            // add value to top of param stack
            *mpCore->SP += value;
        }
        else
        {
            // leave value on param stack
            *--mpCore->SP = value;
        }
    }
}

// interpret/compile a 64-bit constant
void
ForthEngine::ProcessLongConstant( long long value )
{
    if ( mCompileState )
    {
        // compile the literal value
        CompileOpcode( OP_DOUBLE_VAL );
        long long* pDP = (long long *) mDictionary.pCurrent;
        *pDP++ = value;
        mDictionary.pCurrent = (long *) pDP;
    }
    else
    {
        // leave value on param stack
        mpCore->SP -= 2;
        *(long long *) mpCore->SP = value;
    }
}

// return true IFF the last compiled opcode was an integer literal
bool
ForthEngine::GetLastConstant( long& constantValue )
{
    if ( mpLastCompiledOpcode != NULL )
    {
        long op = *mpLastCompiledOpcode;
        if ( ((mpLastCompiledOpcode + 1) == mDictionary.pCurrent)
            && (FORTH_OP_TYPE( op ) == kOpConstant) )
        {
            constantValue = FORTH_OP_VALUE( op );
            return true;
        }
    }
    return false;
}

//
// ExecuteOneOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
// execute forth ops, and is also how systems external to forth execute ops
//
eForthResult
ForthEngine::ExecuteOneOp( long opCode )
{
    long opScratch[2];

    opScratch[0] = opCode;
    opScratch[1] = BUILTIN_OP( OP_DONE );

	eForthResult exitStatus = ExecuteOps( &(opScratch[0]) );
    return exitStatus;
}

//
// ExecuteOps is used by the Outer Interpreter (ForthEngine::ProcessToken) to
// execute a sequence of forth ops, and is also how systems external to forth execute ops
//
// code at pOps must be terminated with OP_DONE
//
inline eForthResult
ForthEngine::ExecuteOps( long *pOps )
{
    long *savedIP;
    eForthResult exitStatus = kResultOk;

    savedIP = mpCore->IP;
    mpCore->IP = pOps;
#ifdef _ASM_INNER_INTERPRETER
    if ( mFastMode )
    {
        exitStatus = InnerInterpreterFast( mpCore );
    }
    else
#endif
    {
        exitStatus = InnerInterpreter( mpCore );
    }
    mpCore->IP = savedIP;
    if ( exitStatus == kResultDone )
    {
        mpCore->state = kResultOk;
        exitStatus = kResultOk;
    }
    return exitStatus;
}


//
// Use this version of ExecuteOps to execute code in a particular thread
// Caller must have already set the thread IP to point to a sequens of ops which ends with 'done'
//
eForthResult
ForthEngine::ExecuteOps( ForthThread* pThread )
{
    eForthResult exitStatus = kResultOk;

#ifdef _ASM_INNER_INTERPRETER
    if ( mFastMode )
    {
        exitStatus = InnerInterpreterFast( &(pThread->mCore) );
    }
    else
#endif
    {
        exitStatus = InnerInterpreter( &(pThread->mCore) );
    }
    return exitStatus;
}


eForthResult
ForthEngine::ExecuteOneMethod( ForthObject& obj, long methodNum )
{
    long opScratch[2];

	opScratch[0] = obj.pMethodOps[ methodNum ];
    opScratch[1] = BUILTIN_OP( OP_DONE );

	ForthCoreState* pCore = mpCore;
	RPUSH( ((long) GET_TPD) );
    RPUSH( ((long) GET_TPM) );
    SET_TPM( obj.pMethodOps );
    SET_TPD( obj.pData );

	eForthResult exitStatus = ExecuteOps( &(opScratch[0]) );
    return exitStatus;
}


void
ForthEngine::AddErrorText( const char *pString )
{
    strcat_s( mpErrorString, (ERROR_STRING_MAX - 1), pString );
}

void
ForthEngine::SetError( eForthError e, const char *pString )
{
    mpCore->error = e;
    if ( pString )
    {
	    strcat_s( mpErrorString, (ERROR_STRING_MAX - 1), pString );
    }
    if ( e == kForthErrorNone )
    {
        // previous error state is being cleared
        mpErrorString[0] = '\0';
    }
    else
    {
        mpCore->state = kResultError;
    }
}

void
ForthEngine::SetFatalError( eForthError e, const char *pString )
{
    mpCore->state = kResultFatalError;
    mpCore->error = e;
    if ( pString )
    {
        strcpy_s( mpErrorString, (ERROR_STRING_MAX - 1), pString );
    }
}

void
ForthEngine::GetErrorString( char *pBuffer, int bufferSize )
{
    int errorNum = (int) mpCore->error;
    if ( errorNum < (sizeof(pErrorStrings) / sizeof(char *)) )
    {
        if ( mpErrorString[0] != '\0' )
        {
            sprintf_s( pBuffer, bufferSize, "%s: %s", pErrorStrings[errorNum], mpErrorString );
        }
        else
        {
            strcpy_s( pBuffer, bufferSize, pErrorStrings[errorNum] );
        }
    }
    else
    {
        sprintf_s( pBuffer, bufferSize, "Unknown Error %d", errorNum );
    }
}


eForthResult
ForthEngine::CheckStacks( void )
{
    long depth;
    eForthResult result = kResultOk;

    // check parameter stack for over/underflow
    depth = mpCore->ST - mpCore->SP;
    if ( depth < 0 )
    {
        SetError( kForthErrorParamStackUnderflow );
        result = kResultError;
    }
    else if ( depth >= (long) mpCore->SLen )
    {
        SetError( kForthErrorParamStackOverflow );
        result = kResultError;
    }
    
    // check return stack for over/underflow
    depth = mpCore->RT - mpCore->RP;
    if ( depth < 0 )
    {
        SetError( kForthErrorReturnStackUnderflow );
        result = kResultError;
    }
    else if ( depth >= (long) mpCore->RLen )
    {
        SetError( kForthErrorReturnStackOverflow );
        result = kResultError;
    }

    return result;
}


void ForthEngine::SetConsoleOut( consoleOutRoutine outRoutine, void* outData )
{
	mDefaultConsoleOut = outRoutine;
	mpDefaultConsoleOutData = outData;
}

void ForthEngine::ResetConsoleOut( ForthCoreState* pCore )
{
	pCore->consoleOut = mDefaultConsoleOut;
	pCore->pConOutData = mpDefaultConsoleOutData;
}


void ForthEngine::ConsoleOut( const char* pBuff )
{
    mpCore->consoleOut( mpCore, pBuff );
}


long ForthEngine::GetTraceFlags( void )
{
	return mTraceFlags;
}

void ForthEngine::SetTraceFlags( long flags )
{
	mTraceFlags = flags;
}

////////////////////////////
//
// enumerated type support
//
void
ForthEngine::StartEnumDefinition( void )
{
    SetFlag( kEngineFlagInEnumDefinition );
    mNextEnum = 0;
    // remember the stack level at start of definition
    // when another enum is to be defined, if the current stack is above this level,
    // the top element on the stack will be popped and set the current value of the enum
    mpEnumStackBase = mpCore->SP;
}

void
ForthEngine::EndEnumDefinition( void )
{
    ClearFlag( kEngineFlagInEnumDefinition );
}

// return milliseconds since engine was created
unsigned long
ForthEngine::GetElapsedTime( void )
{
#ifdef _WINDOWS
#if 0
	struct _timeb now;
    _ftime( &now );

    long seconds = now.time - mStartTime.time;
    long milliseconds = now.millitm - mStartTime.millitm;
    return (unsigned long) ((seconds * 1000) + milliseconds);
#else
	struct __timeb32 now;

	_ftime32_s( &now );
	__time32_t seconds = now.time - mStartTime.time;
    __time32_t milliseconds = now.millitm - mStartTime.millitm;
    return (unsigned long) ((seconds * 1000) + milliseconds);
#endif
#else
    return 0;
#endif
}

//############################################################################
//
//          O U T E R    I N T E R P R E T E R  (sort of)
//
//############################################################################


// return true to exit forth shell
eForthResult
ForthEngine::ProcessToken( ForthParseInfo   *pInfo )
{
    long *pEntry, value;
    long long lvalue;
    eForthResult exitStatus = kResultOk;
    float fvalue;
    double dvalue;
    char *pToken = pInfo->GetToken();
    int len = pInfo->GetTokenLength();
    bool isAString = (pInfo->GetFlags() & PARSE_FLAG_QUOTED_STRING) != 0;
    bool isSingle, isOffset;
    double* pDPD;
    ForthVocabulary* pFoundVocab = NULL;

    mpLastToken = pToken;
    if ( (pToken == NULL)   ||   ((len == 0) && !isAString) )
    {
        // ignore empty tokens, except for the empty quoted string
        return kResultOk;
    }
    
    SPEW_OUTER_INTERPRETER( "%s [%s] flags[%x]\t", mCompileState ? "Compile" : "Interpret", pToken, pInfo->GetFlags() );
    if ( isAString )
    {
        ////////////////////////////////////
        //
        // symbol is a quoted string - the quotes have already been stripped
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "String[%s] flags[%x]\n", pToken, pInfo->GetFlags() );
        if ( mCompileState )
        {
            int lenLongs = ((len + 4) & ~3) >> 2;
            CompileOpcode( lenLongs | (kOpConstantString << 24) );
            strcpy_s( (char *) mDictionary.pCurrent, (len + 1), pToken );
            mDictionary.pCurrent += lenLongs;
        }
        else
        {
            // in interpret mode, stick the string in string buffer A
            //   and leave the address on param stack
            // this hooha turns mpStringBufferA into NUM_INTERP_STRINGS string buffers
            // so that you can user multiple interpretive string buffers
            if ( mNextStringNum >= NUM_INTERP_STRINGS )
            {
                mNextStringNum = 0;
            }
            char *pStr = mpStringBufferA + (INTERP_STRINGS_LEN * mNextStringNum);
            strcpy_s( pStr, (INTERP_STRINGS_LEN - 1), pToken );  // TBD: make string buffer len a symbol
			pStr[ INTERP_STRINGS_LEN - 1 ] = '\0';
            *--mpCore->SP = (long) pStr;
            mNextStringNum++;
        }
        return kResultOk;
        
    }
    else if ( pInfo->GetFlags() & PARSE_FLAG_QUOTED_CHARACTER )
    {
        ////////////////////////////////////
        //
        // symbol is a quoted character - the quotes have already been stripped
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Character[%s] flags[%x]\n", pToken, pInfo->GetFlags() );
        value = *pToken & 0xFF;
        if ( mCompileState )
        {
            CompileOpcode( value | (kOpConstant << 24) );
        }
        else
        {
            // in interpret mode, stick the character on the stack
            *--mpCore->SP = value;
        }
        return kResultOk;
    }
    
    if ( mpInterpreterExtension != NULL )
    {
        if ( (*mpInterpreterExtension)( pToken ) )
        {
            ////////////////////////////////////
            //
            // symbol was processed by user-defined interpreter extension
            //
            ////////////////////////////////////
            return kResultOk;
        }
    }

    // check for local variables
    if ( mCompileState)
    {
        pEntry = mpLocalVocab->FindSymbol( pInfo );
        if ( pEntry )
        {
            ////////////////////////////////////
            //
            // symbol is a local variable
            //
            ////////////////////////////////////
            SPEW_OUTER_INTERPRETER( "Local variable [%s]\n", pToken );
            CompileOpcode( *pEntry );
            return kResultOk;
        }
    }

    // check for member variables and methods
    if ( mCompileState && CheckFlag( kEngineFlagInClassDefinition ) )
    {
        if ( mpTypesManager->ProcessMemberSymbol( pInfo, exitStatus ) )
        {
            ////////////////////////////////////
            //
            // symbol is a member variable or method
            //
            ////////////////////////////////////
            return exitStatus;
        }
    }

    pEntry = NULL;
    if ( pInfo->GetFlags() & PARSE_FLAG_HAS_COLON )
    {
        if ( (len > 2) && (*pToken != ':') && (pToken[len - 1] != ':') )
        {
            ////////////////////////////////////
            //
            // symbol may be of form VOCABULARY:SYMBOL
            //
            ////////////////////////////////////
            char* pColon = strchr( pToken, ':' );
            *pColon = '\0';
            ForthVocabulary* pVocab = ForthVocabulary::FindVocabulary( pToken );
            if ( pVocab != NULL )
            {
                pEntry = pVocab->FindSymbol( pColon + 1 );
                if ( pEntry != NULL )
                {
                    pFoundVocab = pVocab;
                }
            }
            *pColon = ':';
        }
    }
    if ( pEntry == NULL )
    {
#ifdef MAP_LOOKUP
        pEntry = mpVocabStack->FindSymbol( pToken, &pFoundVocab );
#else
        pEntry = mpVocabStack->FindSymbol( pInfo, &pFoundVocab );
#endif
    }
    if ( pEntry != NULL )
    {
        ////////////////////////////////////
        //
        // symbol is a forth op
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Forth op [%s] in vocabulary %s\n", pToken, pFoundVocab->GetName() );
        return pFoundVocab->ProcessEntry( pEntry );
    }

    // see if this is a structure/object access (like structA.fieldB.fieldC)
    if ( pInfo->GetFlags() & PARSE_FLAG_HAS_PERIOD ) 
    {
        if ( mpTypesManager->ProcessSymbol( pInfo, exitStatus ) )
        {
            ////////////////////////////////////
            //
            // symbol is a structure/object access
            //
            ////////////////////////////////////
            return exitStatus;
        }
    }

	// see if this is an array indexing op like structType[] or number[]
    if ( (len > 2) && (strcmp( "[]", &(pToken[len - 2]) ) == 0) )
    {
		// symbol ends with [], see if preceeding token is either a number or a structure type
		pToken[len - 2] = '\0';
		int elementSize = 0;
        ForthStructVocabulary* pStructVocab = mpTypesManager->GetStructVocabulary( pToken );
        if ( pStructVocab != NULL )
        {
			elementSize = pStructVocab->GetSize();
        }
		else
		{
			ForthNativeType *pNative = mpTypesManager->GetNativeTypeFromName( pToken );
			if ( pNative != NULL )
			{
				// string[] is not supported
				if ( pNative->GetBaseType() != kBaseTypeString )
				{
					elementSize = pNative->GetSize();
				}
			}
			else if ( ScanIntegerToken( pToken, value, lvalue, mpCore->base, isOffset, isSingle ) && isSingle )
			{
				elementSize = value;
			}
		}
		pToken[len - 2] = '[';
		if ( elementSize > 0 )
		{
			// compile or execute 
			if ( mCompileState )
			{
				CompileOpcode( elementSize | (kOpArrayOffset << 24) );
			}
			else
			{
				// in interpret mode, stick the result of array indexing on the stack
				value = *mpCore->SP++;		// get base address
				*mpCore->SP = value + (elementSize * (*mpCore->SP));
			}
			return kResultOk;
		}
    }

    // try to convert to a number
    // if there is a period in string
    //    try to covert to a floating point number
    // else
    //    try to convert to an integer
    if ( (pInfo->GetFlags() & PARSE_FLAG_HAS_PERIOD) 
          && ScanFloatToken( pToken, fvalue, dvalue, isSingle ) )
    {
       if ( isSingle )
       {
          ////////////////////////////////////
          //
          // symbol is a single precision fp literal
          //
          ////////////////////////////////////
          SPEW_OUTER_INTERPRETER( "Floating point literal %f\n", fvalue );
          if ( mCompileState )
          {
              // compile the literal value
              CompileOpcode( OP_FLOAT_VAL );
              *(float *) mDictionary.pCurrent++ = fvalue;
          }
          else
          {
              --mpCore->SP;
              *(float *) mpCore->SP = fvalue;
          }
       }
       else
       {
          ////////////////////////////////////
          //
          // symbol is a double precision fp literal
          //
          ////////////////////////////////////
          SPEW_OUTER_INTERPRETER( "Floating point double literal %g\n", dvalue );
          if ( mCompileState )
          {
              // compile the literal value
              CompileOpcode( OP_DOUBLE_VAL );
              pDPD = (double *) mDictionary.pCurrent;
              *pDPD++ = dvalue;
              mDictionary.pCurrent = (long *) pDPD;
          }
          else
          {
              mpCore->SP -= 2;
              *(double *) mpCore->SP = dvalue;
          }
       }
        
    }
    else if ( ScanIntegerToken( pToken, value, lvalue, mpCore->base, isOffset, isSingle ) )
    {

        ////////////////////////////////////
        //
        // symbol is an integer literal
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Integer literal %d\n", value );
        if ( isSingle )
        {
            ProcessConstant( value, isOffset );
        }
        else
        {
            ProcessLongConstant( lvalue );
        }
    }
    else if ( CheckFlag( kEngineFlagInEnumDefinition ) )
    {
        // add symbol as an enumerated value
        if ( mpCore->SP < mpEnumStackBase )
        {
            // pop enum value off stack
            mNextEnum = *mpCore->SP++;
        }
        if ( (mNextEnum < (1 << 23)) && (mNextEnum >= -(1 << 23)) )
        {
            // number is in range supported by kOpConstant, just add it to vocabulary
            mpDefinitionVocab->AddSymbol( pToken, kOpConstant, mNextEnum & 0x00FFFFFF, false );
        }
        else
        {
            // number is out of range of kOpConstant, need to define a user op
            StartOpDefinition( pToken );
            CompileOpcode( OP_DO_CONSTANT );
            CompileLong( mNextEnum );
        }
        mNextEnum++;
    }
    else
    {
        TRACE( "Unknown symbol %s\n", pToken );
        mpCore->error = kForthErrorUnknownSymbol;
        exitStatus = kResultError;
    }

    // TBD: return exit-shell flag
    return exitStatus;
}



//############################################################################
