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

extern baseDictEntry baseDict[];

#ifdef TRACE_INNER_INTERPRETER

// provide trace ability for builtin ops
#define NUM_TRACEABLE_OPS MAX_BUILTIN_OPS
static char *gOpNames[ NUM_TRACEABLE_OPS ];

#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

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
{
    // scratch area for temporary definitions
    mpEngineScratch = new long[70];
    InitCore( mCore );
}

ForthEngine::~ForthEngine()
{
    ForthThread *pNextThread;

    if ( mCore.DBase ) {
        delete [] mCore.DBase;
        delete mpMainVocab;
        delete mpLocalVocab;
        delete mpPrecedenceVocab;
        delete [] mpStringBufferA;
        delete [] mpStringBufferB;
    }

    if ( mCore.builtinOps )
    {
        free( mCore.builtinOps );
    }
    if ( mCore.userOps )
    {
        free( mCore.userOps );
    }

    // delete all threads;
    while ( mpThreads != NULL ) {
        pNextThread = mpThreads->mpNext;
        delete [] mpThreads;
        mpThreads = pNextThread;
    }
    delete mpEngineScratch;
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
    mCore.DBase = new long[totalLongs];
    mCore.DP = mCore.DBase;
    mCore.pEngine = this;

    mpMainVocab = new ForthVocabulary( this, "forth" );
    mpLocalVocab = new ForthVocabulary( this, "local_vars" );
    mpPrecedenceVocab = new ForthPrecedenceVocabulary( this, "precedence_ops" );

    mpStringBufferA = new char[256 * NUM_INTERP_STRINGS];
    mpStringBufferB = new char[256];

    mCore.numBuiltinOps = 0;
    mCore.builtinOps = (ForthOp *) malloc( sizeof(ForthOp) * MAX_BUILTIN_OPS );
    mCore.numUserOps = 0;
    mCore.maxUserOps = 128;
    mCore.userOps = (long **) malloc( sizeof(long *) * mCore.maxUserOps );

    Reset();

    //
    // build dispatch table for different opcode types
    //
    InitDispatchTables( mCore );

    if ( bAddBaseOps ) {
        AddBuiltinOps( baseDict );
    }
    if ( pUserBuiltinOps != NULL ) {
        AddBuiltinOps( pUserBuiltinOps );
    }

}


void
ForthEngine::Reset( void )
{
    mpDefinitionVocab = mpMainVocab;
    mpSearchVocab = mpMainVocab;

    mNextStringNum = 0;

    mLastCompiledOpcode = -1;
    mCompileState = 0;
    mCompileFlags = 0;
}


// add an op to dictionary
long
ForthEngine::AddOp( const long *pOp, forthOpType symType )
{
    long newOp;

    if ( symType == kOpBuiltIn )
    {
        ASSERT( mCore.numBuiltinOps < MAX_BUILTIN_OPS );
        mCore.builtinOps[ mCore.numBuiltinOps++ ] = (ForthOp) pOp;

        newOp = mCore.numBuiltinOps - 1;
    }
    else
    {
        if ( mCore.numUserOps == mCore.maxUserOps ) {
            mCore.maxUserOps += 128;
            mCore.userOps = (long **) realloc( mCore.userOps, sizeof(long *) * mCore.maxUserOps );
        }
        mCore.userOps[ mCore.numUserOps++ ] = (long *) pOp;

        newOp = mCore.numUserOps - 1;
    }
    return newOp;
}


// add an op to dictionary and corresponding symbol to current vocabulary
long
ForthEngine::AddUserOp( const char *pSymbol, bool smudgeIt )
{

    AlignDP();
    mpDefinitionVocab->AddSymbol( pSymbol, kOpUserDef, (long) mCore.DP, true );
    if ( smudgeIt ) {
        mpDefinitionVocab->SmudgeNewestSymbol();
    }

    return mCore.numUserOps - 1;
}


