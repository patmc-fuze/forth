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
    extern InitAsmTables(  ForthCoreState *pCore );
    extern eForthResult InnerInterp( ForthCoreState *pCore );
};

#ifdef TRACE_INNER_INTERPRETER

// provide trace ability for builtin ops
#define NUM_TRACEABLE_OPS MAX_BUILTIN_OPS
static char *gOpNames[ NUM_TRACEABLE_OPS ];

#endif

ForthEngine* ForthEngine::mpInstance = NULL;

#define ERROR_STRING_MAX    256

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthEngine
// 

ForthEngine::ForthEngine()
: mpMainVocab( NULL )
, mpLocalVocab( NULL )
, mpPrecedenceVocab( NULL )
, mpSearchVocab( NULL )
, mpDefinitionVocab( NULL )
, mpStringBufferA( NULL )
, mpStringBufferB( NULL )
, mpThreads( NULL )
, mpInterpreterExtension( NULL )
, mpCurrentThread( NULL )
, mFastMode( false )
, mpLastCompiledOpcode( NULL )
, mNumElements( 0 )
, mpStructsManager( NULL )
{
    // scratch area for temporary definitions
    ASSERT( mpInstance == NULL );
    mpInstance = this;
    mpEngineScratch = new long[70];
    mpCore = new ForthCoreState;
    InitCore( mpCore );
    mpErrorString = new char[ ERROR_STRING_MAX + 1 ];
}

