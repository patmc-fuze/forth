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
#include "ForthClass.h"
#include "ForthInner.h"

extern "C" {

    extern baseDictEntry baseDict[];
    extern void AddBuiltinClasses( ForthEngine* pEngine );
    extern InitAsmTables(  ForthCoreState *pCore );
    extern eForthResult InnerInterp( ForthCoreState *pCore );
    extern void consoleOutToFile( ForthCoreState   *pCore,  const char       *pMessage );
};

//#ifdef TRACE_INNER_INTERPRETER

// provide trace ability for builtin ops
#define NUM_TRACEABLE_OPS MAX_BUILTIN_OPS
static char *gOpNames[ NUM_TRACEABLE_OPS ];

//#endif

ForthEngine* ForthEngine::mpInstance = NULL;

#define ERROR_STRING_MAX    256

///////////////////////////////////////////////////////////////////////
//
// opTypeNames must be kept in sync with forthOpType enum in forth.h
//

static char *opTypeNames[] = {
    "BuiltIn", "BuiltInImmediate", "UserDefined", "UserDefinedImmediate", "UserCode", "UserCodeImmediate", "DLLEntryPoint", 0, 0, 0,
    "Branch", "BranchTrue", "BranchFalse", "CaseBranch", 0, 0, 0, 0, 0, 0,
    "Constant", "ConstantString", "Offset", "ArrayOffset", "AllocLocals", "LocalRef", "InitLocalString", "LocalStructArray", "OffsetFetch", 0,
    "LocalByte", "LocalShort", "LocalInt", "LocalFloat", "LocalDouble", "LocalString", "LocalOp", "LocalObject", 0, 0,
    "FieldByte", "FieldShort", "FieldInt", "FieldFloat", "FieldDouble", "FieldString", "FieldOp", "FieldObject", 0, 0,
    "LocalByteArray", "LocalShortArray", "LocalIntArray", "LocalFloatArray", "LocalDoubleArray", "LocalStringArray", "LocalOpArray", "LocalObjectArray", 0, 0,
    "FieldByteArray", "FieldShortArray", "FieldIntArray", "FieldFloatArray", "FieldDoubleArray", "FieldStringArray", "FieldOpArray", "FieldObjectArray", 0, 0,
    "MemberByte", "MemberShort", "MemberInt", "MemberFloat", "MemberDouble", "MemberString", "MemberOp", "MemberObject", 0, 0,
    "MemberByteArray", "MemberShortArray", "MemberIntArray", "MemberFloatArray", "MemberDoubleArray", "MemberStringArray", "MemberOpArray", "MemberObjectArray", 0, 0,
    "MethodWithThis", "MethodWithTOS", "InitMemberString", 0, 0, 0, 0, 0, 0, 0,
    "LocalUserDefined"
};

///////////////////////////////////////////////////////////////////////
//
// pErrorStrings must be kept in sync with eForthError enum in forth.h
//
static char *pErrorStrings[] =
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
};

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
, mpCurrentThread( NULL )
, mFastMode( false )
, mpLastCompiledOpcode( NULL )
, mpLastIntoOpcode( NULL )
, mNumElements( 0 )
, mpStructsManager( NULL )
, mpVocabStack( NULL )
, mDefaultConsoleOut( consoleOutToFile )
, mpDefaultConsoleOutData( stdout )
{
    // scratch area for temporary definitions
    ASSERT( mpInstance == NULL );
    mpInstance = this;
    mpEngineScratch = new long[70];
    mpCore = new ForthCoreState;
    InitCore( mpCore );
    mpErrorString = new char[ ERROR_STRING_MAX + 1 ];

    // remember creation time for elapsed time method
    _ftime( &mStartTime );
}