void
ForthEngine::AddBuiltinOps( baseDictEntry *pEntries )
{
    // assert if this is called after any user ops have been defined
    ASSERT( mCore.numUserOps == 0 );
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
            if ( (mCore.numBuiltinOps - 1) < NUM_TRACEABLE_OPS ) {
                gOpNames[mCore.numBuiltinOps - 1] = pEntries->name;
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
    if ( opNumber < mCore.numUserOps ) {
        mCore.DP = mCore.userOps[ opNumber ];
        mCore.numUserOps = opNumber;
    } else {
        TRACE( "ForthEngine::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, mCore.numUserOps );
    }
}

void
ForthEngine::ForgetSymbol( const char *pSym )
{
    void *pEntry = NULL;
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
           ForthForgettable::ForgetPropagate( mCore.DP, op );
        }
    }
    else
    {
        TRACE( "Error - attempt to forget unknown op %s from %s\n", pSym, mpSearchVocab->GetName() );
        printf( "Error - attempt to forget unknown op %s from %s\n", pSym, mpSearchVocab->GetName() );
    }
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
ForthEngine::PushInputStream( FILE *pInFile )
{
    mpShell->PushInputStream( pInFile );
}


void
ForthEngine::PopInputStream( void )
{
    mpShell->PopInputStream();
}



void
ForthEngine::StartOpDefinition( bool smudgeIt )
{
    mpLocalVocab->Empty();
    mLocalFrameSize = 0;
    mpLocalAllocOp = NULL;
    AlignDP();
    mpDefinitionVocab->AddSymbol( GetNextSimpleToken(), kOpUserDef, (long) mCore.DP, true );
    if ( smudgeIt ) {
        mpDefinitionVocab->SmudgeNewestSymbol();
    }
    mCompileFlags &= (~kFECompileFlagInVarsDefinition);
}


void
ForthEngine::EndOpDefinition( bool unsmudgeIt )
{
    int nLongs = mLocalFrameSize;
    if ( mpLocalAllocOp != NULL ) {
        *mpLocalAllocOp = COMPILED_OP( kOpAllocLocals, nLongs );
        mpLocalAllocOp = NULL;
        mLocalFrameSize = 0;
    }
    if ( unsmudgeIt ) {
        mpDefinitionVocab->UnSmudgeNewestSymbol();
    }
}


void *
ForthEngine::FindSymbol( const char *pSymName )
{
    void *pEntry = NULL;
    if ( (pEntry = mpPrecedenceVocab->FindSymbol( pSymName )) == NULL ) {
        pEntry = mpSearchVocab->FindSymbol( pSymName );
    }
    return pEntry;
}


void
ForthEngine::StartVarsDefinition( void )
{
    mCompileFlags |= (kFECompileFlagInVarsDefinition | kFECompileFlagHasLocalVars);
    if ( mpLocalAllocOp == NULL ) {
        // leave space for local alloc op
        mpLocalAllocOp = mCore.DP;
        CompileLong( 0 );
    }
}

void
ForthEngine::EndVarsDefinition( void )
{
    mCompileFlags &= (~kFECompileFlagInVarsDefinition);
}

void
ForthEngine::AddLocalVar( const char    *pVarName,
                          forthOpType   varType,
                          long          varSize )
{
    varSize = ((varSize + 3) & ~3) >> 2;
    mLocalFrameSize += varSize;
    mpLocalVocab->AddSymbol( pVarName, varType, mLocalFrameSize, false );
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
            if ( opVal < pEngine->mCore.numBuiltinOps ) {
                ((ForthOp) pEngine->mCore.builtinOps[opVal]) ( g );
            } else {
                g->SetError( kForthErrorBadOpcode );
            }
        } else if ( opVal == kOpUserDef ) {
            if ( opVal < pEngine->mCore.numUserOps ) {
                g->RPush( (long) g->GetIP() );
                g->SetIP( pEngine->mCore.userOps[opVal] );
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
    "Constant", "String",
    "AllocLocals",
    "LocalInt", "LocalFloat", "LocalDouble", "LocalString",
    "InvokeClassMethod",    
    "MemberInt", "MemberFloat", "MemberDouble", "MemberString",
};

// TBD: tracing of built-in ops won't work for user-added builtins...

void
ForthEngine::TraceOp()
{
    long *ip = mCore.IP - 1;
    long op = GetCurrentOp( &mCore );
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
            
        case kOpString:
            TRACE( "# \"%s\" 0x%x   @ IP = 0x%x\n", (char *)ip, op, ip );
            break;
            
        case kOpConstant:
        case kOpBranch:   case kOpBranchNZ:  case kOpBranchZ:
            if ( opVal & 0x800000 ) {
                opVal |= 0xFF000000;
            }
            TRACE( "# %s    %d   @ IP = 0x%x\n", opTypeNames[opType], opVal, ip );
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
        mCore.optypeAction[ opType ] = opAction;
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
bool
ForthEngine::ScanIntegerToken( const char   *pToken,
                               long         *pValue,
                               int          base )
{
    long value, digit;
    bool isNegative;
    char c;


    // handle leading plus or minus sign
    isNegative = (pToken[0] == '-');
    if ( isNegative || (pToken[0] == '+') ) {
        // strip leading plus/minus sign
        pToken++;
    }
    if ( pToken[0] == '\0' ) {
        // just a plus/minus sign, no number
        return false;
    }

    if ( pToken[0] == '0' ) {
        switch( pToken[1] ) {

        case 'x': case 'X':
            if ( sscanf( pToken + 2, "%x", &value ) == 1 ) {
                *pValue = (isNegative) ? 0 - value : value;
                return true;
            }
            break;
        }

    }

    value = 0;
    while ( (c = *pToken++) != 0 ) {

        if ( (c >= '0') && (c <= '9') ) {
            digit = c - '0';
        } else if ( (c >= 'A') && (c <= 'Z') ) {
            digit = 10 + (c - 'A');
        } else if ( (c >= 'a') && (c <= 'z') ) {
            digit = 10 + (c - 'a');
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

    // all chars were valid digits
    *pValue = (isNegative) ? 0 - value : value;
    return true;
}


// compile an opcode
// remember the last opcode compiled so someday we can do optimizations
//   like combining "->" followed by a local var name into one opcode
void
ForthEngine::CompileOpcode( long op )
{
    mLastCompiledOpcode = op;
    *mCore.DP++ = op;
}


void
ForthEngine::SetCurrentThread( ForthThread* pThread )
{
    if ( mpCurrentThread != NULL )
    {
        mpCurrentThread->Deactivate( mCore );
    }
    mpCurrentThread = pThread;
    mpCurrentThread->Activate( mCore );
}

//
// ExecuteOneOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
// execute forth ops, and is also how systems external to forth execute ops
//
eForthResult
ForthEngine::ExecuteOneOp( long opCode )
{
    long opScratch[2];
    long *savedIP;
    eForthResult exitStatus = kResultOk;

    opScratch[0] = opCode;
    opScratch[1] = BUILTIN_OP( OP_DONE );

    savedIP = mCore.IP;
    mCore.IP = opScratch;
    exitStatus = InnerInterpreterFunc( &mCore );
    mCore.IP = savedIP;

    return exitStatus;
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
    char *pToken = pInfo->GetToken();
    int len = pInfo->GetTokenLength();
    bool isAString = (pInfo->GetFlags() & PARSE_FLAG_QUOTED_STRING) != 0;

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
            *mCore.DP++ = COMPILED_OP( kOpString, len );
            strcpy( (char *) mCore.DP, pToken );
            mCore.DP += len;
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
            *--mCore.SP = (long) pStr;
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
            *mCore.DP++ = value | (kOpConstant << 24);
        } else {
            // in interpret mode, stick the character on the stack
            *--mCore.SP = value;
        }
        return kResultOk;
    }
    
    if ( mpInterpreterExtension != NULL ) {
        if ( (*mpInterpreterExtension)( this, pToken ) ) {
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
        pSymbol = (long *) mpLocalVocab->FindSymbol( pInfo );
        if ( pSymbol ) {
            ////////////////////////////////////
            //
            // symbol is a local variable
            //
            ////////////////////////////////////
            mLastCompiledOpcode = *pSymbol;
            *mCore.DP++ = mLastCompiledOpcode;
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

    // try to convert to a number
    // if there is a period in string
    //    try to covert to a floating point number
    // else
    //    try to convert to an integer

    if ( (strchr( pToken, '.' ) != NULL)
        && (sscanf( pToken, "%f", &fvalue ) == 1) ) {

        ////////////////////////////////////
        //
        // symbol is a single precision fp literal
        //
        ////////////////////////////////////
        SPEW_OUTER_INTERPRETER( "Floating point literal %f\n", fvalue );
        if ( mCompileState ) {
            // compile the literal value
            // value too big, must go in next longword
            *mCore.DP++ = OP_FLOAT_VAL;
            *(float *) mCore.DP++ = fvalue;
        } else {
            --mCore.SP;
            *(float *) mCore.SP = fvalue;
        }
        
    } else if ( ScanIntegerToken( pToken, &value, mCore.pThread->base ) ) {

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
                *mCore.DP++ = (value & 0xFFFFFF) | (kOpConstant << 24);
            } else {
                // value too big, must go in next longword
                *mCore.DP++ = OP_INT_VAL;
                *mCore.DP++ = value;
            }
        } else {
            // leave value on param stack
            *--mCore.SP = value;
        }

    } else {

        TRACE( "Unknown symbol %s\n", pToken );
        mCore.pThread->error = kForthErrorUnknownSymbol;
        exitStatus = kResultError;
    }

    // TBD: return exit-shell flag
    return exitStatus;
}



//############################################################################
