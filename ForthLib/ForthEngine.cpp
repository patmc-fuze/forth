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

extern baseDictEntry baseDict[];

// ForthEngine::InnerInterpreter dispatches through this table for all opcode types
// other than SimpleOp and UserOp

// NOTE: remember to update opTypeNames if you add a new optype

optypeActionRoutine optypeAction[256];

#ifdef TRACE_INNER_INTERPRETER

// provide trace ability for builtin ops
#define NUM_TRACEABLE_OPS 500
static char *gOpNames[ NUM_TRACEABLE_OPS ];

#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ForthEngine::ForthEngine()
: mpDBase( NULL )
, mpDP( NULL )
, mpMainVocab( NULL )
, mpLocalVocab( NULL )
, mpPrecedenceVocab( NULL )
, mpSearchVocab( NULL )
, mpDefinitionVocab( NULL )
, mpStringBufferA( NULL )
, mpStringBufferB( NULL )
, mpOpTable( NULL )
, mNumOps( 0 )
, mNumBuiltinOps( 0 )
, mpThreads( NULL )
, mpInterpreterExtension( NULL )
{
    // scratch area for temporary definitions
    mpEngineScratch = new long[70];
}

ForthEngine::~ForthEngine()
{
    ForthThread *pNextThread;

    if ( mpDBase ) {
        delete [] mpDBase;
        delete mpMainVocab;
        delete mpLocalVocab;
        delete mpPrecedenceVocab;
        delete [] mpStringBufferA;
        delete [] mpStringBufferB;
        free( mpOpTable );
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
    mpDBase = new long[totalLongs];
    mpDP = mpDBase;

    mpMainVocab = new ForthVocabulary( this, "forth" );
    mpLocalVocab = new ForthVocabulary( this, "local_vars" );
    mpPrecedenceVocab = new ForthPrecedenceVocabulary( this, "precedence_ops" );

    mpStringBufferA = new char[256 * NUM_INTERP_STRINGS];
    mpStringBufferB = new char[256];

    mNumBuiltinOps = 0;
    mNumOps = 0;
    mMaxOps = 128;
    mpOpTable = (long **) malloc( sizeof(long *) * mMaxOps );

    Reset();

    if ( bAddBaseOps ) {
        AddBuiltinOps( baseDict );
    }
    if ( pUserBuiltinOps != NULL ) {
        AddBuiltinOps( pUserBuiltinOps );
    }

    //
    // build dispatch table for different opcode types
    //
    int i;
    for ( i = 0; i < 256; i++ ) {
        optypeAction[i] = ForthEngine::BadOpcodeTypeError;
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


long
ForthEngine::AddOp( const long *pOp )
{
    if ( mNumOps == mMaxOps ) {
        mMaxOps += 128;
        mpOpTable = (long **) realloc( mpOpTable, sizeof(long *) * mMaxOps );
    }
    mpOpTable[ mNumOps++ ] = (long *) pOp;

    return mNumOps - 1;
}


long
ForthEngine::AddUserOp( const char *pSymbol, bool smudgeIt )
{

    AlignDP();
    mpDefinitionVocab->AddSymbol( pSymbol, kOpUserDef, (long) mpDP, true );
    if ( smudgeIt ) {
        mpDefinitionVocab->SmudgeNewestSymbol();
    }

    return mNumOps - 1;
}


void
ForthEngine::AddBuiltinOps( baseDictEntry *pEntries )
{
    // assert if this is called after any user ops have been defined
    ASSERT( mNumOps == mNumBuiltinOps );
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
            if ( (mNumOps - 1) < NUM_TRACEABLE_OPS ) {
                gOpNames[mNumOps - 1] = pEntries->name;
            }
        }
#endif
        pEntries++;

    }
    mNumBuiltinOps = mNumOps;
}


// forget the specified op and all higher numbered ops, and free the memory where those ops were stored
void
ForthEngine::ForgetOp( ulong opNumber )
{
    if ( opNumber < mNumBuiltinOps ) {
        TRACE( "ForthEngine::ForgetOp error - attempt to forget builtin op # %d\n", opNumber );
    } else if ( opNumber < mNumOps ) {
        mpDP = mpOpTable[ opNumber ];
        mNumOps = opNumber;
    } else {
        TRACE( "ForthEngine::ForgetOp error - attempt to forget bogus op # %d, only %d ops exist\n", opNumber, mNumOps );
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
           ForthForgettable::ForgetPropagate( mpDP, op );
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
    mpDefinitionVocab->AddSymbol( GetNextSimpleToken(), kOpUserDef, (long) mpDP, true );
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
        mpLocalAllocOp = mpDP;
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
            if ( opVal < pEngine->mNumBuiltinOps ) {
                ((ForthOp) pEngine->mpOpTable[opVal]) ( g );
            } else {
                g->SetError( kForthErrorBadOpcode );
            }
        } else if ( opVal == kOpUserDef ) {
            if ( opVal < pEngine->mNumOps ) {
                g->RPush( (long) g->mIP );
                g->mIP = pEngine->mpOpTable[opVal];
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


void
ForthEngine::BadOpcodeTypeError( ForthThread    *g,
                                 ulong          opVal )
{
    g->SetError( kForthErrorBadOpcode );
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
ForthEngine::TraceOp( ForthThread     *g )
{
    long *ip;
    ulong opVal;
    forthOpType opType;

    ip= g->mIP - 1;
    opType = FORTH_OP_TYPE( g->mCurrentOp );
    opVal = FORTH_OP_VALUE( g->mCurrentOp );
    if ( opType >= (sizeof(opTypeNames) / sizeof(char *)) ) {
        TRACE( "# BadOpType 0x%x  @ IP = 0x%x\n", g->mCurrentOp, ip );
    } else {

        switch( opType ){
            
        case kOpBuiltIn:
            if ( opVal != OP_DONE ) {
#ifdef TRACE_INNER_INTERPRETER
                if ( opVal < NUM_TRACEABLE_OPS ) {
                    // traceable built-in op
                    TRACE( "# %s    0x%x   @ IP = 0x%x\n", gOpNames[opVal], g->mCurrentOp, ip );
                } else {
                    // op we don't have name pointer for
                    TRACE( "# %s    0x%x   @ IP = 0x%x\n", opTypeNames[opType], g->mCurrentOp, ip );
                }
#else
                TRACE( "# %s    0x%x   @ IP = 0x%x\n", opTypeNames[opType], g->mCurrentOp, ip );
#endif
            }
            break;
            
        case kOpString:
            TRACE( "# \"%s\" 0x%x   @ IP = 0x%x\n", (char *)(g->mIP), g->mCurrentOp, ip );
            break;
            
        case kOpConstant:
        case kOpBranch:   case kOpBranchNZ:  case kOpBranchZ:
            if ( opVal & 0x800000 ) {
                opVal |= 0xFF000000;
            }
            TRACE( "# %s    %d   @ IP = 0x%x\n", opTypeNames[opType], opVal, ip );
            break;
            
        default:
            TRACE( "# %s    0x%x   @ IP = 0x%x\n", opTypeNames[opType], g->mCurrentOp, ip );
            break;
        }
    }
}



bool
ForthEngine::AddOpType( forthOpType opType, optypeActionRoutine opAction )
{

    if ( (opType >= kOpLocalUserDefined) && (opType <= kOpMaxLocalUserDefined) ) {
        optypeAction[ opType ] = opAction;
    } else {
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
    *mpDP++ = op;
}


//############################################################################
//
//          O U T E R    I N T E R P R E T E R  (sort of)
//
//############################################################################


// return true to exit forth shell
eForthResult
ForthEngine::ProcessToken( ForthThread    *g,
                           ForthParseInfo   *pInfo )
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
            *mpDP++ = COMPILED_OP( kOpString, len );
            strcpy( (char *) mpDP, pToken );
            mpDP += len;
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
            g->Push( (long) pStr );
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
            *mpDP++ = value | (kOpConstant << 24);
        } else {
            // in interpret mode, stick the character on the stack
            g->Push( value );
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
            *mpDP++ = mLastCompiledOpcode;
            return kResultOk;
        }
    }

    if ( mpPrecedenceVocab->ProcessSymbol( pInfo, g, exitStatus ) != NULL )
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
            if ( pVocab->ProcessSymbol( pInfo, g, exitStatus ) )
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
            *mpDP++ = OP_FLOAT_VAL;
            *(float *) mpDP++ = fvalue;
        } else {
            g->FPush( fvalue );
        }
        
    } else if ( ScanIntegerToken( pToken, &value, *(g->GetBaseRef()) ) ) {

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
                *mpDP++ = (value & 0xFFFFFF) | (kOpConstant << 24);
            } else {
                // value too big, must go in next longword
                *mpDP++ = OP_INT_VAL;
                *mpDP++ = value;
            }
        } else {
            // leave value on param stack
            g->Push( value );
        }

    } else {

        TRACE( "Unknown symbol %s\n", pToken );
        g->SetError( kForthErrorUnknownSymbol );
        exitStatus = kResultError;
    }

    // TBD: return exit-shell flag
    return exitStatus;
}