ForthEngine::~ForthEngine()
{
    ForthThread *pNextThread;

    if ( mpCore->DBase ) {
        delete [] mpCore->DBase;
        delete mpForthVocab;
        delete mpLocalVocab;
        delete mpStructsManager;
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
    if ( mpCore->builtinOps )
    {
        free( mpCore->builtinOps );
    }
    if ( mpCore->userOps )
    {
        free( mpCore->userOps );
    }

    // delete all threads;
    while ( mpThreads != NULL ) {
        pNextThread = mpThreads->mpNext;
        delete [] mpThreads;
        mpThreads = pNextThread;
    }
    delete mpEngineScratch;

    delete mpCore;
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

#define NUM_INTERP_STRINGS 8

void
ForthEngine::Initialize( int                totalLongs,
                         bool               bAddBaseOps,
                         baseDictEntry *    pUserBuiltinOps )
{
    mpCore->DBase = new long[totalLongs];
    mpCore->DP = mpCore->DBase;
    mpCore->pEngine = this;

    mpForthVocab = new ForthVocabulary( "forth", NUM_FORTH_VOCAB_VALUE_LONGS );
    mpLocalVocab = new ForthLocalVocabulary( "locals", NUM_LOCALS_VOCAB_VALUE_LONGS );
    mpStringBufferA = new char[256 * NUM_INTERP_STRINGS];
    mpStringBufferB = new char[TMP_STRING_BUFFER_LEN];

    mpCore->numBuiltinOps = 0;
    mpCore->builtinOps = (ForthOp *) malloc( sizeof(ForthOp) * MAX_BUILTIN_OPS );
    mpCore->numUserOps = 0;
    mpCore->maxUserOps = 128;
    mpCore->userOps = (long **) malloc( sizeof(long *) * mpCore->maxUserOps );

    mpStructsManager = new ForthTypesManager();

    mpVocabStack = new ForthVocabularyStack;
    mpVocabStack->Initialize();
    mpVocabStack->Clear();

    Reset();

    //
    // build dispatch table for different opcode types
    //
    InitDispatchTables( mpCore );

    if ( bAddBaseOps ) {
        AddBuiltinOps( baseDict );
        AddBuiltinClasses( this );
    }
    if ( pUserBuiltinOps != NULL ) {
        AddBuiltinOps( pUserBuiltinOps );
    }
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
    int i;

    if ( goFast )
    {
        InitAsmTables( mpCore );
    }
    else
    {
        InitDispatchTables( mpCore );
        for ( i = 0; baseDict[i].value != NULL; i++ )
        {
            mpCore->builtinOps[i] = (ForthOp) (baseDict[i].value);
        }
    }
    mFastMode = goFast;
}

void
ForthEngine::Reset( void )
{
    mpDefinitionVocab = mpForthVocab;
    mpVocabStack->Clear();

    mNextStringNum = 0;

    mpLastCompiledOpcode = NULL;
    mpLastIntoOpcode = NULL;
    mCompileState = 0;
    mCompileFlags = 0;
}

void
ForthEngine::ErrorReset( void )
{
    Reset();
	mpCurrentThread->Reset();
    mpCurrentThread->Activate( mpCore );
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
        if ( mpCore->numUserOps == mpCore->maxUserOps ) {
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
    mpDefinitionVocab->AddSymbol( pSymbol, kOpUserDef, (long) mpCore->DP, true );
    if ( smudgeIt ) {
        mpDefinitionVocab->SmudgeNewestSymbol();
    }

    return mpCore->numUserOps - 1;
}


void
ForthEngine::AddBuiltinOps( baseDictEntry *pEntries )
{
    // assert if this is called after any user ops have been defined
    ASSERT( mpCore->numUserOps == 0 );

    while ( pEntries->value != NULL ) {
        // AddSymbol will call ForthEngine::AddOp to add the operators to op table
        mpDefinitionVocab->AddSymbol( pEntries->name, pEntries->flags, pEntries->value, true );

#ifdef TRACE_INNER_INTERPRETER
        // add built-in op names to table for TraceOp
        if ( (pEntries->flags == kOpBuiltIn) || ((pEntries->flags == kOpBuiltInImmediate)) ) {
            if ( (mpCore->numBuiltinOps - 1) < NUM_TRACEABLE_OPS ) {
                gOpNames[mpCore->numBuiltinOps - 1] = pEntries->name;
            }
        }
#endif
        pEntries++;

    }
}


ForthClassVocabulary*
ForthEngine::AddBuiltinClass( const char* pClassName, ForthClassVocabulary* pParentClass, baseDictEntry *pEntries )
{
    // do "class:" - define class subroutine
    SetFlag( kEngineFlagInStructDefinition );
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->StartClassDefinition( GetNextSimpleToken() );
    CompileOpcode( OP_DO_CLASS_TYPE );
    CompileLong( (long) pVocab );

#if 0
    if ( pParentClass )
    {
        // do "extends" - tie into parent class
        pManager->GetNewestStruct()->Extends( pParentClass );
    }

    // loop through pEntries, adding ops to builtinOps table and adding methods to class
    while ( pEntries->value != NULL ) {
        // add method subroutine to builtinOps table

        // do "method:"
        const char* pMethodName = pEntries->name;
        StartOpDefinition( pMethodName, false );
        // switch to compile mode
        long* pEntry = pVocab->GetNewestEntry();
        // pEntry[0] is initially the opcode for the method, now we replace it with the method index,
        //  and put the opcode in the method table
        long methodIndex = pVocab->AddMethod( pMethodName, pEntry[0] );
        pEntry[0] = methodIndex;
        pEntry[1] |= kDTIsMethod;
        // TBD: support method return values (structs or objects)

        // do ";method"
        EndOpDefinition( false );

#ifdef TRACE_INNER_INTERPRETER
        // add built-in op names to table for TraceOp
        if ( (mpCore->numBuiltinOps - 1) < NUM_TRACEABLE_OPS ) {
            gOpNames[mpCore->numBuiltinOps - 1] = pEntries->name;
        }
#endif
        pEntries++;

    }

    // do ";class"
    ClearFlag( kEngineFlagInStructDefinition );
    pManager->EndClassDefinition();

#endif
    return pVocab;
}


// forget the specified op and all higher numbered ops, and free the memory where those ops were stored
void
ForthEngine::ForgetOp( ulong opNumber )
{
    if ( opNumber < mpCore->numUserOps ) {
        mpCore->DP = mpCore->userOps[ opNumber ];
        mpCore->numUserOps = opNumber;
    } else {
        TRACE( "ForthEngine::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, mpCore->numUserOps );
    }
}

// return true if symbol was found
bool
ForthEngine::ForgetSymbol( const char *pSym )
{
    long *pEntry = NULL;
    long op;
    forthOpType opType;
    bool forgotIt = false;
    char buff[256];

    ForthVocabulary* pFoundVocab = NULL;
    pEntry = GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );

    if ( pFoundVocab != NULL ) {
        op = ForthVocabulary::GetEntryValue( pEntry );
        opType = ForthVocabulary::GetEntryType( pEntry );
        switch ( opType )
        {
            case kOpBuiltIn:
            case kOpBuiltInImmediate:
                // sym is built-in op - no way
                TRACE( "Error - attempt to forget builtin op %s from %s\n", pSym, pFoundVocab->GetName() );
                sprintf( buff, "Error - attempt to forget builtin op %s from %s\n", pSym, pFoundVocab->GetName() );
                break;

            case kOpUserDef:
            case kOpUserDefImmediate:
            case kOpUserCode:
            case kOpUserCodeImmediate:
                ForgetOp( op );
                ForthForgettable::ForgetPropagate( mpCore->DP, op );
                forgotIt = true;
                break;

            default:
                const char* pStr = GetOpTypeName( opType );
                TRACE( "Error - attempt to forget op %s of type %s from %s\n", pSym, pStr, pFoundVocab->GetName() );
                sprintf( buff, "Error - attempt to forget op %s of type %s from %s\n", pSym, pStr, pFoundVocab->GetName() );
                break;

        }
    }
    else
    {
        TRACE( "Error - attempt to forget unknown op %s from %s\n", pSym, GetSearchVocabulary()->GetName() );
    }
    return forgotIt;
}

ForthThread *
ForthEngine::CreateThread( int paramStackSize, int returnStackSize, long *pInitialIP )
{
    ForthThread *pNewThread = new ForthThread( this, paramStackSize, returnStackSize );
    pNewThread->SetIP( pInitialIP );

    pNewThread->mpNext = mpThreads;
    mpThreads = pNewThread;

    return pNewThread;
}


void
ForthEngine::DestroyThread( ForthThread *pThread )
{
    ForthThread *pNext, *pCurrent;

    if ( mpThreads == pThread ) {

        // special case - thread is head of list
        mpThreads = mpThreads->mpNext;
        delete pThread;

    } else {

        // TBD: this is untested
        pCurrent = mpThreads;
        while ( pCurrent != NULL ) {
            pNext = pCurrent->mpNext;
            if ( pThread == pNext ) {
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


void
ForthEngine::PushInputFile( FILE *pInFile )
{
    mpShell->PushInputFile( pInFile );
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
    long* pEntry = mpDefinitionVocab->AddSymbol( pName, opType, (long) mpCore->DP, true );
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
    char c;
    int line = 1;
    bool notDone = true;

    ForthVocabulary* pFoundVocab = NULL;
    pEntry = GetVocabularyStack()->FindSymbol( pSymName, &pFoundVocab );
    if ( pEntry )
    {
        long opType = FORTH_OP_TYPE( pEntry[0] );
        long opValue = FORTH_OP_VALUE( pEntry[0] );
        const char* pStr = GetOpTypeName( opType );
        sprintf( buff, "%s: type %s:%x value 0x%x 0x%x \n", pSymName, pStr, opValue, pEntry[0], pEntry[1] );
        ConsoleOut( buff );
        if ( opType == kOpUserDef )
        {
            // disassemble the op until IP reaches next newer op
            long* curIP = mpCore->userOps[ opValue ];
            long* endIP = (opValue == (mpCore->numUserOps - 1)) ? GetDP() : mpCore->userOps[ opValue + 1 ];
            while ( (curIP < endIP) && notDone )
            {
                sprintf( buff, "  %08x  ", curIP );
                ConsoleOut( buff );
                DescribeOp( curIP, buff, true );
                ConsoleOut( buff );
                sprintf( buff, "\n" );
                ConsoleOut( buff );
                if ( ((line & 31) == 0) && (mpShell != NULL) && mpShell->GetInput()->InputStream()->IsInteractive() )
                {
                    ConsoleOut( "Hit ENTER to continue, 'q' & ENTER to quit\n" );
                    c = getchar();
                    if ( (c == 'q') || (c == 'Q') ) {
                        c = getchar();
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
        sprintf( buff, "Symbol %s not found\n", pSymName );
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
    if ( mpLocalAllocOp == NULL ) {
        // this is first local var definition, leave space for local alloc op
        mCompileFlags |= kEngineFlagHasLocalVars;
        mpLocalAllocOp = mpCore->DP;
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
    if ( mpLocalAllocOp == NULL ) {
        // this is first local var definition, leave space for local alloc op
        mCompileFlags |= kEngineFlagHasLocalVars;
        mpLocalAllocOp = mpCore->DP;
        CompileLong( 0 );
    }
    long elementType = CODE_TO_BASE_TYPE( typeCode );
    if ( elementType != kBaseTypeStruct )
    {
        // array of non-struct
        long arraySize = elementSize * mNumElements;
        arraySize = ((arraySize + 3) & ~3) >> 2;
        mLocalFrameSize += arraySize;
        pEntry = mpLocalVocab->AddSymbol( pArrayName, (CODE_IS_PTR(typeCode) ? kOpLocalIntArray : (kOpLocalByteArray + elementType)), mLocalFrameSize, false );
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
    pEntry[1] = typeCode;

    mNumElements = 0;
    return mLocalFrameSize;
}

#if 0
void
ForthEngine::InvokeClassMethod( ForthThread     *g,
                                ulong           methodNumber )
{
    ForthClassDescriptor *pDesc;
    ulong opVal;
    forthOpType opType;
    ForthEngine *pEngine = g->mpEngine;

    // top of stack is pointer to object instance
    // first field in object instance is pointer to its class descriptor
    pDesc = *(ForthClassDescriptor **) (*(g->GetSP()));
    if ( methodNumber < pDesc->numMethods ) {
        opVal = pDesc->pOpTable[ methodNumber ];
        opType = FORTH_OP_TYPE( opVal );
        opVal = FORTH_OP_VALUE( opVal );

        if ( opVal == kOpBuiltIn ) {
            if ( opVal < pEngine->mpCore->numBuiltinOps ) {
                ((ForthOp) pEngine->mpCore->builtinOps[opVal]) ( g );
            } else {
                g->SetError( kForthErrorBadOpcode );
            }
        } else if ( opVal == kOpUserDef ) {
            if ( opVal < pEngine->mpCore->numUserOps ) {
                g->RPush( (long) g->GetIP() );
                g->SetIP( pEngine->mpCore->userOps[opVal] );
            } else {
                g->SetError( kForthErrorBadOpcode );
            }
        } else {
            g->SetError( kForthErrorBadOpcode );
        }

    } else {
        g->SetError( kForthErrorBadMethod );
    }
}

bool
ForthEngine::IsExecutableType( forthOpType      symType )
{
    switch( symType ) {

    case kOpBuiltIn:
    case kOpUserDef:
        return true;

    default:
        return false;

    }
}
#endif


// TBD: tracing of built-in ops won't work for user-added builtins...
const char *
ForthEngine::GetOpTypeName( long opType )
{
    return (opType < kOpLocalUserDefined) ? opTypeNames[opType] : "unknown";
}

static bool lookupUserTraces = true;

void
ForthEngine::TraceOp( void )
{
#ifdef TRACE_INNER_INTERPRETER
    long *pOp = mpCore->IP;
    char buff[ 256 ];
    int rDepth = mpCore->RT - mpCore->RP;
    if ( rDepth > 8 )
    {
        rDepth = 8;
    }
    char* sixteenSpaces = "                ";     // 16 spaces
    char* pIndent = sixteenSpaces + (16 - (rDepth << 1));
    if ( *pOp != OP_DONE )
    {
        DescribeOp( pOp, buff, lookupUserTraces );
        TRACE( "# 0x%08x %s%s\n", pOp, pIndent, buff );
    }
#endif
}

void
ForthEngine::DescribeOp( long *pOp, char *pBuffer, bool lookupUserDefs )
{
    long op = *pOp;
    forthOpType opType = FORTH_OP_TYPE( op );
    ulong opVal = FORTH_OP_VALUE( op );
    ForthVocabulary *pFoundVocab = NULL;
    long *pEntry = NULL;

    sprintf( pBuffer, "%02x:%06x    ", opType, opVal );
    pBuffer += 13;
    if ( opType >= (sizeof(opTypeNames) / sizeof(char *)) )
    {
        sprintf( pBuffer, "BadOpType" );
    }
    else
    {

        switch( opType )
        {

            case kOpBuiltIn:
            case kOpBuiltInImmediate:
                if ( opVal < NUM_TRACEABLE_OPS ) {
                    // traceable built-in op
                    switch( opVal )
                    {
                        case OP_INT_VAL:
                            sprintf( pBuffer, "%s 0x%x", gOpNames[opVal], pOp[1] );
                            break;

                        case OP_FLOAT_VAL:
                            sprintf( pBuffer, "%s %f", gOpNames[opVal], *((float *)(&(pOp[1]))) );
                            break;

                        case OP_DOUBLE_VAL:
                            sprintf( pBuffer, "%s %g", gOpNames[opVal], *((double *)(&(pOp[1]))) );
                            break;

                        default:
                            sprintf( pBuffer, "%s", gOpNames[opVal] );
                            break;
                    }
                } else {
                    // op we don't have name pointer for
                    sprintf( pBuffer, "%s", opTypeNames[opType] );
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
                    sprintf( pBuffer, "%s", opTypeNames[opType] );
                }
                break;

            case kOpLocalByte:          case kOpLocalByteArray:
            case kOpLocalShort:         case kOpLocalShortArray:
            case kOpLocalInt:           case kOpLocalIntArray:
            case kOpLocalFloat:         case kOpLocalFloatArray:
            case kOpLocalDouble:        case kOpLocalDoubleArray:
            case kOpLocalString:        case kOpLocalStringArray:
            case kOpLocalOp:            case kOpLocalOpArray:
            case kOpLocalObject:        case kOpLocalObjectArray:
            case kOpMemberByte:          case kOpMemberByteArray:
            case kOpMemberShort:         case kOpMemberShortArray:
            case kOpMemberInt:           case kOpMemberIntArray:
            case kOpMemberFloat:         case kOpMemberFloatArray:
            case kOpMemberDouble:        case kOpMemberDoubleArray:
            case kOpMemberString:        case kOpMemberStringArray:
            case kOpMemberOp:            case kOpMemberOpArray:
            case kOpMemberObject:        case kOpMemberObjectArray:
                sprintf( pBuffer, "%s_%x", opTypeNames[opType], opVal );
                break;

                sprintf( pBuffer, "%s_%x", opTypeNames[opType], opVal );
                break;

            case kOpConstantString:
                sprintf( pBuffer, "\"%s\"", (char *)(pOp + 1) );
                break;
            
            case kOpConstant:
                if ( opVal & 0x800000 ) {
                    opVal |= 0xFF000000;
                }
                sprintf( pBuffer, "%s    %d", opTypeNames[opType], opVal );
                break;

            case kOpOffset:
                if ( opVal & 0x800000 )
                {
                    opVal |= 0xFF000000;
                    sprintf( pBuffer, "%s    %d", opTypeNames[opType], opVal );
                }
                else
                {
                    sprintf( pBuffer, "%s    +%d", opTypeNames[opType], opVal );
                }
                break;

            case kOpCaseBranch:
            case kOpBranch:   case kOpBranchNZ:  case kOpBranchZ:
                if ( opVal & 0x800000 ) {
                    opVal |= 0xFF000000;
                }
                sprintf( pBuffer, "%s    0x%08x", opTypeNames[opType], opVal + 1 + pOp );
                break;

            case kOpInitLocalString:   // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
            case kOpInitMemberString:   // bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
                sprintf( pBuffer, "%s    maxBytes %d offset %d", opTypeNames[opType], opVal & 0xFFF, opVal >> 12 );
                break;
            
            case kOpLocalStructArray:   // bits 0..11 are padded struct size in bytes, bits 12..23 are frame offset in longs
                sprintf( pBuffer, "%s    elementSize %d offset %d", opTypeNames[opType], opVal & 0xFFF, opVal >> 12 );
                break;
            
            case kOpAllocLocals:
                sprintf( pBuffer, "%s    longs %d", opTypeNames[opType], opVal );
                break;
            
            case kOpArrayOffset:
                sprintf( pBuffer, "%s    elementSize %d", opTypeNames[opType], opVal );
                break;
            
            case kOpMethodWithThis:
            case kOpMethodWithTOS:
                sprintf( pBuffer, "%s    %d", opTypeNames[opType], opVal );
                break;

            default:
                if ( opType >= (sizeof(opTypeNames) / sizeof(char *)) )
                {
                    sprintf( pBuffer, "BAD OPTYPE!" );
                }
                else
                {
                    sprintf( pBuffer, "%s", opTypeNames[opType] );
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
                               long         *pValue,
                               int          base,
                               bool         &isOffset )
{
    long value, digit;
    bool isNegative;
    char c;
    int digitsFound = 0;
    int len = strlen( pToken );
    char *pLastChar = pToken + (len - 1);
    char lastChar = *pLastChar;

    isOffset = false;

    // handle leading plus or minus sign
    isNegative = (pToken[0] == '-');
    if ( isNegative || (pToken[0] == '+') ) {
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
    else
    {
        pLastChar = NULL;
    }

    if ( (pToken[0] == '0') && (pToken[1] == 'x') ) {
        if ( sscanf( pToken + 2, "%x", &value ) == 1 ) {
            *pValue = (isNegative) ? 0 - value : value;
            return true;
        }
    }

    value = 0;
    while ( (c = *pToken++) != 0 ) {

        if ( (c >= '0') && (c <= '9') ) {
            digit = c - '0';
            digitsFound++;
        } else if ( (c >= 'A') && (c <= 'Z') ) {
            digit = 10 + (c - 'A');
            digitsFound++;
        } else if ( (c >= 'a') && (c <= 'z') ) {
            digit = 10 + (c - 'a');
            digitsFound++;
        } else {
            // char can't be a digit
            return false;
        }

        if ( digit >= base ) {
            // invalid digit for current base
            return false;
        }

        value = (value * base) + digit;
    }

    if ( digitsFound == 0 )
    {
        return false;
    }

    // all chars were valid digits
    *pValue = (isNegative) ? 0 - value : value;

    // restore original last char
    if ( pLastChar != NULL )
    {
        *pLastChar = lastChar;
    }
    return true;
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
   if ( *pLastChar == 'd' )
   {
      *pLastChar = 0;
      if ( sscanf( pToken, "%Lf", &dvalue ) == 1)
      {
         retVal = true;
         isSingle = false;
      }
      *pLastChar = 'd';
   }
   else if ( *pLastChar == 'f' )
   {
      *pLastChar = 0;
      if ( sscanf( pToken, "%f", &fvalue ) == 1)
      {
         retVal = true;
         isSingle = true;
      }
      *pLastChar = 'f';
   }
   else if ( sscanf( pToken, "%f", &fvalue ) == 1)
   {
      retVal = true;
      isSingle = true;
   }

   return retVal;
}


// compile an opcode
// remember the last opcode compiled so someday we can do optimizations
//   like combining "->" followed by a local var name into one opcode
void
ForthEngine::CompileOpcode( long op )
{
    mpLastCompiledOpcode = mpCore->DP;
    if ( op == OP_INTO )
    {
       // we need this to support initialization of local string vars (ugh)
       mpLastIntoOpcode = mpLastCompiledOpcode;
       
    }
    *mpCore->DP++ = op;
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
        mpCore->DP = mpLastCompiledOpcode;
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
    if ( mCompileState ) {
        // compile the literal value
        if ( (value < (1 << 23)) && (value >= -(1 << 23)) ) {
            // value fits in opcode immediate field
            if ( isOffset )
            {
                CompileOpcode( (value & 0xFFFFFF) | (kOpOffset << 24) );
            }
            else
            {
                CompileOpcode( (value & 0xFFFFFF) | (kOpConstant << 24) );
            }
        } else {
            // value too big, must go in next longword
            if ( isOffset )
            {
                CompileOpcode( OP_INT_VAL );
                *mpCore->DP++ = value;
                CompileOpcode( OP_PLUS );
            }
            else
            {
                CompileOpcode( OP_INT_VAL );
                *mpCore->DP++ = value;
            }
        }
    } else {
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

// return true IFF the last compiled opcode was an integer literal
bool
ForthEngine::GetLastConstant( long& constantValue )
{
    if ( mpLastCompiledOpcode != NULL )
    {
        long op = *mpLastCompiledOpcode;
        if ( ((mpLastCompiledOpcode + 1) == mpCore->DP)
            && (FORTH_OP_TYPE( op ) == kOpConstant) )
        {
            constantValue = FORTH_OP_VALUE( op );
            return true;
        }
    }
    return false;
}

void
ForthEngine::SetCurrentThread( ForthThread* pThread )
{
    if ( mpCurrentThread != NULL )
    {
        mpCurrentThread->Deactivate( mpCore );
    }
    mpCurrentThread = pThread;
    mpCurrentThread->Activate( mpCore );
}

//
// ExecuteOneOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
// execute forth ops, and is also how systems external to forth execute ops
//
eForthResult
ForthEngine::ExecuteOneOp( long opCode )
{
    long opScratch[2];
    eForthResult exitStatus = kResultOk;

    opScratch[0] = opCode;
    opScratch[1] = BUILTIN_OP( OP_DONE );

    return ExecuteOps( &(opScratch[0]) );
}

//
// ExecuteOps is used by the Outer Interpreter (ForthEngine::ProcessToken) to
// execute a sequence of forth ops, and is also how systems external to forth execute ops
//
// code at pOps must be terminated with OP_DONE
//
eForthResult
ForthEngine::ExecuteOps( long *pOps )
{
    long *savedIP;
    eForthResult exitStatus = kResultOk;

    savedIP = mpCore->IP;
    mpCore->IP = pOps;
    if ( mFastMode )
    {
        exitStatus = InnerInterpreterFast( mpCore );
    }
    else
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

void
ForthEngine::AddErrorText( const char *pString )
{
    strncat( mpErrorString, pString, ERROR_STRING_MAX );
}

void
ForthEngine::SetError( eForthError e, const char *pString )
{
    mpCore->error = e;
    if ( pString )
    {
        strncat( mpErrorString, pString, ERROR_STRING_MAX );
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
        strncpy( mpErrorString, pString, ERROR_STRING_MAX );
    }
}

void
ForthEngine::GetErrorString( char *pString )
{
    int errorNum = (int) mpCore->error;
    if ( errorNum < (sizeof(pErrorStrings) / sizeof(char *)) ) {
        if ( mpErrorString[0] != '\0' )
        {
            sprintf( pString, "%s: %s", pErrorStrings[errorNum], mpErrorString );
        }
        else
        {
            strcpy( pString, pErrorStrings[errorNum] );
        }
    }
    else
    {
        sprintf( pString, "Unknown Error %d", errorNum );
    }
}


eForthResult
ForthEngine::CheckStacks( void )
{
    long depth;
    eForthResult result = kResultOk;

    // check parameter stack for over/underflow
    depth = mpCore->ST - mpCore->SP;
    if ( depth < 0 ) {
        SetError( kForthErrorParamStackUnderflow );
        result = kResultError;
    } else if ( depth >= (long) mpCore->pThread->SLen ) {
        SetError( kForthErrorParamStackOverflow );
        result = kResultError;
    }
    
    // check return stack for over/underflow
    depth = mpCore->RT - mpCore->RP;
    if ( depth < 0 ) {
        SetError( kForthErrorReturnStackUnderflow );
        result = kResultError;
    } else if ( depth >= (long) mpCore->pThread->RLen ) {
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

void ForthEngine::ResetConsoleOut( ForthThreadState* pThread )
{
	pThread->consoleOut = mDefaultConsoleOut;
	pThread->pConOutData = mpDefaultConsoleOutData;
}


void ForthEngine::ConsoleOut( const char* pBuff )
{
    mpCore->pThread->consoleOut( mpCore, pBuff );
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
    struct _timeb now;
    _ftime( &now );

    long seconds = now.time - mStartTime.time;
    long milliseconds = now.millitm - mStartTime.millitm;
    return (unsigned long) ((seconds * 1000) + milliseconds);
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
    if ( (pToken == NULL)
        || ((len == 0) && !isAString) ) {       // ignore empty tokens, except for the empty quoted string
        return kResultOk;
    }
    
    SPEW_OUTER_INTERPRETER( "%s [%s] flags[%x]\t", mCompileState ? "Compile" : "Interpret", pToken, pInfo->GetFlags() );
    if ( isAString ) {
        ////////////////////////////////////
        //
        // symbol is a quoted string - the quotes have already been stripped
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "String[%s] flags[%x]\n", pToken, pInfo->GetFlags() );
        if ( mCompileState ) {
            len = ((len + 4) & ~3) >> 2;
            CompileOpcode( len | (kOpConstantString << 24) );
            strcpy( (char *) mpCore->DP, pToken );
            mpCore->DP += len;
        } else {
            // in interpret mode, stick the string in string buffer A
            //   and leave the address on param stack
            // this hooha turns mpStringBufferA into NUM_INTERP_STRINGS string buffers
            // so that you can user multiple interpretive string buffers
            if ( mNextStringNum >= NUM_INTERP_STRINGS ) {
                mNextStringNum = 0;
            }
            char *pStr = mpStringBufferA + (256 * mNextStringNum);
            strncpy( pStr, pToken, 255 );  // TBD: make string buffer len a symbol
            *--mpCore->SP = (long) pStr;
            mNextStringNum++;
        }
        return kResultOk;
        
    } else if ( pInfo->GetFlags() & PARSE_FLAG_QUOTED_CHARACTER ) {
        ////////////////////////////////////
        //
        // symbol is a quoted character - the quotes have already been stripped
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Character[%s] flags[%x]\n", pToken, pInfo->GetFlags() );
        value = *pToken & 0xFF;
        if ( mCompileState ) {
            CompileOpcode( value | (kOpConstant << 24) );
        } else {
            // in interpret mode, stick the character on the stack
            *--mpCore->SP = value;
        }
        return kResultOk;
    }
    
    if ( mpInterpreterExtension != NULL ) {
        if ( (*mpInterpreterExtension)( pToken ) ) {
            ////////////////////////////////////
            //
            // symbol was processed by user-defined interpreter extension
            //
            ////////////////////////////////////
            return kResultOk;
        }
    }

    // check for local variables
    if ( mCompileState) {
        pEntry = mpLocalVocab->FindSymbol( pInfo );
        if ( pEntry ) {
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
    if ( mCompileState && CheckFlag( kEngineFlagIsMethod ) )
    {
        if ( mpStructsManager->ProcessMemberSymbol( pInfo, exitStatus ) )
        {
            ////////////////////////////////////
            //
            // symbol is a member variable or method
            //
            ////////////////////////////////////
            return exitStatus;
        }
    }

#ifdef MAP_LOOKUP
    pEntry = mpVocabStack->FindSymbol( pToken, &pFoundVocab );
#else
    pEntry = mpVocabStack->FindSymbol( pInfo, &pFoundVocab );
#endif
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
        if ( mpStructsManager->ProcessSymbol( pInfo, exitStatus ) )
        {
            ////////////////////////////////////
            //
            // symbol is a structure/object access
            //
            ////////////////////////////////////
            return exitStatus;
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
          if ( mCompileState ) {
              // compile the literal value
              CompileOpcode( OP_FLOAT_VAL );
              *(float *) mpCore->DP++ = fvalue;
          } else {
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
          if ( mCompileState ) {
              // compile the literal value
              CompileOpcode( OP_DOUBLE_VAL );
              pDPD = (double *) mpCore->DP;
              *pDPD++ = dvalue;
              mpCore->DP = (long *) pDPD;
          } else {
              mpCore->SP -= 2;
              *(double *) mpCore->SP = dvalue;
          }
       }
        
    } else if ( ScanIntegerToken( pToken, &value, mpCore->pThread->base, isOffset ) ) {

        ////////////////////////////////////
        //
        // symbol is an integer literal
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Integer literal %d\n", value );
        ProcessConstant( value, isOffset );
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
