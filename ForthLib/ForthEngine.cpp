// ForthEngine.cpp: implementation of the ForthEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthEngine.h"
#include "ForthThread.h"

extern baseDictEntry baseDict[];

// these are the ops which take immediate data
typedef void (*immediateDataOp)( ForthThread *g, ulong theData );
immediateDataOp immDataFuncs[256] = {
    ForthEngine::SimpleOp,
    ForthEngine::UserOp,
    ForthEngine::RelBranch,
    ForthEngine::RelBranchTrue,
    ForthEngine::RelBranchFalse,

    ForthEngine::ImmediateLong,
    ForthEngine::ImmediateString,
    ForthEngine::AllocateLocals,
    ForthEngine::LocalIntAction,
    ForthEngine::LocalFloatAction,
    ForthEngine::LocalDoubleAction,
    ForthEngine::LocalStringAction,
    //ForthEngine::LocalStore32,
    //ForthEngine::LocalStore64,
        // this ends the list
        (immediateDataOp) -1
};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ForthEngine::ForthEngine()
{
    mpDBase = NULL;
    mpDP = NULL;
    mpMainVocab = NULL;
    mpLocalVocab = NULL;
    mpCurrentVocab = NULL;
    mpStringBufferA = NULL;
    mpStringBufferB = NULL;
    mpOpTable = NULL;
    mpThreads = NULL;
    // scratch area for temporary definitions
    mpEngineScratch = new long[70];
    // start out with compile turned off
}