//############################################################################
//
//          I N N E R    I N T E R P R E T E R
//
//############################################################################


eForthResult
ForthEngine::InnerInterpreter( ForthThread  *g )
{
    ulong opVal;
    ForthOp builtinOp;
    forthOpType opType;
    long *pSP;
    int methodNum;
    long *pObj, *pClass;

    g->mState = kResultOk;
    
    while ( g->mState == kResultOk ) {
        // fetch op at IP, advance IP
        g->mCurrentOp = *g->mIP++;
#ifdef TRACE_INNER_INTERPRETER
        TraceOp( g );
#endif
        opType = FORTH_OP_TYPE( g->mCurrentOp );
        opVal = FORTH_OP_VALUE( g->mCurrentOp );

        switch ( opType ) {

        case kOpBuiltIn:
            // built in simple op
            // look up routine in table of built-in ops & execute
            if ( opVal < mNumBuiltinOps ) {
                builtinOp = (ForthOp) mpOpTable[opVal];
                builtinOp( g );
            } else {
                g->SetError( kForthErrorBadOpcode );
            }
            break;

        case kOpUserDef:
            // op is normal user-defined, push IP on rstack, lookup new IP
            //  in table of user-defined ops
            if ( opVal < mNumOps ) {
                g->RPush( (long) g->mIP );
                g->mIP = mpOpTable[opVal];
            } else {
                g->SetError( kForthErrorBadOpcode );
            }
            break;

        case kOpBranch:
            if ( (opVal & 0x00800000) != 0 ) {
                // TBD: trap a hard loop (offset == -1)?
                opVal |= 0xFF000000;
            }
            g->mIP += opVal;
            break;

        case kOpBranchNZ:
            if ( g->Pop() != 0 ) {
                if ( (opVal & 0x00800000) != 0 ) {
                    // TBD: trap a hard loop (offset == -1)?
                    opVal |= 0xFF000000;
                }
                g->mIP += opVal;
            }
            break;

        case kOpBranchZ:
            if ( g->Pop() == 0 ) {
                if ( (opVal & 0x00800000) != 0 ) {
                    // TBD: trap a hard loop (offset == -1)?
                    opVal |= 0xFF000000;
                }
                g->mIP += opVal;
            }
            break;

        case kOpCaseBranch:
            // TOS: this_case_value case_selector
            pSP = g->GetSP();
            if ( *pSP == pSP[1] ) {
                // case matched
                pSP += 2;
            } else {
                // no match - drop this_case_value & skip to next case
                pSP++;
                // case branch is always forward
                g->mIP += opVal;
            }
            g->SetSP( pSP );
            break;

        case kOpConstant:
            // push constant in opVal
            if ( (opVal & 0x00800000) != 0 ) {
              opVal |= 0xFF000000;
            }
            g->Push( opVal );
            break;

        case kOpString:
            // push address of immediate string & skip over
            // opVal is number of longwords in string
            g->Push( (long) g->mIP );
            g->mIP += opVal;
            break;

        case kOpAllocLocals:
            // allocate a local var stack frame
            g->RPush( (long) g->mFP );      // rpush old FP
            g->mFP = g->mRP;                // set FP = RP, points at oldFP
            g->mRP -= opVal;                // allocate storage for local vars
            break;

        case kOpLocalInt:
            LocalIntAction( g, opVal );
            break;

        case kOpLocalFloat:
            LocalFloatAction( g, opVal );
            break;

        case kOpLocalDouble:
            LocalDoubleAction( g, opVal );
            break;

        case kOpLocalString:
            LocalStringAction( g, opVal );
            break;

        case kOpFieldInt:
            FieldIntAction( g, opVal );
            break;

        case kOpFieldFloat:
            FieldFloatAction( g, opVal );
            break;

        case kOpFieldDouble:
            FieldDoubleAction( g, opVal );
            break;

        case kOpFieldString:
            FieldStringAction( g, opVal );
            break;

        case kOpMemberInt:
            MemberIntAction( g, opVal );
            break;

        case kOpMemberFloat:
            MemberFloatAction( g, opVal );
            break;

        case kOpMemberDouble:
            MemberDoubleAction( g, opVal );
            break;

        case kOpMemberString:
            MemberStringAction( g, opVal );
            break;

        case kOpMethodWithThis:
            // this is called when a method is invoked from inside another
            // method in the same class - the difference is that in this case
            // there is no explicit source for the "this" pointer, we just keep
            // on using the current "this" pointer
            pObj = g->GetTP();
            if ( pObj != NULL ) {
                // pObj is a pair of pointers, first pointer is to
                //   class descriptor for this type of object,
                //   second pointer is to storage for object (this ptr)
                pClass = (long *) (*pObj);
                if ( pClass[2] > (long) opVal ) {
                    g->RPush( (long) g->mIP );
                    g->RPush( (long) g->mTP );
                    g->mTP = pObj;
                    g->mIP = (long *) (pClass[opVal + 3]);
                } else {
                    // bad method number
                    g->SetError( kForthErrorBadOpcode );
                }
            } else {
                g->SetError( kForthErrorBadOpcode );
            }
            break;


        default:

            if ( opType >= kOpUserMethods ) {

                // token is object method invocation
                methodNum = ((int) opType) & 0x7F;
                pObj = NULL;
                if ( opVal & 0x00800000 ) {
                    // object ptr is in local variable
                    pObj = g->GetFP() - (opVal & 0x007F0000);
                } else {
                    // object ptr is in global op
                    if ( opVal < mNumOps ) {
                        pObj = (long *) (mpOpTable[opVal]);
                    } else {
                        g->SetError( kForthErrorBadOpcode );
                    }
                }
                if ( pObj != NULL ) {
                    // pObj is a pair of pointers, first pointer is to
                    //   class descriptor for this type of object,
                    //   second pointer is to storage for object (this ptr)
                    pClass = (long *) (*pObj);
                    if ( (pClass[1] == CLASS_MAGIC_NUMBER)
                      && (pClass[2] > methodNum) ) {
                        g->RPush( (long) g->mIP );
                        g->RPush( (long) g->mTP );
                        g->mTP = pObj;
                        g->mIP = (long *) (pClass[methodNum + 3]);
                    } else {
                        // bad class magic number, or bad method number
                        g->SetError( kForthErrorBadOpcode );
                    }
                } else {
                    g->SetError( kForthErrorBadOpcode );
                }

            } else {
                optypeAction[opType]( g, opVal );
            }
        }

    }

    return g->mState;
}

//############################################################################