ForthEngine::~ForthEngine()
{
    ForthThread *pNextThread;

    if ( mpCore->DBase ) {
        delete [] mpCore->DBase;
        delete mpMainVocab;
        delete mpLocalVocab;
        delete mpPrecedenceVocab;
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

    mpMainVocab = new ForthVocabulary( "forth", NUM_FORTH_VOCAB_VALUE_LONGS );
    mpLocalVocab = new ForthVocabulary( "locals", NUM_LOCALS_VOCAB_VALUE_LONGS );
    mpPrecedenceVocab = new ForthPrecedenceVocabulary( "precedence_ops" );
    mpStringBufferA = new char[256 * NUM_INTERP_STRINGS];
    mpStringBufferB = new char[TMP_STRING_BUFFER_LEN];

    mpCore->numBuiltinOps = 0;
    mpCore->builtinOps = (ForthOp *) malloc( sizeof(ForthOp) * MAX_BUILTIN_OPS );
    mpCore->numUserOps = 0;
    mpCore->maxUserOps = 128;
    mpCore->userOps = (long **) malloc( sizeof(long *) * mpCore->maxUserOps );

    mpStructsManager = new ForthStructsManager();

    Reset();

    //
    // build dispatch table for different opcode types
    //
    InitDispatchTables( mpCore );

    if ( bAddBaseOps ) {
        AddBuiltinOps( baseDict );
    }
    if ( pUserBuiltinOps != NULL ) {
        AddBuiltinOps( pUserBuiltinOps );
    }

}


void
ForthEngine::ToggleFastMode( void )
{
    SetFastMode(!mFastMode );
    if ( mFastMode )
    {
        printf( "Fast mode!\n" );
    }
    else
    {
        printf( "Slow mode!\n" );
    }
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
    mpDefinitionVocab = mpMainVocab;
    mpSearchVocab = mpMainVocab;

    mNextStringNum = 0;

    mpLastCompiledOpcode = NULL;
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

    if ( symType == kOpBuiltIn )
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
    forthOpType opType;

    while ( pEntries->value != NULL ) {
        // AddSymbol will call ForthEngine::AddOp to add the operators to op table
        opType = (forthOpType) (pEntries->flags & 0xFF);
        if ( pEntries->flags & BASE_DICT_PRECEDENCE_FLAG ) {
            // symbol has precedence, put in precedence vocab
            mpPrecedenceVocab->AddSymbol( pEntries->name, opType, pEntries->value, true );
        } else {
            mpDefinitionVocab->AddSymbol( pEntries->name, opType, pEntries->value, true );
        }

#ifdef TRACE_INNER_INTERPRETER
        // add built-in op names to table for TraceOp
        if ( opType == kOpBuiltIn ) {
            if ( (mpCore->numBuiltinOps - 1) < NUM_TRACEABLE_OPS ) {
                gOpNames[mpCore->numBuiltinOps - 1] = pEntries->name;
            }
        }
#endif
        pEntries++;

    }
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
    ForthVocabulary *pFoundVocab = NULL;
    long op;
    forthOpType opType;

    if ( (pEntry = mpSearchVocab->FindSymbol( pSym )) != NULL ) {
        pFoundVocab = mpSearchVocab;
    } else if ( (pEntry = mpPrecedenceVocab->FindSymbol( pSym )) != NULL ) {
        pFoundVocab = mpPrecedenceVocab;
    }

    if ( pFoundVocab != NULL ) {
        op = ForthVocabulary::GetEntryValue( pEntry );
        opType = ForthVocabulary::GetEntryType( pEntry );
        if ( opType == kOpBuiltIn ) {
            // sym is unknown, or in built-in ops - no way
            TRACE( "Error - attempt to forget builtin op %s from %s\n", pSym, pFoundVocab->GetName() );
            printf( "Error - attempt to forget builtin op %s from %s\n", pSym, pFoundVocab->GetName() );
        }
        else
        {
           ForgetOp( op );
           ForthForgettable::ForgetPropagate( mpCore->DP, op );
        }
    }
    else
    {
        TRACE( "Error - attempt to forget unknown op %s from %s\n", pSym, mpSearchVocab->GetName() );
        return false;
    }
    return true;
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
ForthEngine::StartOpDefinition( const char *pName, bool smudgeIt )
{
    mpLocalVocab->Empty();
    mLocalFrameSize = 0;
    mpLocalAllocOp = NULL;
    mpLastCompiledOpcode = NULL;
    AlignDP();
    if ( pName == NULL )
    {
        pName = GetNextSimpleToken();
    }
    long* pEntry = mpDefinitionVocab->AddSymbol( pName, kOpUserDef, (long) mpCore->DP, true );
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
    if ( unsmudgeIt )
    {
        mpDefinitionVocab->UnSmudgeNewestSymbol();
    }
}


long *
ForthEngine::FindSymbol( const char *pSymName )
{
    long *pEntry = NULL;
    if ( (pEntry = mpPrecedenceVocab->FindSymbol( pSymName )) == NULL ) {
        pEntry = mpSearchVocab->FindSymbol( pSymName );
    }
    return pEntry;
}

void
ForthEngine::StartStructDefinition( void )
{
    mCompileFlags |= kFECompileFlagInStructDefinition;
}

void
ForthEngine::EndStructDefinition( void )
{
    mCompileFlags &= (~kFECompileFlagInStructDefinition);
}

long
ForthEngine::AddLocalVar( const char        *pVarName,
                          long              typeCode,
                          long              varSize )
{
    long *pEntry;
    if ( mpLocalAllocOp == NULL ) {
        // this is first local var definition, leave space for local alloc op
        mCompileFlags |= kFECompileFlagHasLocalVars;
        mpLocalAllocOp = mpCore->DP;
        CompileLong( 0 );
    }
    varSize = ((varSize + 3) & ~3) >> 2;
    mLocalFrameSize += varSize;
    if ( CODE_IS_NATIVE( typeCode ) )
    {
        long varType = CODE_TO_NATIVE_TYPE( typeCode );
        pEntry = mpLocalVocab->AddSymbol( pVarName, (kOpLocalByte + varType), mLocalFrameSize, false );
    }
    else
    {
        if ( CODE_IS_PTR( typeCode ) )
        {
            // treat a struct ptr like an int
            pEntry = mpLocalVocab->AddSymbol( pVarName, kOpLocalInt, mLocalFrameSize, false );
        }
        else
        {
            pEntry = mpLocalVocab->AddSymbol( pVarName, kOpLocalRef, mLocalFrameSize, false );
        }
    }
    pEntry[1] = typeCode;

    return mLocalFrameSize;
}

long
ForthEngine::AddLocalArray( const char          *pArrayName,
                            long                typeCode,
                            long                elementSize )
{
    long arraySize = elementSize * mNumElements;
    long *pEntry;
    if ( mpLocalAllocOp == NULL ) {
        // this is first local var definition, leave space for local alloc op
        mCompileFlags |= kFECompileFlagHasLocalVars;
        mpLocalAllocOp = mpCore->DP;
        CompileLong( 0 );
    }
    arraySize = ((arraySize + 3) & ~3) >> 2;
    mLocalFrameSize += arraySize;
    if ( CODE_IS_NATIVE( typeCode ) )
    {
        long elementType = CODE_TO_NATIVE_TYPE( typeCode );
        pEntry = mpLocalVocab->AddSymbol( pArrayName, (kOpLocalByteArray + elementType), mLocalFrameSize, false );
    }
    else
    {
        long fieldBytes, alignment, padding, alignMask;
        ForthStructsManager* pManager = ForthStructsManager::GetInstance();
        pManager->GetFieldInfo( typeCode, fieldBytes, alignment );
        alignMask = alignment - 1;
        padding = fieldBytes & alignMask;
        long paddedSize = (padding) ? (fieldBytes + (alignment - padding)) : fieldBytes;
        pEntry = mpLocalVocab->AddSymbol( pArrayName, kOpLocalStructArray, (mLocalFrameSize << 12) + paddedSize, false );
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


static char *opTypeNames[] = {
    "BuiltIn", "UserDefined", 
    "Branch", "BranchTrue", "BranchFalse", "CaseBranch",
    "Constant", "Offset", "ArrayOffset", "ConstantString",
    "AllocLocals", "InitLocalString",
    "LocalByte", "LocalShort", "LocalInt", "LocalFloat", "LocalDouble", "LocalString",
    "FieldByte", "FieldShort", "FieldInt", "FieldFloat", "FieldDouble", "FieldString",
    "InvokeClassMethod",    
    "MemberByte", "MemberShort", "MemberInt", "MemberFloat", "MemberDouble", "MemberString",
};

// TBD: tracing of built-in ops won't work for user-added builtins...

void
ForthEngine::TraceOp()
{
    long *ip = mpCore->IP - 1;
    long op = GetCurrentOp( mpCore );
    forthOpType opType = FORTH_OP_TYPE( op );
    ulong opVal = FORTH_OP_VALUE( op );

    if ( opType >= (sizeof(opTypeNames) / sizeof(char *)) ) {
        TRACE( "# BadOpType 0x%x  @ IP = 0x%x\n", op, ip );
    } else {

        switch( opType ){
            
        case kOpBuiltIn:
            if ( opVal != OP_DONE ) {
#ifdef TRACE_INNER_INTERPRETER
                if ( opVal < NUM_TRACEABLE_OPS ) {
                    // traceable built-in op
                    TRACE( "# %s    0x%x   @ IP = 0x%x\n", gOpNames[opVal], op, ip );
                } else {
                    // op we don't have name pointer for
                    TRACE( "# %s    0x%x   @ IP = 0x%x\n", opTypeNames[opType], op, ip );
                }
#else
                TRACE( "# %s    0x%x   @ IP = 0x%x\n", opTypeNames[opType], op, ip );
#endif
            }
            break;
            
        case kOpConstantString:
            TRACE( "# \"%s\" 0x%x   @ IP = 0x%x\n", (char *)ip, op, ip );
            break;
            
        case kOpConstant:
        case kOpOffset:
        case kOpBranch:   case kOpBranchNZ:  case kOpBranchZ:
            if ( opVal & 0x800000 ) {
                opVal |= 0xFF000000;
            }
            TRACE( "# %s    %d   @ IP = 0x%x\n", opTypeNames[opType], opVal, ip );
            break;
            
        case kOpArrayOffset:
            TRACE( "# %s    elementSize %d  @ IP = 0x%x\n", opTypeNames[opType], opVal, ip );
            break;
            
        default:
            TRACE( "# %s    0x%x   @ IP = 0x%x\n", opTypeNames[opType], op, ip );
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
    *mpCore->DP++ = op;
}

void
ForthEngine::UncompileLastOpcode( void )
{
    if ( mpLastCompiledOpcode != NULL )
    {
        mpCore->DP = mpLastCompiledOpcode;
        mpLastCompiledOpcode = NULL;
    }
    else
    {
        TRACE( "ForthEngine::UncompileLastOpcode called with no previous opcode\n" );
        SetError( kForthErrorMissingSize, "UncompileLastOpcode called with no previous opcode" );
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

    return exitStatus;
}

static char *pErrorStrings[] =
{
    "No Error",
    "Bad Opcode",
    "Bad OpcodeType",
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
    "Syntax error",
    "Syntax error - else without matching if",
    "Syntax error - endif without matching if/else",
    "Syntax error - loop without matching do",
    "Syntax error - until without matching begin",
    "Syntax error - while without matching begin",
    "Syntax error - repeat without matching while",
    "Syntax error - again without matching begin",
};

void
ForthEngine::SetError( eForthError e, const char *pString )
{
    mpCore->error = e;
    if ( pString )
    {
        strncpy( mpErrorString, pString, ERROR_STRING_MAX );
    }
    else
    {
        mpErrorString[0] = '\0';
    }
    if ( e != kForthErrorNone )
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
    else
    {
        mpErrorString[0] = '\0';
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


//############################################################################
//
//          O U T E R    I N T E R P R E T E R  (sort of)
//
//############################################################################


// return true to exit forth shell
eForthResult
ForthEngine::ProcessToken( ForthParseInfo   *pInfo )
{
    long *pSymbol, value;
    eForthResult exitStatus = kResultOk;
    float fvalue;
    double dvalue;
    char *pToken = pInfo->GetToken();
    int len = pInfo->GetTokenLength();
    bool isAString = (pInfo->GetFlags() & PARSE_FLAG_QUOTED_STRING) != 0;
    bool isSingle, isOffset;
    double* pDPD;

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
        pSymbol = mpLocalVocab->FindSymbol( pInfo );
        if ( pSymbol ) {
            ////////////////////////////////////
            //
            // symbol is a local variable
            //
            ////////////////////////////////////
            CompileOpcode( *pSymbol );
            return kResultOk;
        }
    }

    if ( mpPrecedenceVocab->ProcessSymbol( pInfo, exitStatus ) != NULL )
    {
        ////////////////////////////////////
        //
        // symbol is a forth op with precedence
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Precedence Op\n" );
        return exitStatus;
    }
    else
    {
        ForthVocabulary* pVocab = mpSearchVocab;
        while ( pVocab != NULL )
        {
            if ( pVocab->ProcessSymbol( pInfo, exitStatus ) )
            {
                ////////////////////////////////////
                //
                // symbol is a forth op
                //
                ////////////////////////////////////
                SPEW_OUTER_INTERPRETER( "Op in vocab %s\n", pVocab->GetName() );
                return exitStatus;
            }
            pVocab = pVocab->GetNextSearchVocabulary();
        }

    }

    // see if this is a structure access (like structA.fieldB.fieldC)
    if ( pInfo->GetFlags() & PARSE_FLAG_HAS_PERIOD ) 
    {
        if ( mpStructsManager->ProcessSymbol( pInfo, exitStatus ) )
        {
            ////////////////////////////////////
            //
            // symbol is a structure access
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
              // value too big, must go in next longword
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
              // value too big, must go in next longword
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