ForthEngine::~ForthEngine()
{
    ForthThread *pNextThread;

    if ( mpDBase ) {
        delete [] mpDBase;
        delete [] mpMainVocab;
        delete [] mpLocalVocab;
        delete [] mpStringBufferA;
        delete [] mpStringBufferB;
        free( mpOpTable );
    }

    // delete all threads;
    while ( mpThreads != NULL ) {
        pNextThread = mpThreads;
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
ForthEngine::Initialize( int        totalLongs,
                         int        vocabularyLongs,
                         int        maxUserEntries,
                         bool       bAddBaseOps )
{
    mpDBase = new long[totalLongs];
    mpDP = mpDBase;

    mpMainVocab = new ForthVocabulary( this, vocabularyLongs << 2 );
    mpCurrentVocab = mpMainVocab;
    mpLocalVocab = new ForthVocabulary( this, 256 );

    mpStringBufferA = new char[256 * NUM_INTERP_STRINGS];
    mNextStringNum = 0;
    mpStringBufferB = new char[256];

    mpOpTable = (long **) malloc( sizeof(long *) * maxUserEntries );
    mNumBuiltinOps = 0;
    mNumOps = 0;
    mMaxOps = maxUserEntries;

    if ( bAddBaseOps ) {
        AddBuiltinOps( baseDict );
    }

    mLastCompiledOpcode = -1;
    mCompileState = 0;
    mbInVarsDefinition = false;

    int i;
    bool stuffIt = false;
    for ( i = 0; i < 256; i++ ) {
        if ( stuffIt ) {
            immDataFuncs[i] = ForthEngine::BadOpcodeTypeError;
        } else {
            if ( (long) immDataFuncs[i] == -1 ) {
                stuffIt = true;
                immDataFuncs[i] = ForthEngine::BadOpcodeTypeError;
            }
        }
    }
}

long
ForthEngine::AddOp( long *pOp )
{
    if ( mNumOps == mMaxOps ) {
        mMaxOps += 128;
        mpOpTable = (long **) realloc( mpOpTable, sizeof(long *) * mMaxOps );
    }
    mpOpTable[ mNumOps++ ] = pOp;

    return mNumOps - 1;
}

void
ForthEngine::AddBuiltinOps( baseDictEntry *pEntries )
{
    while ( pEntries->value != NULL ) {
        mpCurrentVocab->AddSymbol( pEntries->name, pEntries->flags, pEntries->value );
        pEntries++;
        mNumBuiltinOps++;
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
ForthEngine::StartOpDefinition( void )
{
    mpLocalVocab->Empty();
    mLocalFrameSize = 0;
    mpLocalAllocOp = NULL;
    AlignDP();
    mpCurrentVocab->AddSymbol( GetNextSimpleToken(), kOpUserDef, (long) mpDP );
    mbInVarsDefinition = false;
}


void
ForthEngine::EndOpDefinition( void )
{
    int nLongs = (mLocalFrameSize + 3) >> 2;
    if ( mpLocalAllocOp != NULL ) {
        *mpLocalAllocOp = COMPILED_OP( kOpAllocLocals, nLongs );
        mpLocalAllocOp = NULL;
        mLocalFrameSize = 0;
    }
}


void
ForthEngine::StartVarsDefinition( void )
{
    mbInVarsDefinition = true;
    if ( mpLocalAllocOp == NULL ) {
        // leave space for local alloc op
        mpLocalAllocOp = mpDP;
        CompileLong( 0 );
    }
}

void
ForthEngine::EndVarsDefinition( void )
{
    mbInVarsDefinition = false;
}

void
ForthEngine::AddLocalVar( char          *pVarName,
                          forthOpType   varType,
                          long          varSize )
{
    mpLocalVocab->AddSymbol( pVarName, varType, mLocalFrameSize );
    mLocalFrameSize += varSize;
}


// built in simple op
// look up routine in table of built-in ops & execute
// this routine is probably never called, inner interpreter does
// dispatch for these ops directly
void
ForthEngine::SimpleOp( ForthThread    *g,
                       ulong          loOp )
{
    ForthEngine *pEngine = g->mpEngine;

   if ( loOp < pEngine->mNumBuiltinOps ) {
      ((ForthOp) pEngine->mpOpTable[loOp]) ( g );
   } else {
      g->SetError( kForthErrorBadOpcode );
   }
}


// token is normal user-defined, push IP on rstack, lookup new IP
//  in table of user-defined ops
// this routine is probably never called, inner interpreter does
// dispatch for these ops directly
void
ForthEngine::UserOp( ForthThread    *g,
                     ulong          loOp )
{
    ForthEngine *pEngine = g->mpEngine;

   if ( loOp < pEngine->mNumOps ) {
      g->RPush( (long) g->mIP );
      g->mIP = pEngine->mpOpTable[loOp];
   } else {
      g->SetError( kForthErrorBadOpcode );
   }
}


#if 0
// token is user-defined <builds-does>, push IP on rstack,
// op table .value points to lword holding new IP followed by
//   immediate data
// this routine is probably never called, inner interpreter does
// dispatch for these ops directly
void
paramOp( ForthThread    *g,
         ulong          loOp )
{
   long tmp, *pTmp;
    ForthEngine *pEngine = g->mpEngine;

   if ( loOp < gNumOps ) {
      RPUSH( ((ulong) g->mIP) );
      pTmp = pEngine->mpOpTable[loOp];
      tmp = *pTmp++;
      g->mIP = tmp;
      PUSH( (char *) pTmp - (char *) gpMBase );
   } else {
      g->SetError( FLAG_BADOP );
   }
}
#endif

// the immediate data is just a constant
void
ForthEngine::ImmediateLong( ForthThread    *g,
                                ulong          theData )
{
   if ( theData & 0x800000 ) {
      theData |= 0xFF000000;
   }
   g->Push( theData );
}


// the immediate data is # of longwords of string data following this opcode
void
ForthEngine::ImmediateString( ForthThread    *g,
                              ulong          theData )
{
   g->Push( (long) g->mIP );
   g->mIP += theData;
}


// for all relative branches: branch address is relative to address right after opcode, so
//  a relative branch value of 0 means the instruction just after the branch, a value of -1
//  means branch to the instruction itself

// unconditional relative branch
void
ForthEngine::RelBranch( ForthThread    *g,
                        ulong          theData )
{
   long offset = theData;

   if ( theData & 0x00800000 ) {
      // offset is negative
      offset |= 0xFF000000;
   }

    // TBD: trap a hard loop (offset == -1)?
    g->mIP += offset;
}


// relative branch if TOS is true
void
ForthEngine::RelBranchTrue( ForthThread    *g,
                            ulong          theData )
{
   long offset = theData;
   long bJump = g->Pop();

   if ( bJump ) {
      if ( theData & 0x00800000 ) {
         // offset is negative
         offset |= 0xFF000000;
      }
        g->mIP += offset;

   }
}


// relative branch if TOS is false
void
ForthEngine::RelBranchFalse( ForthThread    *g,
                             ulong          theData )
{
   long offset = theData;
   long bJump = g->Pop();

   if ( bJump == 0 ) {
      if ( theData & 0x00800000 ) {
         // offset is negative
         offset |= 0xFF000000;
      }

        g->mIP += offset;
   }
}


//
// do a user-defined immediate data op
//
void
ForthEngine::UserImmediateOp( ForthThread  *g,
                              ulong        theData )
{
   ulong userOp;
    ForthEngine *pEngine = g->mpEngine;

   // push IP on rstack, lookup new IP
   //  in table of user-defined ops
   // DOH! all user-defined immediate ops must be in first 256 definitions
   userOp = g->mToken >> 24;
   if ( userOp < pEngine->mNumOps ) {
      g->RPush( (long) g->mIP );
      g->Push( theData );
      g->mIP = pEngine->mpOpTable[userOp];
   } else {
      g->SetError( kForthErrorBadOpcode );
   }
}

static long unravelIP = OP_UNRAVEL;

// allocate a frame of local variables
void
ForthEngine::AllocateLocals( ForthThread    *g,
                             ulong          numLongs )
{
    long *oldFP, *oldRP;   
    
    // save old FP
    oldFP = g->mFP;
    
    // save old RP
    oldRP = g->mRP;
    
    // set RP = RP - numLongs
    g->mRP -= numLongs;
    
    // set FP = RP
    g->mFP = g->mRP;
    
    // rpush old FP
    g->RPush( (long) oldFP );
    
    // rpush old RP
    g->RPush( (long) oldRP );
    
    // rpush IP of unraveler code
    g->RPush( (long) &unravelIP );
}

// ForthEngine::LocalInt, LocalFloat and LocalDouble are in ForthOps.cpp
//   since they share code with the non-local variable ops

void
ForthEngine::LocalStore32( ForthThread   *g,
                           ulong         offset )
{
    g->mFP[offset] = g->Pop();
}

void
ForthEngine::LocalStore64( ForthThread   *g,
                           ulong         offset )
{
    *(double *)(&g->mFP[offset]) = g->DPop();
}

#if 0

void
ForthEngine::LVFetch( ForthThread   *g,
                      ulong         offset )
{
    g->Push( g->fp[offset] );
}

void
ForthEngine::LVAddr( ForthThread   *g,
                     ulong         offset )
{
    // Is this right?
    g->Push( (long) (g->fp + offset) );
}


#endif

void
ForthEngine::BadOpcodeTypeError( ForthThread    *g,
                                 ulong          loOp )
{
    g->SetError( kForthErrorBadOpcode );
}





static char *opTypeNames[] = {
    "BuiltIn", "UserOp",  /* "ParamOp",  */
    "Branch", "BranchTrue", "BranchFalse",
    "Constant", "String",
    "AllocLocals",
    "LocalInt", "LocalFloat", "LocalDouble"
};

// TBD: tracing of built-in ops won't work for user-added builtins...

void
ForthEngine::TraceOp( ForthThread     *g )
{
    long *ip;
    ulong hiOp, loOp;

    ip= g->mIP - 1;
    hiOp = (g->mToken >> 24) & 0x7F;
    loOp = g->mToken & 0xFFFFFF;
    if ( hiOp >= (sizeof(opTypeNames) / sizeof(char *)) ) {
        TRACE( "# BadOpType 0x%x  @ IP = 0x%x\n", g->mToken, ip );
    } else {
        switch( hiOp ){
            
        case kOpBuiltIn:
            if ( loOp != OP_DONE ) {
                TRACE( "# %s    0x%x   @ IP = 0x%x\n", baseDict[loOp].name, g->mToken, ip );
            }
            break;
            
        case kOpString:
            TRACE( "# \"%s\" 0x%x   @ IP = 0x%x\n", (char *)(g->mIP), g->mToken, ip );
            break;
            
        case kOpConstant:
        case kOpBranch:   case kOpBranchNZ:  case kOpBranchZ:
            if ( loOp & 0x800000 ) {
                loOp |= 0xFF000000;
            }
            TRACE( "# %s    %d   @ IP = 0x%x\n", opTypeNames[hiOp], loOp, ip );
            break;
            
        default:
            TRACE( "# %s    0x%x   @ IP = 0x%x\n", opTypeNames[hiOp], g->mToken, ip );
            break;
        }
    }
}

//############################################################################
//
//          I N N E R    I N T E R P R E T E R
//
//############################################################################


eForthResult
ForthEngine::InnerInterpreter( ForthThread  *g )
{
    ulong loOp, hiOp;
    ForthOp builtinOp;
    
    g->mState = kResultOk;
    
    while ( g->mState == kResultOk ) {
        // fetch token at IP, advance IP
        g->mToken = *g->mIP++;
        TraceOp( g );
        hiOp = (g->mToken >> 24) & 0x7F;
        loOp = g->mToken & 0xFFFFFF;
        if ( hiOp == kOpBuiltIn ) {
            // built in simple op
            // look up routine in table of built-in ops & execute
            if ( loOp < mNumBuiltinOps ) {
                builtinOp = (ForthOp) mpOpTable[loOp];
                builtinOp( g );
            } else {
                g->SetError( kForthErrorBadOpcode );
            }
        } else if ( hiOp == kOpUserDef ) {
            // token is normal user-defined, push IP on rstack, lookup new IP
            //  in table of user-defined ops
            if ( loOp < mNumOps ) {
                g->RPush( (long) g->mIP );
                g->mIP = mpOpTable[loOp];
            } else {
                g->SetError( kForthErrorBadOpcode );
            }
        } else {
            // TBD: trap hiOp out of range
            // op which takes 24-bit immediate data
            immDataFuncs[hiOp]( g, loOp );
        }
        
    }

    return g->mState;
}

char *
ForthEngine::GetLastInputToken( void )
{
    return mpLastToken;
}


//############################################################################
//
//          O U T E R    I N T E R P R E T E R  (sort of)
//
//############################################################################


// return true to exit forth shell
eForthResult
ForthEngine::ProcessToken( ForthThread    *g,
                           char           *pToken,
                           int            parseFlags )
{
    long *pSymbol, value;
    int len;
    eForthResult exitStatus = kResultOk;
    float fvalue;

    mpLastToken = pToken;
    if ( pToken == NULL ) {
        return kResultOk;
    }
    
    TRACE( "%s [%s] flags[%x]\t", mCompileState ? "Compile" : "Interpret", pToken, parseFlags );
    if ( parseFlags & PARSE_FLAG_QUOTED_STRING ) {
        //
        // symbol is a quoted string - the quotes have already been stripped
        //
        TRACE( "String[%s] flags[%x]\n", pToken, parseFlags );
        len = strlen( pToken );
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
        
    } else if ( parseFlags & PARSE_FLAG_QUOTED_CHARACTER ) {
        //
        // symbol is a quoted character - the quotes have already been stripped
        //
        TRACE( "Character[%s] flags[%x]\n", pToken, parseFlags );
        value = *pToken & 0xFF;
        if ( mCompileState ) {
            *mpDP++ = value | (kOpConstant << 24);
        } else {
            // in interpret mode, stick the character on the stack
            g->Push( value );
        }
        return kResultOk;
    }
    
    // check for local variables
    if ( mCompileState) {
        pSymbol = (long *) mpLocalVocab->FindSymbol( pToken );
        if ( pSymbol ) {
            mLastCompiledOpcode = *pSymbol;
            *mpDP++ = mLastCompiledOpcode;
            return kResultOk;
        }
    }
    
    pSymbol = (long *) mpCurrentVocab->FindSymbol( pToken );
    
    if ( pSymbol ) {
        TRACE( "Op\n" );
        // the symbol is in the vocabulary
        // TBD: deal with different types of symbols
        value = (*pSymbol) >> 24;
        // value holds the symbol type - types > 127 have precedence
        if ( mCompileState && ((value & 0x80) == 0) ) {
            // compile the opcode
            // remember the last opcode compiled so someday we can do optimizations
            //   like combining "->" followed by a local var name into one opcode
            mLastCompiledOpcode = *pSymbol;
            *mpDP++ = mLastCompiledOpcode;
        } else {
            // execute the opcode
            mpEngineScratch[0] = *pSymbol;
            mpEngineScratch[1] = BUILTIN_OP( OP_DONE );

            g->SetIP( mpEngineScratch );
            exitStatus = InnerInterpreter( g );
            if ( exitStatus == kResultDone ) {
                exitStatus = kResultOk;
            }
        }
    } else {
        // try to convert to a number
        // if there is a period in string
        //    try to covert to a floating point number
        // else
        //    try to convert to an integer
        // TBD: support 0x 0X 0b 0B prefixes
        // TBD: support floating point literals
        if ( (strchr( pToken, '.' ) != NULL)
            && (sscanf( pToken, "%f", &fvalue ) == 1) ) {
            // this is a single precision fp literal
            TRACE( "Floating point literal %f\n", fvalue );
            if ( mCompileState ) {
                // compile the literal value
                // value too big, must go in next longword
                *mpDP++ = OP_FLOAT_VAL;
                *(float *) mpDP++ = fvalue;
            } else {
                g->FPush( fvalue );
            }
            
        } else if ( sscanf( pToken, "%d", &value ) == 1 ) {

            // this is an integer literal
            TRACE( "Integer literal %d\n", value );
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
        
    }

    // TBD: return exit-shell flag
    return exitStatus;
}

