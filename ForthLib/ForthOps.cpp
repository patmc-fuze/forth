//////////////////////////////////////////////////////////////////////
//
// ForthOps.cpp: forth builtin operator definitions
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"


#ifdef ARM9
#include <nds.h>
#endif

#include <ctype.h>
#include <time.h>
#if defined(WIN32)
#include <sys/timeb.h>
#endif
#include "Forth.h"
#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthVocabulary.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include "ForthStructs.h"
#include "ForthServer.h"
#include "ForthClient.h"
#include "ForthMessages.h"

#ifdef LINUX
#include <strings.h>
#endif

long gCompiledOps[NUM_COMPILED_OPS];

extern "C"
{

// compiled token is 32-bits,
// top 8 bits are opcode type (opType)
// bottom 24 bits are immediate operand

extern void intVarAction( ForthCoreState* pCore, int* pVar );
extern GFORTHOP( byteVarActionOp );
extern GFORTHOP( ubyteVarActionOp );
extern GFORTHOP( shortVarActionOp );
extern GFORTHOP( ushortVarActionOp );
extern GFORTHOP( intVarActionOp );
extern GFORTHOP( longVarActionOp );
extern GFORTHOP( floatVarActionOp );
extern GFORTHOP( doubleVarActionOp );
extern GFORTHOP( stringVarActionOp );
extern GFORTHOP( opVarActionOp );
extern GFORTHOP( objectVarActionOp );

//############################################################################
//
//   normal forth ops
//
//############################################################################

// bye is a user command which causes the entire forth program to exit
FORTHOP( byeOp )
{
    SET_STATE( kResultExitShell );
}

// badOp is used to detect executing uninitialized op variables
FORTHOP( badOpOp )
{
    SET_ERROR( kForthErrorBadOpcode );
}

// unimplementedMethodOp is used to detect executing unimplemented methods
FORTHOP( unimplementedMethodOp )
{
    SET_ERROR( kForthErrorUnimplementedMethod );
}

FORTHOP( argvOp )
{
    NEEDS( 1 );
    SPUSH( (long) (GET_ENGINE->GetShell()->GetArg( SPOP )) );
}

FORTHOP( argcOp )
{
    SPUSH( GET_ENGINE->GetShell()->GetArgCount() );
}

//##############################
//
// math ops
//

FORTHOP( ldivideOp )
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    LPUSH( a / b );
}

FORTHOP( lmodOp )
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    LPUSH( a % b );
}

FORTHOP( ldivmodOp )
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    LPUSH( a % b );
    LPUSH( a / b );
}

FORTHOP( udivmodOp )
{
    NEEDS(3);
    unsigned long b = (unsigned long) SPOP;
    unsigned long long a = LPOP;
    unsigned long long quotient = a / b;
	unsigned long remainder = a % b;
    SPUSH( ((long) remainder) );
    LPUSH( ((long long) quotient) );
}

//##############################
//
// double-precision fp comparison ops
//

FORTHOP(lEqualsOp)
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(lNotEqualsOp)
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(lGreaterThanOp)
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(lGreaterEqualsOp)
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(lLessThanOp)
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(lLessEqualsOp)
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(lEqualsZeroOp)
{
    NEEDS(2);
    long long a = LPOP;
    SPUSH( ( a == 0 ) ? -1L : 0 );
}

FORTHOP(lNotEqualsZeroOp)
{
    NEEDS(2);
    long long a = LPOP;
    SPUSH( ( a != 0 ) ? -1L : 0 );
}

FORTHOP(lGreaterThanZeroOp)
{
    NEEDS(2);
    long long a = LPOP;
    SPUSH( ( a > 0 ) ? -1L : 0 );
}

FORTHOP(lGreaterEqualsZeroOp)
{
    NEEDS(2);
    long long a = LPOP;
    SPUSH( ( a >= 0 ) ? -1L : 0 );
}

FORTHOP(lLessThanZeroOp)
{
    NEEDS(2);
    long long a = LPOP;
    SPUSH( ( a < 0 ) ? -1L : 0 );
}

FORTHOP(lLessEqualsZeroOp)
{
    NEEDS(2);
    long long a = LPOP;
    SPUSH( ( a <= 0 ) ? -1L : 0 );
}

FORTHOP(lWithinOp)
{
    NEEDS(3);
    long long hiLimit = LPOP;
    long long loLimit = LPOP;
    long long val = LPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(lMinOp)
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    LPUSH( ( a < b ) ? a : b );
}

FORTHOP(lMaxOp)
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    LPUSH( ( a > b ) ? a : b );
}



FORTHOP( i2lOp )
{
    NEEDS(1);
    int a = SPOP;
    long long b = (long long) a;
    LPUSH( b );
}

FORTHOP( l2fOp )
{
    NEEDS(1);
    long long a = LPOP;
    float b = (float) a;
    FPUSH( b );
}

FORTHOP( l2dOp )
{
    NEEDS(1);
    long long a = LPOP;
    double b = (double) a;
    DPUSH( b );
}

FORTHOP( f2lOp )
{
    NEEDS(1);
    float a = FPOP;
    long long b = (long long) a;
    LPUSH( b );
}

FORTHOP( d2lOp )
{
    NEEDS(2);
    double a = DPOP;
    long long b = (long long) a;
    LPUSH( b );
}

//##############################
//
// control ops
//
// TBD: replace branch, tbranch, fbranch with immediate ops

// has precedence!
FORTHOP(doOp)
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShellStack *pShellStack = pEngine->GetShell()->GetShellStack();
    pShellStack->Push( gCompiledOps[ OP_DO_DO ] );
    // save address for loop/+loop
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagDo );
    // this will be fixed by loop/+loop
    pEngine->CompileBuiltinOpcode( OP_ABORT );
    pEngine->CompileLong( 0 );
}

// has precedence!
FORTHOP(checkDoOp)
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShellStack *pShellStack = pEngine->GetShell()->GetShellStack();
    pShellStack->Push( gCompiledOps[ OP_DO_CHECKDO ] );
    // save address for loop/+loop
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagDo );
    // this will be fixed by loop/+loop
    pEngine->CompileBuiltinOpcode( OP_ABORT );
    pEngine->CompileLong( 0 );
}

// has precedence!
FORTHOP(loopOp)
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "loop", pShellStack->Pop(), kShellTagDo ) )
    {
        return;
    }
    long *pDoOp = (long *) pShellStack->Pop();
	long doOpcode = pShellStack->Pop();
    *pDoOp++ = doOpcode;
    // compile the "_loop" opcode
    pEngine->CompileBuiltinOpcode( OP_DO_LOOP );
    // fill in the branch to after loop opcode
    *pDoOp = COMPILED_OP( kOpBranch, (GET_DP - pDoOp) - 1 );
}

// has precedence!
FORTHOP(loopNOp)
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "loop+", pShellStack->Pop(), kShellTagDo ) )
    {
        return;
    }
    long *pDoOp = (long *) pShellStack->Pop();
	long doOpcode = pShellStack->Pop();
    *pDoOp++ = doOpcode;
    // compile the "_loop" opcode
    pEngine->CompileBuiltinOpcode( OP_DO_LOOPN );
    // fill in the branch to after loop opcode
    *pDoOp = COMPILED_OP( kOpBranch, (GET_DP - pDoOp) - 1 );
}

// if - has precedence
FORTHOP( ifOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // save address for else/endif
    pShellStack->Push( (long)GET_DP );
    // flag that this is the "if" branch
    pShellStack->Push( kShellTagIf );
    // this will be fixed by else/endif
    pEngine->CompileBuiltinOpcode( OP_ABORT );
}


// else - has precedence
FORTHOP( elseOp )
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "else", pShellStack->Pop(), kShellTagIf ) )
    {
        return;
    }
    long *pIfOp = (long *) pShellStack->Pop();
    // save address for endif
    pShellStack->Push( (long) GET_DP );
    // flag that this is the "else" branch
    pShellStack->Push( kShellTagElse );
    // this will be fixed by endif
    pEngine->CompileBuiltinOpcode( OP_ABORT );
    // fill in the branch taken when "if" arg is false
    *pIfOp = COMPILED_OP( kOpBranchZ, (GET_DP - pIfOp) - 1 );
}


// endif - has precedence
FORTHOP( endifOp )
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    long tag = pShellStack->Pop();
    long *pOp = (long *) pShellStack->Pop();
    if ( tag == kShellTagElse )
    {
        // else branch
        // fill in the branch at end of path taken when "if" arg is true
        *pOp = COMPILED_OP( kOpBranch, (GET_DP - pOp) - 1 );
    }
    else if ( pShell->CheckSyntaxError( "endif", tag, kShellTagIf ) )
    {
        // if branch
        // fill in the branch taken when "if" arg is false
        *pOp = COMPILED_OP( kOpBranchZ, (GET_DP - pOp) - 1 );
    }
}



// begin - has precedence
FORTHOP( beginOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // save address for repeat/until/again
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagBegin );
}


// until - has precedence
FORTHOP( untilOp )
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "until", pShellStack->Pop(), kShellTagBegin ) )
    {
        return;
    }
    long *pBeginOp =  (long *) pShellStack->Pop();
    pEngine->CompileOpcode( COMPILED_OP( kOpBranchZ, (pBeginOp - GET_DP) - 1 ) );
}



// while - has precedence
FORTHOP( whileOp )
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "while", pShellStack->Pop(), kShellTagBegin ) )
    {
        return;
    }
    pShellStack->Push( (long) GET_DP );
    pShellStack->Push( kShellTagWhile );
    // repeat will fill this in
    pEngine->CompileBuiltinOpcode( OP_ABORT );
}


// repeat - has precedence
FORTHOP( repeatOp )
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "repeat", pShellStack->Pop(), kShellTagWhile ) )
    {
        return;
    }
    // get address of "while"
    long *pOp =  (long *) pShellStack->Pop();
    *pOp = COMPILED_OP( kOpBranchZ, (GET_DP - pOp) );
    // get address of "begin"
    pOp =  (long *) pShellStack->Pop();
    pEngine->CompileOpcode( COMPILED_OP( kOpBranch, (pOp - GET_DP) - 1 ) );
}

// again - has precedence
FORTHOP( againOp )
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "again", pShellStack->Pop(), kShellTagBegin ) )
    {
        return;
    }
    long *pBeginOp =  (long *) pShellStack->Pop();
    pEngine->CompileOpcode( COMPILED_OP( kOpBranch, (pBeginOp - GET_DP) - 1 ) );
}

// case - has precedence
FORTHOP( caseOp )
{
    // leave marker for end of list of case-exit branches for endcase
   ForthEngine *pEngine = GET_ENGINE;
   ForthShell *pShell = pEngine->GetShell();
   ForthShellStack *pShellStack = pShell->GetShellStack();
   pShellStack->Push( 0 );
   pShellStack->Push( kShellTagCase );
}


// of - has precedence
FORTHOP( ofOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "of", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }

    // save address for endof
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagOf );
    pShellStack->Push( kShellTagCase );
    // this will be set to a caseBranch by endof
    pEngine->CompileBuiltinOpcode( OP_ABORT );
}


// ofif - has precedence
FORTHOP( ofifOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "ofif", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }

    // save address for endof
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagOfIf );
    pShellStack->Push( kShellTagCase );
    // this will be set to a zBranch by endof
    pEngine->CompileBuiltinOpcode( OP_ABORT );
	// if the ofif test succeeded, we need to dispose of the switch input value
    pEngine->CompileBuiltinOpcode( OP_DROP );
}


// endof - has precedence
FORTHOP( endofOp )
{
    NEEDS(1);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    long *pDP = GET_DP;

    // this will be fixed by endcase
    pEngine->CompileBuiltinOpcode( OP_ABORT );

    if ( !pShell->CheckSyntaxError( "endof", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }
    long tag = pShellStack->Pop();
    long *pOp = (long *) pShellStack->Pop();
    // fill in the branch taken when case doesn't match
	if ( tag == kShellTagOfIf )
	{
        *pOp = COMPILED_OP( kOpBranchZ, (GET_DP - pOp) - 1 );
	}
	else if ( pShell->CheckSyntaxError( "endof", tag, kShellTagOf ) )
	{
	    *pOp = COMPILED_OP( kOpCaseBranch, (GET_DP - pOp) - 1 );
	}
	else
	{
		return;
	}

    // save address for endcase
    pShellStack->Push( (long) pDP );
    pShellStack->Push( kShellTagCase );
}


// endcase - has precedence
FORTHOP( endcaseOp )
{
    long *pSP = GET_SP;
    long *pEndofOp;

    ForthEngine *pEngine = GET_ENGINE;
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "endcase", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }

    if ( ((GET_DP) - (long *)(pShellStack->Peek())) == 1 )
    {
        // there is no default case, we must compile a "drop" to
        //   dispose of the case selector on TOS
        pEngine->CompileBuiltinOpcode( OP_DROP );
    }
    // patch branches from end-of-case to common exit point
    while ( (pEndofOp = (long *) (pShellStack->Pop())) != NULL )
    {
        *pEndofOp = COMPILED_OP( kOpBranch, (GET_DP - pEndofOp) - 1 );
    }
    SET_SP( pSP );
}

// align (upwards) DP to longword boundary
FORTHOP( alignOp )
{
    GET_ENGINE->AlignDP();
}        

FORTHOP( allotOp )
{
    GET_ENGINE->AllotLongs( SPOP );
}        

FORTHOP( commaOp )
{
    GET_ENGINE->CompileLong( SPOP );
}        

FORTHOP( cCommaOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pChar = (char *)GET_DP;
    *pChar++ = (char) SPOP;
    pEngine->SetDP( (long *) pChar);
}        

FORTHOP( hereOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pChar = (char *)GET_DP;

	switch( GET_VAR_OPERATION )
	{
		case kVarDefaultOp:
		case kVarFetch:
		    SPUSH( (long) pChar );
			break;
		case kVarStore:
		    pEngine->SetDP( (long *) (SPOP) );
			break;
		case kVarPlusStore:
			pChar += SPOP;
		    pEngine->SetDP( (long *) pChar );
			break;
		case kVarMinusStore:
			pChar -= SPOP;
		    pEngine->SetDP( (long *) pChar );
			break;
		case kVarRef:
		    SPUSH( (long) &(pCore->pDictionary->pCurrent) );
			break;
		default:
            SET_ERROR( kForthErrorBadVarOperation );
	}
	CLEAR_VAR_OPERATION;
}

FORTHOP( mallocOp )
{
    NEEDS(1);
    long numBytes = SPOP;
    void* pMemory = malloc( numBytes );
    SPUSH( (long) pMemory  );
}

FORTHOP( reallocOp )
{
    NEEDS(2);
    size_t newSize = (size_t)(SPOP);
    void* allocPtr = (void *)(SPOP);
    SPUSH(  (long) realloc( allocPtr, newSize )  );
}

FORTHOP( freeOp )
{
    NEEDS(1);
    free( (void *) SPOP );
}

FORTHOP( removeEntryOp )
{
    SET_VAR_OPERATION( kVocabRemoveEntry );
}

FORTHOP( entryLengthOp )
{
    SET_VAR_OPERATION( kVocabEntryLength );
}

FORTHOP( numEntriesOp )
{
    SET_VAR_OPERATION( kVocabNumEntries );
}

FORTHOP( vocabToClassOp )
{
    SET_VAR_OPERATION( kVocabGetClass );
}

FORTHOP( initStringArrayOp )
{
    // TOS: ptr to first char of first element, maximum length, number of elements
    long len, nLongs;
    long* pStr;
    int i, numElements;

    pStr = ((long *) (SPOP)) - 2;
    len = SPOP;
    numElements = SPOP;
    nLongs = (len >> 2) + 3;

    for ( i = 0; i < numElements; i++ )
    {
        *pStr = len;
        pStr[1] = 0;
        *((char *) (pStr + 2)) = 0;
        pStr += nLongs;
    }
}

//##############################
//
//  forth defining ops
//

static long    *gpSavedDP;

// builds
// - does not have precedence
// - is executed while executing the defining word
// - begins definition of new symbol
//
FORTHOP(buildsOp)
{
    ForthEngine *pEngine = GET_ENGINE;
	long *pEntry;

    // get next symbol, add it to vocabulary with type "builds/does"
    pEngine->AlignDP();
    pEngine->AddUserOp( pEngine->GetNextSimpleToken(), &pEntry, true );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    // remember current DP (for does)
    gpSavedDP = GET_DP;
    // compile dummy word at DP, will be filled in by does
    pEngine->CompileLong( 0 );
}

// does
// - has precedence
// - is executed while compiling the defining word
// - it completes the definition of the builds part of the word, and starts
//   the definition of the does part of the word
//
FORTHOP( doesOp )
{
    long newUserOp;
    ForthEngine *pEngine = GET_ENGINE;
    
    // compile dodoes opcode & dummy word
    pEngine->CompileBuiltinOpcode( OP_END_BUILDS );
    pEngine->CompileLong( 0 );
    // create a nameless vocabulary entry for does-body opcode
    newUserOp = pEngine->AddOp( GET_DP );
    newUserOp = COMPILED_OP( kOpUserDef, newUserOp );
    pEngine->CompileBuiltinOpcode( OP_DO_DOES );
    // stuff does-body opcode in dummy word
    GET_DP[-2] = newUserOp;
    // compile local vars allocation op (if needed)
    pEngine->EndOpDefinition();
}

// endBuilds
// - does not have precedence
// - is executed while executing the defining word
// - it completes the definition of the new symbol created by builds by compiling
//   the opcode (defined by does) into the newly defined word
//
FORTHOP( endBuildsOp )
{
    // finish current symbol definition (of op defined by builds)
    GET_ENGINE->GetDefinitionVocabulary()->UnSmudgeNewestSymbol();
    
    // fetch opcode at pIP, compile it into dummy word remembered by builds
    *gpSavedDP = *GET_IP;
    // we are done defining, bail out
    SET_IP( (long *) (RPOP) );
}

// exit has precedence
FORTHOP( exitOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // compile exitOp
    long flags = pEngine->GetFlags();

    switch ( flags & (kEngineFlagHasLocalVars | kEngineFlagIsMethod) )
    {
    case 0:
        // normal definition, no local vars, not a method
        pEngine->CompileBuiltinOpcode( OP_DO_EXIT );
        break;
    case kEngineFlagHasLocalVars:
        // normal definition with local vars
        pEngine->CompileBuiltinOpcode( OP_DO_EXIT_L );
        break;
    case kEngineFlagIsMethod:
        // method definition, no local vars
        pEngine->CompileBuiltinOpcode( OP_DO_EXIT_M );
        break;
    case (kEngineFlagHasLocalVars | kEngineFlagIsMethod):
        // method definition, with local vars
        pEngine->CompileBuiltinOpcode( OP_DO_EXIT_ML );
        break;
    }
}

// semi has precedence
FORTHOP( semiOp )
{
    ForthEngine *pEngine = GET_ENGINE;

    exitOp( pCore );
    // switch back from compile mode to execute mode
    pEngine->SetCompileState( 0 );
    pEngine->ClearFlag( kEngineFlagHasLocalVars );
    // finish current symbol definition
    // compile local vars allocation op (if needed)
    pEngine->EndOpDefinition( true );
}

FORTHOP( colonOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    long* pEntry = pEngine->StartOpDefinition( NULL, true );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    // switch to compile mode
    pEngine->SetCompileState( 1 );
    pEngine->ClearFlag( kEngineFlagHasLocalVars );
}

FORTHOP( codeOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    long* pEntry = pEngine->StartOpDefinition( NULL, false );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    long newestOp = *pEntry;
    *pEntry = COMPILED_OP( kOpNative, FORTH_OP_VALUE( newestOp ) );
}

FORTHOP( createOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    long* pEntry = pEngine->StartOpDefinition( NULL, false );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_VAR );
}

FORTHOP( forthVocabOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->GetForthVocabulary()->DoOp( pCore );
}

FORTHOP( definitionsOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pEngine->SetDefinitionVocabulary( pVocabStack->GetTop() );
}

FORTHOP( alsoOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pVocabStack->DupTop();
}

FORTHOP( previousOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    if ( !pVocabStack->DropTop() )
    {
        CONSOLE_STRING_OUT( "Attempt to drop last item on vocabulary stack ignored.\n" );
    }
}

FORTHOP( onlyOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pVocabStack->Clear();
}

FORTHOP( vocabularyOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabulary *pDefinitionsVocab = pEngine->GetDefinitionVocabulary();
    // get next symbol, add it to vocabulary with type "user op"
    char* pVocabName = pEngine->GetNextSimpleToken();
    long* pEntry = pEngine->StartOpDefinition( pVocabName );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_VOCAB );
    ForthVocabulary* pVocab = new ForthVocabulary( pVocabName,
                                                   NUM_FORTH_VOCAB_VALUE_LONGS,
                                                   512,
                                                   GET_DP,
                                                   ForthVocabulary::GetEntryValue( pDefinitionsVocab->GetNewestEntry() ) );
    pEngine->CompileLong( (long) pVocab );
}

FORTHOP( forgetOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char* pSym = pEngine->GetNextSimpleToken();
    pEngine->ForgetSymbol( pSym, false );
    // reset search & definitions vocabs in case we deleted a vocab we were using
    pEngine->SetDefinitionVocabulary( pEngine->GetForthVocabulary() );
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pVocabStack->Clear();
}

// just like forget, but no error message if symbol not found
FORTHOP( autoforgetOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->ForgetSymbol( pEngine->GetNextSimpleToken(), true );
    // reset search & definitions vocabs in case we deleted a vocab we were using
    pEngine->SetDefinitionVocabulary( pEngine->GetForthVocabulary() );
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pVocabStack->Clear();
}

#define SCREEN_COLUMNS 120

// return 'q' IFF user quit out
static char
ShowVocab( ForthCoreState   *pCore,
           ForthVocabulary  *pVocab,
           bool             quick )
{
    int i;
    char retVal = 0;
    ForthShell *pShell = GET_ENGINE->GetShell();
    int nEntries = pVocab->GetNumEntries();
    long *pEntry = pVocab->GetFirstEntry();
#define ENTRY_COLUMNS 16
    char spaces[ENTRY_COLUMNS + 1];
    int column = 0;
#define BUFF_SIZE 256
    char buff[BUFF_SIZE];

    memset( spaces, ' ', ENTRY_COLUMNS );
    spaces[ENTRY_COLUMNS] = '\0';

    for ( i = 0; i < nEntries; i++ )
    {
        bool lastEntry = (i == (nEntries-1));
        if ( quick )
        {
            int nameLength = pVocab->GetEntryNameLength( pEntry );
            int numSpaces = ENTRY_COLUMNS - (nameLength % ENTRY_COLUMNS);
            if ( (column + nameLength) > SCREEN_COLUMNS )
            {
                CONSOLE_STRING_OUT( "\n" );
                column = 0;
            }
            pVocab->GetEntryName( pEntry, buff, sizeof(buff) );
            CONSOLE_STRING_OUT( buff );
            column += (nameLength + numSpaces);
            if ( column >= SCREEN_COLUMNS )
            {
                CONSOLE_STRING_OUT( "\n" );
                column = 0;
            }
            else
            {
                CONSOLE_STRING_OUT( spaces + (ENTRY_COLUMNS - numSpaces) );
            }
            if ( lastEntry )
            {
                CONSOLE_STRING_OUT( "\n" );
            }
            else
            {
                pEntry = pVocab->NextEntry( pEntry );
            }
        }
        else
        {
            pVocab->PrintEntry( pEntry );
            CONSOLE_STRING_OUT( "\n" );
            pEntry = pVocab->NextEntry( pEntry );
            if ( ((i % 22) == 21) || lastEntry )
            {
                if ( (pShell != NULL) && pShell->GetInput()->InputStream()->IsInteractive() )
                {
                    CONSOLE_STRING_OUT( "\nHit ENTER to continue, 'q' & ENTER to quit, 'n' & ENTER to do next vocabulary\n" );
                    retVal = tolower( pShell->GetChar() );
                    if ( retVal == 'q' )
                    {
                        pShell->GetChar();
                        break;
                    }
                    else if ( retVal == 'n' )
                    {
                        pShell->GetChar();
                        break;
                    }
                }
            }
        }
    }

    return retVal;
}


FORTHOP( vlistOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    bool quit = false;
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    ForthVocabulary* pVocab;
    int depth = 0;
	bool verbose = (GET_VAR_OPERATION != kVarDefaultOp);
    CONSOLE_STRING_OUT( "vocab stack:" ); 
    while ( true )
    {
        pVocab = pVocabStack->GetElement( depth );
        if ( pVocab == NULL )
        {
            break;
        }
        CONSOLE_STRING_OUT( " " );
        CONSOLE_STRING_OUT( pVocab->GetName() );
        depth++;
    }
    CONSOLE_STRING_OUT( "\n" );
    CONSOLE_STRING_OUT( "definitions vocab: " );
    CONSOLE_STRING_OUT( pEngine->GetDefinitionVocabulary()->GetName() );
    CONSOLE_STRING_OUT( "\n" );
    depth = 0;
    while ( !quit )
    {
        pVocab = pVocabStack->GetElement( depth );
        if ( pVocab == NULL )
        {
            return;
        }
        CONSOLE_STRING_OUT( pVocab->GetName() );
        CONSOLE_STRING_OUT( " vocabulary:\n" );
        quit = ( ShowVocab( pCore, pVocab, !verbose ) == 'q' );
        depth++;
    }
	CLEAR_VAR_OPERATION;
}

FORTHOP( findOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabulary* pVocab = pEngine->GetVocabularyStack()->GetTop();

    char *pSymbol = (char *) (SPOP);
    long* pEntry = pVocab->FindSymbol( pSymbol );
    SPUSH( ((long) pEntry) );
}


FORTHOP( variableOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_VAR );
    pEngine->CompileLong( 0 );
}

FORTHOP( doVariableOp )
{
    // IP points to data field
    SPUSH( (long) GET_IP );
    SET_IP( (long *) (RPOP) );
}

FORTHOP( constantOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_CONSTANT );
    pEngine->CompileLong( SPOP );
}

FORTHOP( dconstantOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_DCONSTANT );
    double d = DPOP;
    pEngine->CompileDouble( d );
}

FORTHOP( byteOp )
{
    char val = 0;
	gNativeTypeByte.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( ubyteOp )
{
    char val = 0;
	gNativeTypeUByte.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( shortOp )
{
    short val = 0;
	gNativeTypeShort.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( ushortOp )
{
    short val = 0;
	gNativeTypeUShort.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( intOp )
{
    int val = 0;
	gNativeTypeInt.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( uintOp )
{
    int val = 0;
	gNativeTypeUInt.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( longOp )
{
    long long val = 0;
	gNativeTypeLong.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( ulongOp )
{
    long long val = 0;
	gNativeTypeULong.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( floatOp )
{
    float val = 0.0;
	gNativeTypeFloat.DefineInstance( GET_ENGINE, &val );
}


FORTHOP( doubleOp )
{
    double val = 0.0;
	gNativeTypeDouble.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( stringOp )
{
    gNativeTypeString.DefineInstance( GET_ENGINE, NULL );
}

FORTHOP( opOp )
{
    int val = gCompiledOps[ OP_BAD_OP] ;
	gNativeTypeOp.DefineInstance( GET_ENGINE, &val );
}

//FORTHOP( objectOp )
//{
//    long val[2] = { 0, 0  };
//	gNativeTypeObject.DefineInstance( GET_ENGINE, val );
//}

FORTHOP( voidOp )
{
}

FORTHOP( arrayOfOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long numElements;

    if ( pEngine->IsCompiling() )
    {
        // the symbol just before "arrayOf" should have been an integer constant
        if ( pEngine->GetLastConstant( numElements ) )
        {
            // uncompile the integer contant opcode
            pEngine->UncompileLastOpcode();
            // save #elements for var declaration ops
            pEngine->SetArraySize( numElements );
        }
        else
        {
            SET_ERROR( kForthErrorMissingSize );
        }
    }
    else
    {
        // save #elements for var declaration ops
        numElements = SPOP;
        pEngine->SetArraySize( numElements );
    }
}

FORTHOP( ptrToOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->SetFlag( kEngineFlagIsPointer );
}

FORTHOP( structOp )
{
    ForthEngine* pEngine = GET_ENGINE;
    pEngine->SetFlag( kEngineFlagInStructDefinition );
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthStructVocabulary* pVocab = pManager->StartStructDefinition( pEngine->GetNextSimpleToken() );
    pEngine->CompileBuiltinOpcode( OP_DO_STRUCT_TYPE );
    pEngine->CompileLong( (long) pVocab );
}

FORTHOP( endstructOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->ClearFlag( kEngineFlagInStructDefinition );
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    pManager->EndStructDefinition();
}

FORTHOP( classOp )
{
    ForthEngine* pEngine = GET_ENGINE;
    pEngine->StartClassDefinition( pEngine->GetNextSimpleToken() );
}

FORTHOP( endclassOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->EndClassDefinition();
}

FORTHOP( methodOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    const char* pMethodName = pEngine->GetNextSimpleToken();
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->GetNewestClass();

    long methodIndex = pVocab->FindMethod( pMethodName );
    pEngine->StartOpDefinition( pMethodName, true );
    // switch to compile mode
    pEngine->SetCompileState( 1 );
    pEngine->ClearFlag( kEngineFlagHasLocalVars );
    pEngine->SetFlag( kEngineFlagIsMethod );
    if ( pVocab )
    {
        long* pEntry = pVocab->GetNewestEntry();
        if ( pEntry )
        {
            methodIndex = pVocab->AddMethod( pMethodName, methodIndex, pEntry[0] );
            pEntry[0] = methodIndex;
            pEntry[1] |= kDTIsMethod;
        }
        else
        {
            // TBD: error
        }
    }
    else
    {
        // TBD: report adding a method outside a class definition
    }
}

FORTHOP( endmethodOp )
{
    // TBD
    ForthEngine *pEngine = GET_ENGINE;

    exitOp( pCore );
    // switch back from compile mode to execute mode
    pEngine->SetCompileState( 0 );
    pEngine->ClearFlag( kEngineFlagHasLocalVars );
    // finish current symbol definition
    // compile local vars allocation op (if needed)
    pEngine->EndOpDefinition( true );
    pEngine->ClearFlag( kEngineFlagIsMethod );
}

FORTHOP( returnsOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    if ( pEngine->CheckFlag( kEngineFlagIsMethod ) )
    {
        char *pToken = pEngine->GetNextSimpleToken();
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthClassVocabulary* pVocab = pManager->GetNewestClass();
        long* pEntry = pVocab->GetNewestEntry();
        if ( strcmp( pToken, "ptrTo" ) == 0 )
        {
            pToken = pEngine->GetNextSimpleToken();
            pEntry[1] |= kDTIsPtr;
        }
        forthBaseType baseType = pManager->GetBaseTypeFromName( pToken );
        if ( baseType != kBaseTypeUnknown )
        {
            pEntry[1] |= NATIVE_TYPE_TO_CODE( 0, baseType );
        }
        else
        {
            // see if it is a struct type
            ForthStructVocabulary* pTypeVocab = pManager->GetStructVocabulary( pToken );
            if ( pTypeVocab )
            {
                if ( pTypeVocab->IsClass() )
                {
                    pEntry[1] |= OBJECT_TYPE_TO_CODE( 0, pTypeVocab->GetTypeIndex() );
                }
                else
                {
                    pEntry[1] |= STRUCT_TYPE_TO_CODE( 0, pTypeVocab->GetTypeIndex() );
                }
            }
            else
            {
                pEngine->AddErrorText( pToken );
                pEngine->SetError( kForthErrorUnknownSymbol, " is not a known return type" );
            }
        }
    }
    else
    {
        // report error - "returns" outside of method
        pEngine->SetError( kForthErrorBadSyntax, "returns can only be used inside a method" );
    }
}

FORTHOP( doMethodOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long methodIndex = SPOP;
    RPUSH( ((long) GET_TPD) );
    RPUSH( ((long) GET_TPM) );
    long* pMethods = ((long *) (SPOP));
    SET_TPM( pMethods );
    SET_TPD( ((long *) (SPOP)) );
    pEngine->ExecuteOneOp( pMethods[ methodIndex ] );
}

FORTHOP( implementsOp )
{
    // TBD
    ForthEngine *pEngine = GET_ENGINE;

    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->GetNewestClass();
    if ( pVocab )
    {
        pVocab->Implements( pEngine->GetNextSimpleToken() );
    }
    else
    {
        // TBD: report error - implements in struct
    }
}

FORTHOP( endimplementsOp )
{
    // TBD
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->GetNewestClass();
    if ( pVocab )
    {
        pVocab->EndImplements();
    }
    else
    {
        // TBD: report error - ;implements in struct
    }
}

FORTHOP( unionOp )
{
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    pManager->GetNewestStruct()->StartUnion();
}

FORTHOP( extendsOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pSym = pEngine->GetNextSimpleToken();
    ForthVocabulary* pFoundVocab;
    long *pEntry = pEngine->GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );
    if ( pEntry )
    {
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthStructVocabulary* pParentVocab = pManager->GetStructVocabulary( pEntry[0] );
        if ( pParentVocab )
        {
			if ( pEngine->CheckFlag( kEngineFlagInClassDefinition ) )
			{
				if ( pParentVocab->IsClass() )
				{
					ForthClassVocabulary* pParentClassVocab = reinterpret_cast<ForthClassVocabulary *>(pParentVocab);
					pManager->GetNewestClass()->Extends( pParentClassVocab );
				}
				else
				{
					pEngine->SetError( kForthErrorBadSyntax, "extend class from struct is illegal" );
				}
			}
			else
			{
				if ( pEngine->CheckFlag( kEngineFlagInStructDefinition ) )
				{
					if ( pParentVocab->IsClass() )
					{
						pEngine->SetError( kForthErrorBadSyntax, "extend struct from class is illegal" );
					}
					else
					{
						pManager->GetNewestStruct()->Extends( pParentVocab );
					}
				}
				else
				{
					pEngine->SetError( kForthErrorBadSyntax, "extends must be used in a class or struct definition" );
				}
			}
        }
        else
        {
            pEngine->SetError( kForthErrorUnknownSymbol, pSym );
            pEngine->AddErrorText( " is not a structure" );
        }
    }
    else
    {
        pEngine->SetError( kForthErrorUnknownSymbol, pSym );
    }
}

FORTHOP( sizeOfOp )
{
    // TBD: allow sizeOf to be applied to variables
    // TBD: allow sizeOf to apply to native types, including strings
    ForthEngine *pEngine = GET_ENGINE;
    char *pSym = pEngine->GetNextSimpleToken();
    ForthVocabulary* pFoundVocab;
    long *pEntry = pEngine->GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );

    if ( pEntry )
    {
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthStructVocabulary* pStructVocab = pManager->GetStructVocabulary( pEntry[0] );
        if ( pStructVocab )
        {
            pEngine->ProcessConstant( pStructVocab->GetSize() );
        }
        else
        {
            long size = pManager->GetBaseTypeSizeFromName( pSym );
            if ( size > 0 )
            {
                pEngine->ProcessConstant( size );
            }
            else
            {
                pEngine->AddErrorText( pSym );
                pEngine->SetError( kForthErrorUnknownSymbol, " is not a structure or base type" );
            }
        }
    }
    else
    {
        pEngine->SetError( kForthErrorUnknownSymbol, pSym );
    }
}

FORTHOP( offsetOfOp )
{
    // TBD: allow offsetOf to be take variable.field instead of just type.field
    ForthEngine *pEngine = GET_ENGINE;
    char *pType = pEngine->GetNextSimpleToken();
    char *pField = strchr( pType, '.' );
    if ( pField == NULL )
    {
        pEngine->SetError( kForthErrorBadSyntax, "argument must contain a period" );
        return;
    }
    *pField++ = '\0';

    ForthVocabulary* pFoundVocab;
    long *pEntry = pEngine->GetVocabularyStack()->FindSymbol( pType, &pFoundVocab );
    if ( pEntry )
    {
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthStructVocabulary* pStructVocab = pManager->GetStructVocabulary( pEntry[0] );
        if ( pStructVocab )
        {
            pEntry = pStructVocab->FindSymbol( pField );
            if ( pEntry )
            {
                pEngine->ProcessConstant( pEntry[0] );
            }
            else
            {
                pEngine->AddErrorText( pField );
                pEngine->AddErrorText( " is not a field in " );
                pEngine->SetError( kForthErrorUnknownSymbol, pType );
            }
        }
        else
        {
            pEngine->AddErrorText( pType );
            pEngine->SetError( kForthErrorUnknownSymbol, " is not a structure" );
        }
    }
    else
    {
        pEngine->SetError( kForthErrorUnknownSymbol, pType );
    }
}

FORTHOP( superOp )
{
	// push version of this which has current objects data pointer and super class method pointer
    SPUSH( ((long) GET_TPD) );
	long* pMethods = GET_TPM;
	// the long before method 0 holds the class object pointer
	ForthClassObject* pClassObject = (ForthClassObject*) pMethods[-1];
	// TBD: some error checking here might be nice
	pMethods = pClassObject->pVocab->ParentClass()->GetInterface(0)->GetMethods();
    SPUSH( ((long) pMethods) );
}

FORTHOP( newOp )
{
    // TBD: allow sizeOf to be applied to variables
    // TBD: allow sizeOf to apply to native types, including strings
    ForthEngine *pEngine = GET_ENGINE;
    char *pSym = pEngine->GetNextSimpleToken();
    ForthVocabulary* pFoundVocab;
    long *pEntry = pEngine->GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );

    if ( pEntry )
    {
        ForthTypesManager* pManager = ForthTypesManager::GetInstance();
        ForthClassVocabulary* pClassVocab = (ForthClassVocabulary *) (pManager->GetStructVocabulary( pEntry[0] ));

        if ( pClassVocab && pClassVocab->IsClass() )
        {
            if ( pEngine->IsCompiling() )
            {
                pEngine->CompileBuiltinOpcode( OP_DO_NEW );
                pEngine->CompileLong( (long) pClassVocab );
            }
            else
            {
                SPUSH( (long) pClassVocab );
                pEngine->ExecuteOneOp( pClassVocab->GetClassObject()->newOp );
            }
        }
        else
        {
            pEngine->AddErrorText( pSym );
            pEngine->SetError( kForthErrorUnknownSymbol, " is not a class" );
        }
    }
    else
    {
        pEngine->SetError( kForthErrorUnknownSymbol, pSym );
    }
}

FORTHOP( doNewOp )
{
	// this op is compiled for 'new foo', the class vocabulary ptr is compiled immediately after it
    ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (*pCore->IP++);
    SPUSH( (long) pClassVocab );
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->ExecuteOneOp( pClassVocab->GetClassObject()->newOp );
}

FORTHOP( allocObjectOp )
{
	// allocObject is the default new op, it is used only for classes which don't define their
	//   own 'new' op, or extend a class that does
    ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
    ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
    if ( pPrimaryInterface )
    {
        long nBytes = pClassVocab->GetSize();
        void* pData = malloc( nBytes );
		memset( pData, 0, nBytes );
        SPUSH( (long) pData );
        SPUSH( (long) (pPrimaryInterface->GetMethods()) );
    }
    else
    {
        ForthEngine *pEngine = GET_ENGINE;
        pEngine->AddErrorText( pClassVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " failure in new - has no primary interface" );
    }
}

FORTHOP( initMemberStringOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pString = pEngine->GetNextSimpleToken();
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->GetNewestClass();
    long* pEntry;

    if ( !pEngine->CheckFlag( kEngineFlagIsMethod ) || (pVocab == NULL) )
    {
        pEngine->SetError( kForthErrorBadSyntax, "initMemberString can only be used inside a method" );
        return;
    }
    pEntry = pVocab->FindSymbol( pString );
    if ( !pEntry )
    {
        pEngine->AddErrorText( pString );
        pEngine->SetError( kForthErrorUnknownSymbol, " is not a field in this class" );
        return;
    }

    long typeCode = pEntry[1];
    if ( !CODE_IS_SIMPLE(typeCode) || !CODE_IS_NATIVE(typeCode) || (CODE_TO_BASE_TYPE(typeCode) != kBaseTypeString) )
    {
        pEngine->SetError( kForthErrorBadSyntax, "initMemberString can only be used on a simple string" );
        return;
    }
	// pEntry[2] is the total string struct length in bytes, subtract 9 for maxLen, curLen and terminating null
    long len = pEntry[2] - 9;
	if ( len > 4095 )
    {
        pEngine->SetError( kForthErrorBadParameter, "initMemberString can not be used on string with size > 4095" );
        return;
    }
	if ( pEntry[0] > 4095 )
    {
        pEngine->SetError( kForthErrorBadParameter, "initMemberString can not be used on string with member offset > 4095" );
        return;
    }

	// only shift up by 10 bits since pEntry[0] is member byte offset, opcode holds member offset in longs
    long varOffset = (pEntry[0] << 10) | len;

    pEngine->CompileOpcode( COMPILED_OP( kOpMemberStringInit, varOffset ) );
}

FORTHOP( enumOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    if ( pEngine->CheckFlag( kEngineFlagInStructDefinition ) )
    {
        pEngine->SetError( kForthErrorBadSyntax, "enum definition not allowed inside struct definition" );
        return;
    }
    pEngine->StartEnumDefinition();
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileBuiltinOpcode( OP_DO_ENUM );
}

FORTHOP( endenumOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->EndEnumDefinition();
}

FORTHOP( doVocabOp )
{
    // IP points to data field
    ForthVocabulary* pVocab = (ForthVocabulary *) (*GET_IP);
    pVocab->DoOp( pCore );
    SET_IP( (long *) (RPOP) );
}

// doStructTypeOp is compiled at the start of each user-defined structure defining word 
FORTHOP( doStructTypeOp )
{
    // IP points to data field
    ForthStructVocabulary *pVocab = (ForthStructVocabulary *) (*GET_IP);
    pVocab->DefineInstance();
    SET_IP( (long *) (RPOP) );
}

// doClassTypeOp is compiled at the start of each user-defined class defining word 
FORTHOP( doClassTypeOp )
{
    // IP points to data field
    ForthClassVocabulary *pVocab = (ForthClassVocabulary *) (*GET_IP);
    if ( GET_VAR_OPERATION == kVocabGetClass )
    {
        // this is invoked at runtime when code explicitly invokes methods on class objects (IE CLASSNAME.new)
        pVocab->DoOp( pCore );
    }
    else
    {
        pVocab->DefineInstance();
    }
    SET_IP( (long *) (RPOP) );
}

// has precedence!
// this is compiled as the only op for each enum set defining word
// an enum set defining word acts just like the "int" op - it defines
// a variable or field that is 4 bytes in size
extern FORTHOP( intOp );
FORTHOP( doEnumOp )
{
    intOp( pCore );
    // we need to pop the IP, since there is no instruction after this one
    SET_IP( (long *) (RPOP) );
}

// in traditional FORTH, the vocabulary entry for the operation currently
// being defined is "smudged" so that if a symbol appears in its own definition
// it will normally only match any previously existing symbol of the same name.
// This makes it easy to redefine existing symbols in terms of their old
// definitions.
// The "recursive" op just unsmudges the vocabulary entry currently being
// defined so that a definition can refer to itself.

// has precedence!
FORTHOP( recursiveOp )
{
    GET_ENGINE->GetDefinitionVocabulary()->UnSmudgeNewestSymbol();
}

FORTHOP( precedenceOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pSym = pEngine->GetNextSimpleToken();
    long *pEntry = pEngine->GetDefinitionVocabulary()->FindSymbol( pSym );
    
    if ( pEntry )
    {
        long op = *pEntry;
        ulong opVal = FORTH_OP_VALUE( op );
        switch ( FORTH_OP_TYPE( op ) )
        {
            case kOpNative:
                *pEntry = COMPILED_OP( kOpNativeImmediate, opVal );
                break;

            case kOpUserDef:
                *pEntry = COMPILED_OP( kOpUserDefImmediate, opVal );
                break;

            case kOpCCode:
                *pEntry = COMPILED_OP( kOpCCodeImmediate, opVal );
                break;

            default:
                CONSOLE_STRING_OUT( "!!!! Can\'t set precedence for " );
                CONSOLE_STRING_OUT( pSym );
                CONSOLE_STRING_OUT( "s - wrong type !!!!\n" );
                TRACE( "!!!! Can\'t set precedence for %s - wrong type !!!!\n", pSym );
                break;
        }
    }
    else
    {
        CONSOLE_STRING_OUT( "!!!! Failure finding symbol " );
        CONSOLE_STRING_OUT( pSym );
        TRACE( "!!!! Failure finding symbol %s !!!!\n", pSym );
    }
}

FORTHOP( strLoadOp )
{
    char *pFileName = ((char *) (SPOP));

    if ( pFileName != NULL )
    {
        ForthEngine *pEngine = GET_ENGINE;
        if ( pEngine->PushInputFile( pFileName ) == false )
        {
            CONSOLE_STRING_OUT( "!!!! Failure opening source file " );
            CONSOLE_STRING_OUT( pFileName );
            CONSOLE_STRING_OUT( " !!!!\n" );
            TRACE( "!!!! Failure opening source file %s !!!!\n", pFileName );
        }
    }
}

FORTHOP( loadOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pFileName = pEngine->GetNextSimpleToken();
    SPUSH( ((long) pFileName) );
    strLoadOp( pCore );
}

FORTHOP( loadDoneOp )
{
    GET_ENGINE->PopInputStream();
}

FORTHOP( requiresOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pSymbolName = pEngine->GetNextSimpleToken();
    ForthVocabulary  *pVocab = pEngine->GetSearchVocabulary();
    if ( pVocab->FindSymbol( pSymbolName ) == NULL )
    {
        // symbol not found - load symbol.txt
        char *pFileName = new char[ strlen( pSymbolName ) + 8 ];
        sprintf( pFileName, "%s.txt", pSymbolName );
        if ( pEngine->PushInputFile( pFileName ) == false )
        {
            CONSOLE_STRING_OUT( "!!!! Failure opening source file " );
            CONSOLE_STRING_OUT( pFileName );
            CONSOLE_STRING_OUT( " !!!!\n" );
            TRACE( "!!!! Failure opening source file %s !!!!\n", pFileName );
        }
        delete [] pFileName;
    }
}

FORTHOP( interpretOp )
{
    char* pStr = (char *) SPOP;
    if ( pStr != NULL )
    {
        int len = strlen( pStr ) + 1;
        ForthEngine *pEngine = GET_ENGINE;
        pEngine->PushInputBuffer( pStr, len );
		ForthShell* pShell = pEngine->GetShell();
		pShell->ProcessLine( pStr );
		pEngine->PopInputStream();
    }
}

FORTHOP( stateInterpretOp )
{
    GET_ENGINE->SetCompileState( 0 );
}

FORTHOP( stateCompileOp )
{
    GET_ENGINE->SetCompileState( 1 );
}

FORTHOP( stateOp )
{
    SPUSH( (long) GET_ENGINE->GetCompileStatePtr() );
}

FORTHOP( strTickOp )
{
    ForthEngine *pEngine = GET_ENGINE;
	char* pToken = (char *) SPOP;
    long *pSymbol = pEngine->FindSymbol( pToken );
    if ( pSymbol != NULL )
    {
        SPUSH( *pSymbol );
    }
    else
    {
        SET_ERROR( kForthErrorUnknownSymbol );
        pEngine->AddErrorText( pToken );
    }
}

// has precedence!
FORTHOP( compileOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pToken = pEngine->GetNextSimpleToken();
    long *pSymbol = pEngine->FindSymbol( pToken );
    if ( pSymbol != NULL )
    {
        pEngine->CompileOpcode( *pSymbol );
    }
    else
    {
        SET_ERROR( kForthErrorUnknownSymbol );
    }
}

// has precedence!
FORTHOP( bracketTickOp )
{
    // TBD: what should this do if state is interpret? an error? or act the same as tick?
    ForthEngine *pEngine = GET_ENGINE;
    char *pToken = pEngine->GetNextSimpleToken();
    long *pSymbol = pEngine->FindSymbol( pToken );
    if ( pSymbol != NULL )
    {
        pEngine->CompileBuiltinOpcode( OP_INT_VAL );
        pEngine->CompileLong( *pSymbol );
    }
    else
    {
        SET_ERROR( kForthErrorUnknownSymbol );
    }
}

   
//##############################
//
//  output ops
//

void
consoleOutToFile( ForthCoreState   *pCore,
                  const char       *pMessage )
{    
    FILE *pOutFile = static_cast<FILE*>(GET_CON_OUT_DATA);
    if ( pOutFile != NULL )
    {
        fprintf(pOutFile, "%s", pMessage );
    }
    else
    {
        // TBD: report error
    }
}

void
consoleOutToString( ForthCoreState   *pCore,
                    const char       *pMessage )
{    
    char *pOutStr = static_cast<char*>(GET_CON_OUT_DATA);
    if ( pOutStr != NULL )
    {
        strcat( pOutStr, pMessage );
    }
    else
    {
        // TBD: report error
    }
}

void
consoleOutToOp( ForthCoreState   *pCore,
                const char       *pMessage )
{
    SPUSH( (long) pMessage );
    long op = GET_CON_OUT_OP;
    ForthEngine* pEngine = GET_ENGINE;
    pEngine->ExecuteOneOp( op );
}

static void
printNumInCurrentBase( ForthCoreState   *pCore,
                       long             val )
{
    NEEDS(1);
#define PRINT_NUM_BUFF_CHARS 68
    char buff[ PRINT_NUM_BUFF_CHARS ];
    char *pNext = &buff[ PRINT_NUM_BUFF_CHARS ];
    ldiv_t v;
    long base;
    bool bIsNegative, bPrintUnsigned;
    ulong urem;
    ePrintSignedMode signMode;

    if ( val == 0 )
    {
        strcpy( buff, "0" );
        pNext = buff;
    }
    else
    {
        base = *(GET_BASE_REF);

        signMode = (ePrintSignedMode) GET_PRINT_SIGNED_NUM_MODE;
        if ( (base == 10) && (signMode == kPrintSignedDecimal) )
        {

            // most common case - print signed decimal
            sprintf( buff, "%d", val );
            pNext = buff;

        }
        else
        {

            // unsigned or any base other than 10

            *--pNext = 0;
            bPrintUnsigned = !(signMode == kPrintAllSigned);
            if ( bPrintUnsigned )
            {
                // since div is defined as signed divide/mod, make sure
                //   that the number is not negative by generating the bottom digit
                bIsNegative = false;
                urem = ((ulong) val) % ((ulong) base);
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                val = ((ulong) val) / ((ulong) base);
            }
            else
            {
                bIsNegative = ( val < 0 );
                if ( bIsNegative )
                {
                    val = (-val);
                }
            }
            while ( val != 0 )
            {
#if defined(WIN32)
#if defined(MSDEV)
                v = div( val, base );
#else
                v = ldiv( val, base );
#endif
#elif defined(LINUX)
                v = ldiv( val, base );
#endif
                *--pNext = (char) ( (v.rem < 10) ? (v.rem + '0') : ((v.rem - 10) + 'a') );
                val = v.quot;
            }
            if ( bIsNegative )
            {
                *--pNext = '-';
            }
        }
    }
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", pNext );
#endif

    CONSOLE_STRING_OUT( pNext );
}


static void
printLongNumInCurrentBase( ForthCoreState   *pCore,
						   long long        val )
{
    NEEDS(1);
#define PRINT_NUM_BUFF_CHARS 68
    char buff[ PRINT_NUM_BUFF_CHARS ];
    char *pNext = &buff[ PRINT_NUM_BUFF_CHARS ];
    ulong base;
    bool bIsNegative, bPrintUnsigned;
    ulong urem;
	unsigned long long uval;
    ePrintSignedMode signMode;

    if ( val == 0L )
    {
        strcpy( buff, "0" );
        pNext = buff;
    }
    else
    {
        base = (ulong) *(GET_BASE_REF);

        signMode = (ePrintSignedMode) GET_PRINT_SIGNED_NUM_MODE;
        if ( base == 10 ) 
        {

            // most common case - print signed decimal
			switch ( signMode )
			{
			case kPrintAllUnsigned:
	            sprintf( buff, "%ulld", val );
				break;
			default:
	            sprintf( buff, "%lld", val );
				break;
			}
            pNext = buff;

        }
        else
        {

            // unsigned or any base other than 10

            *--pNext = 0;
            bPrintUnsigned = !(signMode == kPrintAllSigned);
            if ( bPrintUnsigned )
            {
                // since div is defined as signed divide/mod, make sure
                //   that the number is not negative by generating the bottom digit
                bIsNegative = false;
				uval = (unsigned long long) val;
                urem = uval % base;
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                uval = uval / base;
            }
            else
            {
                bIsNegative = ( val < 0 );
                if ( bIsNegative )
                {
                    uval = (-val);
                }
				else
				{
					uval = val;
				}
            }
            while ( uval != 0 )
            {
                urem = uval % base;
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                uval = uval / base;
            }
            if ( bIsNegative )
            {
                *--pNext = '-';
            }
        }
    }
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", pNext );
#endif

    CONSOLE_STRING_OUT( pNext );
}


FORTHOP( printNumOp )
{
    printNumInCurrentBase( pCore, SPOP );
}

FORTHOP( printLongNumOp )
{
	long long val = LPOP;
    printLongNumInCurrentBase( pCore, val );
}

FORTHOP( printNumDecimalOp )
{
    NEEDS(1);
    char buff[36];

    long val = SPOP;
    sprintf( buff, "%d", val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printNumHexOp )
{
    NEEDS(1);
    char buff[12];

    long val = SPOP;
    sprintf( buff, "%x", val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printLongDecimalOp )
{
    NEEDS(2);
    char buff[40];

    long long val = LPOP;
    sprintf( buff, "%I64d", val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printLongHexOp )
{
    NEEDS(1);
    char buff[20];

    long long val = LPOP;
    sprintf( buff, "%I64x", val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printFloatOp )
{
    NEEDS(1);
    char buff[36];

    float fval = FPOP;
    sprintf( buff, "%f", fval );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printDoubleOp )
{
    NEEDS(2);
    char buff[36];
    double dval = DPOP;

    sprintf( buff, "%f", dval );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printFormattedOp )
{
    NEEDS(2);
    char buff[80];

    char* pFmt = (char *) (SPOP);
    long val = SPOP;
    sprintf( buff, pFmt, val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

#ifdef _ASM_INNER_INTERPRETER
extern long fprintfSub( ForthCoreState* pCore );
extern long sprintfSub( ForthCoreState* pCore );
extern long fscanfSub( ForthCoreState* pCore );
extern long sscanfSub( ForthCoreState* pCore );
#else

long fprintfSub( ForthCoreState* pCore )
{
    int a[8];
    // TBD: assert if numArgs > 8
    long numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = SPOP;
    }
    const char* fmt = (const char *) SPOP;
    FILE* outfile = (FILE *) SPOP;
    return fprintf( outfile, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
}

long sprintfSub( ForthCoreState* pCore )
{
    int a[8];
    // TBD: assert if numArgs > 8
    long numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = SPOP;
    }
    const char* fmt = (const char *) SPOP;
    char* outbuff = (char *) SPOP;
    return sprintf( outbuff, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
}

long fscanfSub( ForthCoreState* pCore )
{
    void* a[8];
    // TBD: assert if numArgs > 8
    long numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = (void *) SPOP;
    }
    const char* fmt = (const char *) SPOP;
    FILE* infile = (FILE *) SPOP;
    return fscanf( infile, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
}

long sscanfSub( ForthCoreState* pCore )
{
    void* a[8];
    // TBD: assert if numArgs > 8
    long numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = (void *) SPOP;
    }
    const char* fmt = (const char *) SPOP;
    char* inbuff = (char *) SPOP;
    return sscanf( inbuff, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
}

#endif

FORTHOP( fprintfOp )
{
    // TOS: N argN ... arg1 formatStr filePtr       (arg1 to argN are optional)
    fprintfSub( pCore );
}

FORTHOP( sprintfOp )
{
    // TOS: N argN ... arg1 formatStr bufferPtr       (arg1 to argN are optional)
    sprintfSub( pCore );
}

FORTHOP( fscanfOp )
{
    // TOS: N argN ... arg1 formatStr filePtr       (arg1 to argN are optional)
    fscanfSub( pCore );
}

FORTHOP( sscanfOp )
{
    // TOS: N argN ... arg1 formatStr bufferPtr       (arg1 to argN are optional)
    sscanfSub( pCore );
}

FORTHOP( printStrOp )
{
    NEEDS(1);
    char *buff = (char *) SPOP;
	if ( buff == NULL )
	{
		buff = "<<<NULL>>>";
	}
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printBlockOp )
{
    NEEDS(2);
    char buff[2];
    buff[1] = 0;
    long count = SPOP;
    const char* pChars = (const char*)(SPOP);
	if ( pChars == NULL )
	{
		pChars = "<<<NULL>>>";
		count = strlen( pChars );
	}
    for ( int i = 0; i < count; i++ )
    {
        buff[0] = *pChars++;
        CONSOLE_STRING_OUT( buff );
    }
}

FORTHOP( printCharOp )
{
    NEEDS(1);
    char buff[2];
    buff[0] = (char) SPOP;
    buff[1] = 0;

#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printSpaceOp )
{
    SPUSH( (long) ' ' );
    printCharOp( pCore );
}

FORTHOP( printNewlineOp )
{
    SPUSH( (long) '\n' );
    printCharOp( pCore );
}

FORTHOP( baseOp )
{
    SPUSH( (long) GET_BASE_REF );
}

FORTHOP( octalOp )
{
    *GET_BASE_REF = 8;
}

FORTHOP( decimalOp )
{
    *GET_BASE_REF = 10;
}

FORTHOP( hexOp )
{
    *GET_BASE_REF = 16;
}

FORTHOP( printDecimalSignedOp )
{
    SET_PRINT_SIGNED_NUM_MODE( kPrintSignedDecimal );
}

FORTHOP( printAllSignedOp )
{
    SET_PRINT_SIGNED_NUM_MODE( kPrintAllSigned );
}

FORTHOP( printAllUnsignedOp )
{
    SET_PRINT_SIGNED_NUM_MODE( kPrintAllUnsigned );
}

FORTHOP( outToScreenOp )
{
    ForthEngine *pEngine = GET_ENGINE;

	pEngine->ResetConsoleOut( pCore );
}

FORTHOP( outToFileOp )
{
    NEEDS( 1 );
    SET_CON_OUT_ROUTINE( consoleOutToFile );
    SET_CON_OUT_DATA( reinterpret_cast<void*>(SPOP) );
}

FORTHOP( outToStringOp )
{
    NEEDS( 1 );
    SET_CON_OUT_ROUTINE( consoleOutToString );
    SET_CON_OUT_DATA( (char *) SPOP );
}

FORTHOP( outToOpOp )
{
    NEEDS( 1 );
    SET_CON_OUT_ROUTINE( consoleOutToOp );
    SET_CON_OUT_OP( SPOP );
}

FORTHOP( getConOutFileOp )
{
    SPUSH( (long) GET_CON_OUT_DATA );
}

//##############################
//
//  File ops
//
FORTHOP( opendirOp )
{
    NEEDS(1);
	const char* pPath = (const char*) SPOP;
    int result = (int) (pCore->pFileFuncs->openDir( pPath ));
	// result is actually a DIR*
    SPUSH( result );
}

FORTHOP( readdirOp )
{
    NEEDS(1);
	void* pDir = (void *) SPOP;
    int result = (int)(pCore->pFileFuncs->readDir( pDir ));
	// result is actually a struct dirent*
    SPUSH( result );
}

FORTHOP( closedirOp )
{
    NEEDS(1);
	void* pDir = (void *) SPOP;
    int result = pCore->pFileFuncs->closeDir( pDir );
    SPUSH( result );
}

FORTHOP( rewinddirOp )
{
    NEEDS(1);
	void* pDir = (void *) SPOP;
    pCore->pFileFuncs->rewindDir( pDir );
}


FORTHOP( removeOp )
{
	const char* pFilename = (const char *) (SPOP);
    int result = pCore->pFileFuncs->fileRemove( pFilename );
    SPUSH( result );
}

FORTHOP( _dupOp )
{
	int fileHandle = SPOP;
    int result = pCore->pFileFuncs->fileDup( fileHandle );
    SPUSH( result );
}

FORTHOP( _dup2Op )
{
	int dstFileHandle = SPOP;
	int srcFileHandle = SPOP;
    int result = pCore->pFileFuncs->fileDup2( srcFileHandle, dstFileHandle );
    SPUSH( result );
}

FORTHOP( _filenoOp )
{
	FILE* pFile = (FILE *) SPOP;
	int result = pCore->pFileFuncs->fileNo( pFile );
    SPUSH( result );
}

FORTHOP( tmpnamOp )
{
	char* pOutname = (char *) malloc( L_tmpnam );
    ForthEngine *pEngine = GET_ENGINE;
	if ( pCore->pFileFuncs->getTmpnam( pOutname ) == NULL )
	{
        SET_ERROR( kForthErrorFileOpen );
        pEngine->AddErrorText( "system: failure creating standard out tempfile name" );
		free( pOutname );
		pOutname = NULL;
	}
    SPUSH( (long) pOutname );
}

FORTHOP( fflushOp )
{
	FILE* pFile = (FILE *) SPOP;
	int result = pCore->pFileFuncs->fileFlush( pFile );
    SPUSH( result );
}

FORTHOP( systemOp )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->runSystem( (char *) SPOP );
    SPUSH( result );
}

FORTHOP( chdirOp )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->changeDir( (const char *) SPOP );
    SPUSH( result );
}

FORTHOP( mkdirOp )
{
    NEEDS(2);
	int mode = SPOP;
	const char* pPath = (const char*) SPOP;
    int result = pCore->pFileFuncs->makeDir( pPath, mode );
    SPUSH( result );
}

FORTHOP( rmdirOp )
{
    NEEDS(1);
	const char* pPath = (const char*) SPOP;
    int result = pCore->pFileFuncs->removeDir( pPath );
    SPUSH( result );
}

FORTHOP( renameOp )
{
    NEEDS(2);
	const char* pDstPath = (const char*) SPOP;
	const char* pSrcPath = (const char*) SPOP;
    int result = pCore->pFileFuncs->renameFile( pSrcPath, pDstPath );
    SPUSH( result );
}


//##############################
//
// Default input/output file ops
//

FORTHOP( stdinOp )
{
    int result = (int) (pCore->pFileFuncs->getStdIn());
    SPUSH( result );
}

FORTHOP( stdoutOp )
{
    int result = (int) (pCore->pFileFuncs->getStdOut());
    SPUSH( result );
}

FORTHOP( stderrOp )
{
    int result = (int) (pCore->pFileFuncs->getStdErr());
    SPUSH( result );
}

FORTHOP( dstackOp )
{
    long *pSP = GET_SP;
    int nItems = GET_SDEPTH;
    int i;
    char buff[64];

    sprintf( buff, "stack[%d]:", nItems );
    CONSOLE_STRING_OUT( buff );
    for ( i = 0; i < nItems; i++ )
    {
        CONSOLE_STRING_OUT( " " );
        printNumInCurrentBase( pCore, *pSP++ );
    }
    CONSOLE_STRING_OUT( "\n" );
}


FORTHOP( drstackOp )
{
    long *pRP = GET_RP;
    int nItems = GET_RDEPTH;
    int i;
    char buff[64];

    sprintf( buff, "rstack[%d]:", nItems );
    CONSOLE_STRING_OUT( buff );
    for ( i = 0; i < nItems; i++ )
    {
        CONSOLE_STRING_OUT( " " );
        printNumInCurrentBase( pCore, *pRP++ );
    }
    CONSOLE_STRING_OUT( "\n" );
}


FORTHOP( statsOp )
{
    char buff[512];

    sprintf( buff, "pCore %p pEngine %p     DP %p DBase %p    IP %p\n",
             pCore, pCore->pEngine, pCore->pDictionary, pCore->pDictionary->pBase, pCore->IP );
    CONSOLE_STRING_OUT( buff );
    sprintf( buff, "SP %p ST %p SLen %d    RP %p RT %p RLen %d\n",
             pCore->SP, pCore->ST, pCore->SLen,
             pCore->RP, pCore->RT, pCore->RLen );
    CONSOLE_STRING_OUT( buff );
    sprintf( buff, "%d builtins    %d userops\n", pCore->numBuiltinOps, pCore->numOps );
    CONSOLE_STRING_OUT( buff );
}

FORTHOP( describeOp )
{
    char buff[512];


    ForthEngine *pEngine = GET_ENGINE;
    char* pSym = pEngine->GetNextSimpleToken();
	strcpy( buff, pSym );

	char* pMethod = strchr( buff, '.' );
	if ( pMethod != NULL )
	{
		*pMethod++ = '\0';
	}
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthStructVocabulary* pVocab = pManager->GetStructVocabulary( buff );
	bool verbose = (GET_VAR_OPERATION != kVarDefaultOp);

    if ( pVocab )
    {
		if ( pMethod != NULL )
		{
			ForthVocabulary* pFoundVocab = NULL;
			long* pEntry = pVocab->FindSymbol( pMethod );
			if ( (pEntry != NULL) && pVocab->IsClass() )
			{
				ForthClassVocabulary* pClassVocab = (ForthClassVocabulary*) pVocab;
				long typeCode = pEntry[1];
				if ( CODE_IS_METHOD( typeCode ) )
				{
					// TBD: support secondary interfaces
					pEngine->DescribeOp( pSym, pClassVocab->GetInterface(0)->GetMethod(pEntry[0]), pEntry[1] );
				}
				else if ( CODE_TO_BASE_TYPE( typeCode ) == kBaseTypeUserDefinition )
				{
					pEngine->DescribeOp( pSym, pEntry[0], pEntry[1] );
				}
				else
				{
					pVocab->PrintEntry( pEntry );
				}
			}
			else
			{
				sprintf( buff, "Failed to find method %s\n", pSym );
				CONSOLE_STRING_OUT( buff );
			}
		}
		else
		{
			// show structure vocabulary entries
			while ( pVocab )
			{
				sprintf( buff, "%s vocabulary %s:\n", ((pVocab->IsClass() ? "class" : "struct")), pVocab->GetName() );
				CONSOLE_STRING_OUT( buff );
				char quit = ShowVocab( pEngine->GetCoreState(), pVocab, !verbose );
				if ( quit == 'q' )
				{
					break;
				}
				else
				{
					pVocab = pVocab->BaseVocabulary();
				}
			}
		}
    }
    else
    {
        pEngine->DescribeSymbol( pSym );
    }
	CLEAR_VAR_OPERATION;
}

#ifdef WIN32

FORTHOP( DLLVocabularyOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabulary *pDefinitionsVocab = pEngine->GetDefinitionVocabulary();
    // get next symbol, add it to vocabulary with type "user op"
    char* pDLLOpName = pEngine->GetNextSimpleToken();
    pEngine->StartOpDefinition( pDLLOpName );
    char* pDLLName = pEngine->GetNextSimpleToken();
    ForthDLLVocabulary* pVocab = new ForthDLLVocabulary( pDLLOpName,
                                                         pDLLName,
                                                         NUM_FORTH_VOCAB_VALUE_LONGS,
                                                         512,
                                                         GET_DP,
                                                         ForthVocabulary::GetEntryValue( pDefinitionsVocab->GetNewestEntry() ) );
    pEngine->CompileBuiltinOpcode( OP_DO_VOCAB );
    pVocab->LoadDLL();
    pEngine->CompileLong( (long) pVocab );
}

FORTHOP( addDLLEntryOp )
{
    NEEDS( 2 );

    ForthEngine *pEngine = GET_ENGINE;
    char* pProcName = (char *) SPOP;
    ForthDLLVocabulary* pVocab = (ForthDLLVocabulary *) (pEngine->GetDefinitionVocabulary());
    if ( strcmp( pVocab->GetType(), "dllOp" ) )
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " is not a DLL vocabulary - addDllEntry" );
    }
    ulong numArgs = SPOP;
    pVocab->AddEntry( pProcName, numArgs );
}

FORTHOP( DLLVoidOp )
{
	NEEDS( 0 );

    ForthEngine *pEngine = GET_ENGINE;
    ForthDLLVocabulary* pVocab = (ForthDLLVocabulary *) (pEngine->GetDefinitionVocabulary());
    if ( strcmp( pVocab->GetType(), "dllOp" ) )
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " is not a DLL vocabulary - DLLVoidOp" );
    }
	else
	{
		pVocab->SetFlag( DLL_ENTRY_FLAG_RETURN_VOID );
	}
}

FORTHOP( DLLLongOp )
{
	NEEDS( 0 );

    ForthEngine *pEngine = GET_ENGINE;
    ForthDLLVocabulary* pVocab = (ForthDLLVocabulary *) (pEngine->GetDefinitionVocabulary());
    if ( strcmp( pVocab->GetType(), "dllOp" ) )
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " is not a DLL vocabulary - DLLLongOp" );
    }
	else
	{
		pVocab->SetFlag( DLL_ENTRY_FLAG_RETURN_64BIT );
	}
}

FORTHOP( DLLStdCallOp )
{
	NEEDS( 0 );

    ForthEngine *pEngine = GET_ENGINE;
    ForthDLLVocabulary* pVocab = (ForthDLLVocabulary *) (pEngine->GetDefinitionVocabulary());
    if ( strcmp( pVocab->GetType(), "dllOp" ) )
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( kForthErrorBadParameter, " is not a DLL vocabulary - DLLStdCallOp" );
    }
	else
	{
		pVocab->SetFlag( DLL_ENTRY_FLAG_STDCALL );
	}
}
#endif

FORTHOP( blwordOp )
{
    NEEDS( 0 );
    ForthShell *pShell = GET_ENGINE->GetShell();
	char *pSrc = pShell->GetNextSimpleToken();
	char *pDst = GET_ENGINE->GetTmpStringBuffer();
	strncpy( pDst, pSrc, (TMP_STRING_BUFFER_LEN - 1) );
    SPUSH( (long) pDst );
}

FORTHOP( wordOp )
{
    NEEDS( 1 );
    ForthShell *pShell = GET_ENGINE->GetShell();
    char delim = (char) (SPOP);
	char *pSrc = pShell->GetToken( delim );
	char *pDst = GET_ENGINE->GetTmpStringBuffer();
	strncpy( pDst, pSrc, (TMP_STRING_BUFFER_LEN - 1) );
    SPUSH( (long) pDst );
}

// has precedence!
// inline comment using "/*"
FORTHOP( commentOp )
{
    NEEDS( 0 );
    ForthShell *pShell = GET_ENGINE->GetShell();
    ForthInputStack* pInput = pShell->GetInput();
    char *pSrc = pInput->GetBufferPointer();
    char *pEnd = strstr( pSrc, "*/" );
    if ( pEnd != NULL )
    {
        pInput->SetBufferPointer( pEnd + 2 );
    }
    else
    {
        // end of comment not found on line, just terminate line here
        *pSrc = '\0';
    }
}

// has precedence!
// inline comment using parens
// strictly for ease of porting old style forth code
FORTHOP( parenCommentOp )
{
    NEEDS( 0 );
    ForthShell *pShell = GET_ENGINE->GetShell();
	pShell->GetToken( ')' );
}

// fake variable used to turn on/off old-style paren comments mode
FORTHOP( parenIsCommentOp )
{
    NEEDS( 1 );
    if ( GET_VAR_OPERATION == kVarStore )
    {
        if ( SPOP )
        {
            GET_ENGINE->SetFlag( kEngineFlagParenIsComment );
        }
        else
        {
            GET_ENGINE->ClearFlag( kEngineFlagParenIsComment );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        SPUSH( GET_ENGINE->CheckFlag( kEngineFlagParenIsComment ) ? ~0 : 0 );
    }
}

FORTHOP( getInBufferBaseOp )
{
    ForthInputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    SPUSH( (long) (pInput->GetBufferBasePointer()) );
}

FORTHOP( getInBufferPointerOp )
{
    ForthInputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    SPUSH( (long) (pInput->GetBufferPointer()) );
}

FORTHOP( setInBufferPointerOp )
{
    ForthInputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    pInput->SetBufferPointer( (char *) (SPOP) );
}

FORTHOP( getInBufferLengthOp )
{
    ForthInputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    SPUSH( (long) (pInput->GetBufferLength()) );
}

FORTHOP( fillInBufferOp )
{
    ForthInputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    SPUSH( (long) (pInput->GetLine( (char *) (SPOP) )) );
}

FORTHOP( keyOp )
{
	int key = getch();
    SPUSH( key );
}

FORTHOP( keyHitOp )
{
	int keyHit = kbhit();
    SPUSH( keyHit );
}

FORTHOP( vocNewestEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	SPUSH( (long) (pVocab->GetNewestEntry()) );
}

FORTHOP( vocNextEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	long *pEntry = (long *) (SPOP);
	SPUSH( (long) (pVocab->NextEntry( pEntry )) );
}

FORTHOP( vocNumEntriesOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	SPUSH( (long) (pVocab->GetNumEntries()) );
}

FORTHOP( vocNameOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	SPUSH( (long) (pVocab->GetName()) );
}

FORTHOP( vocChainHeadOp )
{
	SPUSH( (long) (ForthVocabulary::GetVocabularyChainHead()) );
}

FORTHOP( vocChainNextOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	SPUSH( (long) (pVocab->GetNextChainVocabulary()) );
}

FORTHOP( vocFindEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	const char* pName = (const char *) (SPOP);
	SPUSH( (long) (pVocab->FindSymbol( pName )) );
}

FORTHOP( vocFindNextEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	const char* pName = (const char *) (SPOP);
	long *pEntry = (long *) (SPOP);
	SPUSH( (long) (pVocab->FindNextSymbol( pName, pEntry )) );
}

FORTHOP( vocFindEntryByValueOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	long val = SPOP;
	SPUSH( (long) (pVocab->FindSymbolByValue( val )) );
}

FORTHOP( vocFindNextEntryByValueOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	long val = SPOP;
	long *pEntry = (long *) (SPOP);
	SPUSH( (long) (pVocab->FindNextSymbolByValue( val, pEntry )) );
}

FORTHOP( vocAddEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
    long opVal = SPOP;
    long opType = SPOP;
    char* pSymbol = (char *) (SPOP);
	bool addToEngineOps = (opType <= kOpDLLEntryPoint);
    pVocab->AddSymbol( pSymbol, opType, opVal, addToEngineOps );
}

FORTHOP( vocRemoveEntryOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	long *pEntry = (long *) (SPOP);
	pVocab->DeleteEntry( pEntry );
}

FORTHOP( vocValueLengthOp )
{
	ForthVocabulary* pVocab = (ForthVocabulary *) (SPOP);
	SPUSH( (long) (pVocab->GetValueLength()) );
}

FORTHOP( turboOp )
{
    GET_ENGINE->ToggleFastMode();
    SET_STATE( kResultDone );
}

FORTHOP( errorOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->SetError( kForthErrorUserDefined, (char *) (SPOP) );
}

FORTHOP( addErrorTextOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->AddErrorText( (char *) (SPOP) );
}

FORTHOP( timeOp )
{
    time_t rawtime;
    time ( &rawtime );
    DPUSH( *((double *) &rawtime) );
}

FORTHOP( strftimeOp )
{
    double dtime = DPOP;
    time_t rawtime = *((time_t*) &dtime);
    struct tm * timeinfo;
    const char* fmt = (const char *)(SPOP);
    size_t bufferSize = (size_t)(SPOP);
    char* buffer = (char *)(SPOP);

    timeinfo = localtime ( &rawtime );
    // Www Mmm dd yyyy (weekday, month, day, year)
    strftime( buffer, bufferSize, fmt, timeinfo);
}

FORTHOP( millitimeOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    SPUSH( (long) pEngine->GetElapsedTime() );
}

FORTHOP( millisleepOp )
{
#ifdef WIN32
	DWORD dwMilliseconds = (DWORD)(SPOP);
    ::Sleep( dwMilliseconds );
#else
    int milliseconds = SPOP;
    usleep( milliseconds * 1000 );
#endif
}


//##############################
//
//  computation utilities - randoms, hashing, sorting and searcing
//

FORTHOP( randOp )
{
    SPUSH( rand() );
}

FORTHOP( srandOp )
{
    srand( (unsigned int) (SPOP) );
    SPUSH( rand() );
}

#define get16bits(d) ((((unsigned long)(((const unsigned char *)(d))[1])) << 8)\
						+(unsigned long)(((const unsigned char *)(d))[0]) )

unsigned long
SuperFastHash (const char * data, int len, unsigned long hash)
{

	unsigned long tmp;
	int rem;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (unsigned short);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3: hash += get16bits (data);
			hash ^= hash << 16;
			hash ^= data[sizeof (unsigned short)] << 18;
			hash += hash >> 11;
			break;
		case 2: hash += get16bits (data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
		case 1: hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

// data len hashIn ... hashOut
FORTHOP( hashOp )
{
    unsigned int hashVal = SPOP;
    int len = SPOP;
    const char* pData = (const char*) (SPOP);
    hashVal = SuperFastHash( pData, len, hashVal );
    SPUSH( hashVal );
}

namespace
{
    struct qsInfo
    {
        void*   temp;
        int     offset;
        int     elementSize;
        int     compareSize;
        int     (*compare)( const void* a, const void* b, int );
    };

    int u8Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((unsigned char *) pA);
        int b = (int) *((unsigned char *) pB);
        return a - b;
    }

    int s8Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((char *) pA);
        int b = (int) *((char *) pB);
        return a - b;
    }

    int u16Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((unsigned short *) pA);
        int b = (int) *((unsigned short *) pB);
        return a - b;
    }

    int s16Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((short *) pA);
        int b = (int) *((short *) pB);
        return a - b;
    }

    int u32Compare( const void* pA, const void* pB, int )
    {
        unsigned int a = *((unsigned int *) pA);
        unsigned int b = *((unsigned int *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int s32Compare( const void* pA, const void* pB, int )
    {
        int a = *((int *) pA);
        int b = *((int *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int f32Compare( const void* pA, const void* pB, int )
    {
        float a = *((float *) pA);
        float b = *((float *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int f64Compare( const void* pA, const void* pB, int )
    {
        double a = *((double *) pA);
        double b = *((double *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int u64Compare( const void* pA, const void* pB, int )
    {
        unsigned long long a = *((unsigned long long *) pA);
        unsigned long long b = *((unsigned long long *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int s64Compare( const void* pA, const void* pB, int )
    {
        long long a = *((long long *) pA);
        long long b = *((long long *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int strCompare( const void* pA, const void* pB, int numChars )
    {
        char* a = (char *) pA;
        char* b = (char *) pB;
        return strncmp( a, b, numChars );
    }

    int qsPartition( char* pData, qsInfo* pQS, int left, int right )
    {
        int elementSize = pQS->elementSize;
        char* pLeft = pData + (pQS->elementSize * left) + pQS->offset;
        char* pRight = pData + (pQS->elementSize * right) + pQS->offset;
        // pivot = pData[left]
        char* pivot = (char *)(pQS->temp);
        int compareSize = pQS->compareSize;
        memcpy( pivot, pLeft, compareSize );

	    while ( true )
	    {
		    // while (a[left] < pivot) left++;
            while ( pQS->compare( pLeft, pivot, compareSize ) < 0 )
            {
                left++;
                pLeft += elementSize;
            }

		    // while (a[right] > pivot) right--;
            while ( pQS->compare( pRight, pivot, compareSize ) > 0 )
            {
                right--;
                pRight -= elementSize;
            }

            if ( pQS->compare( pLeft, pRight, compareSize ) == 0 )
            {
                left++;
                pLeft += elementSize;
            }
		    else if ( left < right )
		    {
			    // swap(a[left], a[right]);
                memcpy( pivot, pLeft, pQS->elementSize );
                memcpy( pLeft, pRight, pQS->elementSize );
                memcpy( pRight, pivot, pQS->elementSize );
		    }
		    else
		    {
			    return right;
		    }
	    }
    }

    void qsStep( void* pData, qsInfo* pQS, int left, int right )
    {
	    if ( left < right )
	    {
		    int pivot = qsPartition( (char *) pData, pQS, left, right );
		    qsStep( pData, pQS, left, (pivot-1) );
		    qsStep( pData, pQS, (pivot+1), right );
	    }
    }

}

FORTHOP( qsortOp )
{
    // qsort( ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET )
    NEEDS(6);
    qsInfo qs;

    qs.offset = SPOP;
    long compareType = SPOP;
    qs.elementSize = SPOP;
    long numElements = SPOP;
    void* pArrayBase = (void *)(SPOP);

    qs.temp = malloc( qs.elementSize );

    switch ( CODE_TO_BASE_TYPE(compareType) )
    {
    case kBaseTypeByte:
        qs.compareSize = 1;
        qs.compare = s8Compare;
        break;

    case kBaseTypeUByte:
        qs.compareSize = 1;
        qs.compare = u8Compare;
        break;

    case kBaseTypeShort:
        qs.compareSize = 2;
        qs.compare = s16Compare;
        break;

    case kBaseTypeUShort:
        qs.compareSize = 2;
        qs.compare = u16Compare;
        break;

    case kBaseTypeInt:
        qs.compareSize = 4;
        qs.compare = s32Compare;
        break;

    case kBaseTypeUInt:
        qs.compareSize = 4;
        qs.compare = u32Compare;
        break;

    case kBaseTypeLong:
        qs.compareSize = 8;
        qs.compare = s64Compare;
        break;

    case kBaseTypeULong:
        qs.compareSize = 8;
        qs.compare = u64Compare;
        break;

    case kBaseTypeFloat:
        qs.compareSize = 4;
        qs.compare = f32Compare;
        break;

    case kBaseTypeDouble:
        qs.compareSize = 8;
        qs.compare = f64Compare;
        break;

    case kBaseTypeString:
        qs.compareSize = CODE_TO_STRING_BYTES(compareType);
        qs.compare = strCompare;
        break;

    default:
        break;
    }

    qsStep( pArrayBase, &qs, 0, (numElements - 1) );

    free( qs.temp );
}

FORTHOP(bsearchOp)
{
    // bsearch( KEY_ADDR ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET )
    NEEDS(7);

    long offset = SPOP;
    long compareType = SPOP;
    long elementSize = SPOP;
    long numElements = SPOP;
    char* pData = (char *)(SPOP);
    char* pKey = (char *)(SPOP);
    long compareSize;
    int (*compare)( const void* a, const void* b, int );

    switch ( CODE_TO_BASE_TYPE(compareType) )
    {
    case kBaseTypeByte:
        compareSize = 1;
        compare = s8Compare;
        break;

    case kBaseTypeUByte:
        compareSize = 1;
        compare = u8Compare;
        break;

    case kBaseTypeShort:
        compareSize = 2;
        compare = s16Compare;
        break;

    case kBaseTypeUShort:
        compareSize = 2;
        compare = u16Compare;
        break;

    case kBaseTypeInt:
        compareSize = 4;
        compare = s32Compare;
        break;

    case kBaseTypeUInt:
        compareSize = 4;
        compare = u32Compare;
        break;

    case kBaseTypeLong:
        compareSize = 8;
        compare = s64Compare;
        break;

    case kBaseTypeULong:
        compareSize = 8;
        compare = u64Compare;
        break;

    case kBaseTypeFloat:
        compareSize = 4;
        compare = f32Compare;
        break;

    case kBaseTypeDouble:
        compareSize = 8;
        compare = f64Compare;
        break;

    case kBaseTypeString:
        compareSize = CODE_TO_STRING_BYTES(compareType);
        compare = strCompare;
        break;

    default:
        break;
    }

    int first = 0;
    int last = numElements - 1;
    while ( first <= last )
    {
        int mid = (first + last) / 2;  // compute mid point.
        char* pMid = pData + (elementSize * mid) + offset;
        int result = compare( pKey, pMid, compareSize );
        if ( result > 0 )           //if (key > sortedArray[mid]) 
        {
            first = mid + 1;  // repeat search in top half.
        }
        else if ( result < 0 )                 // if (key < sortedArray[mid]) 
        {
            last = mid - 1; // repeat search in bottom half.
        }
        else
        {
            SPUSH( mid );     // found it. return position
            return;
        }
    }
    // key is not in array, return negative of insertion point+1
    //  the one is added so we can distinguish between an exact match with element 0 (return 0)
    //  and the key value being less than element 0 (return -1)
    SPUSH( -(first + 1) );    // failed to find key
}

///////////////////////////////////////////
//  conditional compilation
///////////////////////////////////////////
FORTHOP( poundIfOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundIf();
}

FORTHOP( poundIfdefOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundIfdef( true );
}

FORTHOP( poundIfndefOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundIfdef( false );
}

FORTHOP( poundElseOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundElse();
}

FORTHOP( poundEndifOp )
{
    ForthShell *pShell = GET_ENGINE->GetShell();
    pShell->PoundEndif();
}

FORTHOP( setTraceOp )
{
	int traceFlags = SPOP;
	GET_ENGINE->SetTraceFlags( traceFlags );
}

///////////////////////////////////////////
//  threads
///////////////////////////////////////////

FORTHOP( createThreadOp )
{
    long threadOp  = SPOP;
    int returnStackSize = (int)(SPOP);
    int paramStackSize = (int)(SPOP);
    ForthThread* pThread = GET_ENGINE->CreateThread( threadOp, paramStackSize, returnStackSize );
    SPUSH( (long) pThread );
}

FORTHOP( destroyThreadOp )
{
    ForthThread* pThread = (ForthThread*)(SPOP);
    GET_ENGINE->DestroyThread( pThread );
}

FORTHOP( threadGetStateOp )
{
    ForthThread* pThread = (ForthThread*)(SPOP);
	SPUSH( (long) (pThread->GetCoreState()) );
}

FORTHOP( stepThreadOp )
{
    ForthThread* pThread = (ForthThread*)(SPOP);
	ForthCoreState* pThreadCore = pThread->GetCoreState();
	long op = *(pThreadCore->IP)++;
    long result;
    ForthEngine *pEngine = GET_ENGINE;
	if ( pEngine->GetFastMode() )
	{
		result = (long) InterpretOneOpFast( pCore, op );
	}
	else
	{
		result = (long) InterpretOneOp( pCore, op );
	}
    SPUSH( result );
}

FORTHOP( startThreadOp )
{
    ForthThread* pThread = (ForthThread*)(SPOP);
    long result = pThread->Start();
    SPUSH( result );
}

FORTHOP( exitThreadOp )
{
    ForthThread* pThread = (ForthThread*)(pCore->pThread);
    pThread->Exit();
}

#ifdef WIN32
///////////////////////////////////////////
//  Windows support
///////////////////////////////////////////

FORTHOP( createEventOp )
{
    LPCTSTR lpName = (LPCTSTR)(SPOP);
    BOOL bInitialState = (BOOL)(SPOP);
    BOOL bManualReset = (BOOL)(SPOP);
    LPSECURITY_ATTRIBUTES lpEventAttributes = (LPSECURITY_ATTRIBUTES) (SPOP);
    HANDLE result = ::CreateEvent( lpEventAttributes, bManualReset, bInitialState, lpName );
    SPUSH( (long) result );
}

FORTHOP( closeHandleOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::CloseHandle( hObject );
    SPUSH( (long) result );
}

FORTHOP( setEventOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::SetEvent( hObject );
    SPUSH( (long) result );
}

FORTHOP( resetEventOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::ResetEvent( hObject );
    SPUSH( (long) result );
}

FORTHOP( pulseEventOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::PulseEvent( hObject );
    SPUSH( (long) result );
}

FORTHOP( getLastErrorOp )
{
    DWORD result = ::GetLastError();
    SPUSH( (long) result );
}

FORTHOP( waitForSingleObjectOp )
{
    DWORD dwMilliseconds = (DWORD)(SPOP);
    HANDLE hHandle = (HANDLE)(SPOP);

    DWORD result = ::WaitForSingleObject( hHandle, dwMilliseconds );
    SPUSH( (long) result );
}

FORTHOP( waitForMultipleObjectsOp )
{
    DWORD dwMilliseconds = (DWORD)(SPOP);
    BOOL bWaitAll = (BOOL)(SPOP);
    const HANDLE* lpHandles = (const HANDLE*)(SPOP);
    DWORD nCount = (DWORD)(SPOP);

    DWORD result = ::WaitForMultipleObjects( nCount, lpHandles, bWaitAll, dwMilliseconds );
    SPUSH( (long) result );
}

FORTHOP( initializeCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::InitializeCriticalSection( pCriticalSection );
}

FORTHOP( deleteCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::DeleteCriticalSection( pCriticalSection );
}

FORTHOP( enterCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::EnterCriticalSection( pCriticalSection );
}

FORTHOP( leaveCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::LeaveCriticalSection( pCriticalSection );
}

FORTHOP( findFirstFileOp )
{
    LPWIN32_FIND_DATA pOutData = (LPWIN32_FIND_DATA)(SPOP);
    LPCTSTR pPath = (LPCTSTR)(SPOP);
    HANDLE result = ::FindFirstFile( pPath, pOutData );
    SPUSH( (long) result );
}

FORTHOP( findNextFileOp )
{
    LPWIN32_FIND_DATA pOutData = (LPWIN32_FIND_DATA)(SPOP);
    HANDLE searchHandle = (HANDLE)(SPOP);
    BOOL result = ::FindNextFile( searchHandle, pOutData );
    SPUSH( (long) result );
}

FORTHOP( findCloseOp )
{
    HANDLE searchHandle = (HANDLE)(SPOP);
    BOOL result = ::FindClose( searchHandle );
    SPUSH( (long) result );
}

FORTHOP( windowsConstantsOp )
{
    static int winConstants[5] =
    {
        (sizeof(winConstants)/sizeof(int)) - 1,
        sizeof(TCHAR),
        MAX_PATH,
        sizeof(WIN32_FIND_DATA),
        sizeof(CRITICAL_SECTION)
    };
    SPUSH( (long) (&(winConstants[0])) );
}

#endif

///////////////////////////////////////////
//  Network support
///////////////////////////////////////////

FORTHOP( clientOp )
{
	long ipAddress = SPOP;
	ForthEngine *pEngine = GET_ENGINE;
	int result = ForthClientMainLoop( pEngine, ipAddress, FORTH_SERVER_PORT );
	SPUSH( result );
}

FORTHOP( serverOp )
{
	ForthEngine *pEngine = GET_ENGINE;
	int result = ForthServerMainLoop( pEngine, false, FORTH_SERVER_PORT );
	SPUSH( result );
}

FORTHOP( shutdownOp )
{
    SET_STATE( kResultShutdown );
}

// NOTE: the order of the first few entries in this table must agree
// with the list near the top of the file!  (look for COMPILED_OP)

extern GFORTHOP( doByteOp );
extern GFORTHOP( doUByteOp );
extern GFORTHOP( doShortOp );
extern GFORTHOP( doUShortOp );
extern GFORTHOP( doIntOp );
extern GFORTHOP( doIntOp );
extern GFORTHOP( doFloatOp );
extern GFORTHOP( doDoubleOp );
extern GFORTHOP( doStringOp );
extern GFORTHOP( doOpOp );
extern GFORTHOP( doLongOp );
extern GFORTHOP( doObjectOp );
extern GFORTHOP( doByteArrayOp );
extern GFORTHOP( doUByteArrayOp );
extern GFORTHOP( doShortArrayOp );
extern GFORTHOP( doUShortArrayOp );
extern GFORTHOP( doIntArrayOp );
extern GFORTHOP( doIntArrayOp );
extern GFORTHOP( doFloatArrayOp );
extern GFORTHOP( doDoubleArrayOp );
extern GFORTHOP( doStringArrayOp );
extern GFORTHOP( doOpArrayOp );
extern GFORTHOP( doLongArrayOp );
extern GFORTHOP( doObjectArrayOp );

typedef struct {
   const char       *name;
   ulong            flags;
   ulong            value;
   int				index;
} baseDictionaryCompiledEntry;

// helper macro for built-in op entries in baseDictionary
#define NATIVE_DEF( func, funcName )  { funcName, kOpNative, (ulong) func }
#define NATIVE_COMPILED_DEF( func, funcName, index )  { funcName, kOpNative, (ulong) func, index }

// helper macro for built-in op entries in baseDictionary
#define OP_DEF( func, funcName )  { funcName, kOpCCode, (ulong) func }
#define OP_COMPILED_DEF( func, funcName, index )  { funcName, kOpCCode, (ulong) func, index }

// helper macro for ops which have precedence (execute at compile time)
#define PRECOP_DEF( func, funcName )  { funcName, kOpCCodeImmediate, (ulong) func }
#define PRECOP_COMPILED_DEF( func, funcName, index )  { funcName, kOpCCodeImmediate, (ulong) func }

extern GFORTHOP( abortBop ); extern GFORTHOP( dropBop ); extern GFORTHOP( doDoesBop ); extern GFORTHOP( litBop ); extern GFORTHOP( dlitBop ); extern GFORTHOP( doVariableBop );
extern GFORTHOP( doConstantBop ); extern GFORTHOP( doDConstantBop ); extern GFORTHOP( doneBop ); extern GFORTHOP( doByteBop ); extern GFORTHOP( doUByteBop ); extern GFORTHOP( doShortBop );
extern GFORTHOP( doUShortBop ); extern GFORTHOP( doIntBop ); extern GFORTHOP( doLongBop ); extern GFORTHOP( doFloatBop );
extern GFORTHOP( doDoubleBop ); extern GFORTHOP( doStringBop ); extern GFORTHOP( doOpBop ); extern GFORTHOP( doObjectBop ); extern GFORTHOP( doExitBop ); extern GFORTHOP( doExitLBop );
extern GFORTHOP( doExitMBop ); extern GFORTHOP( doExitMLBop ); extern GFORTHOP( doByteArrayBop ); extern GFORTHOP( doUByteArrayBop ); extern GFORTHOP( doShortArrayBop ); extern GFORTHOP( doUShortArrayBop );
extern GFORTHOP( doIntArrayBop ); extern GFORTHOP( doIntArrayBop ); extern GFORTHOP( doLongArrayBop ); extern GFORTHOP( doLongArrayBop ); extern GFORTHOP( doFloatArrayBop ); extern GFORTHOP( doDoubleArrayBop );
extern GFORTHOP( doStringArrayBop ); extern GFORTHOP( doOpArrayBop ); extern GFORTHOP( doObjectArrayBop ); extern GFORTHOP( initStringBop ); extern GFORTHOP( plusBop ); extern GFORTHOP( fetchBop );
extern GFORTHOP( doStructBop ); extern GFORTHOP( doStructArrayBop ); extern GFORTHOP( doDoBop ); extern GFORTHOP( doLoopBop ); extern GFORTHOP( doLoopNBop ); extern GFORTHOP( dfetchBop );
extern GFORTHOP( vocabToClassBop ); extern GFORTHOP( addressOfBop ); extern GFORTHOP( intoBop ); extern GFORTHOP( addToBop ); extern GFORTHOP( subtractFromBop ); extern GFORTHOP( doCheckDoBop );
extern GFORTHOP( thisBop ); extern GFORTHOP( thisDataBop ); extern GFORTHOP( thisMethodsBop ); extern GFORTHOP( executeBop ); extern GFORTHOP( callBop ); extern GFORTHOP( gotoBop );
extern GFORTHOP( iBop ); extern GFORTHOP( jBop ); extern GFORTHOP( unloopBop ); extern GFORTHOP( leaveBop ); extern GFORTHOP( hereBop ); extern GFORTHOP( addressOfBop );
extern GFORTHOP( intoBop ); extern GFORTHOP( addToBop ); extern GFORTHOP( subtractFromBop ); extern GFORTHOP( removeEntryBop ); extern GFORTHOP( entryLengthBop ); extern GFORTHOP( numEntriesBop );
extern GFORTHOP( minusBop ); extern GFORTHOP( timesBop ); extern GFORTHOP( utimesBop ); extern GFORTHOP( times2Bop ); extern GFORTHOP( times4Bop ); extern GFORTHOP( times8Bop );
extern GFORTHOP( divideBop ); extern GFORTHOP( divide2Bop ); extern GFORTHOP( divide4Bop ); extern GFORTHOP( divide8Bop ); extern GFORTHOP( divmodBop ); extern GFORTHOP( modBop );
extern GFORTHOP( negateBop ); extern GFORTHOP( fplusBop ); extern GFORTHOP( fminusBop ); extern GFORTHOP( ftimesBop ); extern GFORTHOP( fdivideBop ); extern GFORTHOP( fEqualsBop );
extern GFORTHOP( fNotEqualsBop ); extern GFORTHOP( fGreaterThanBop ); extern GFORTHOP( fGreaterEqualsBop ); extern GFORTHOP( fLessThanBop ); extern GFORTHOP( fLessEqualsBop ); extern GFORTHOP( fEqualsZeroBop );
extern GFORTHOP( fNotEqualsZeroBop ); extern GFORTHOP( fGreaterThanZeroBop ); extern GFORTHOP( fGreaterEqualsZeroBop ); extern GFORTHOP( fLessThanZeroBop ); extern GFORTHOP( fLessEqualsZeroBop ); extern GFORTHOP( fWithinBop );
extern GFORTHOP( fMinBop ); extern GFORTHOP( fMaxBop ); extern GFORTHOP( dplusBop ); extern GFORTHOP( dminusBop ); extern GFORTHOP( dtimesBop ); extern GFORTHOP( ddivideBop );
extern GFORTHOP( dEqualsBop ); extern GFORTHOP( dNotEqualsBop ); extern GFORTHOP( dGreaterThanBop ); extern GFORTHOP( dGreaterEqualsBop ); extern GFORTHOP( dLessThanBop ); extern GFORTHOP( dLessEqualsBop );
extern GFORTHOP( dEqualsZeroBop ); extern GFORTHOP( dNotEqualsZeroBop ); extern GFORTHOP( dGreaterThanZeroBop ); extern GFORTHOP( dGreaterEqualsZeroBop ); extern GFORTHOP( dLessThanZeroBop ); extern GFORTHOP( dLessEqualsZeroBop );
extern GFORTHOP( dWithinBop ); extern GFORTHOP( dMinBop ); extern GFORTHOP( dMaxBop ); extern GFORTHOP( dsinBop ); extern GFORTHOP( dasinBop ); extern GFORTHOP( dcosBop );
extern GFORTHOP( dacosBop ); extern GFORTHOP( dtanBop ); extern GFORTHOP( datanBop ); extern GFORTHOP( datan2Bop ); extern GFORTHOP( dexpBop ); extern GFORTHOP( dlnBop );
extern GFORTHOP( dlog10Bop ); extern GFORTHOP( dpowBop ); extern GFORTHOP( dsqrtBop ); extern GFORTHOP( dceilBop ); extern GFORTHOP( dfloorBop ); extern GFORTHOP( dabsBop );
extern GFORTHOP( dldexpBop ); extern GFORTHOP( dfrexpBop ); extern GFORTHOP( dmodfBop ); extern GFORTHOP( dfmodBop ); extern GFORTHOP( i2fBop ); extern GFORTHOP( i2dBop );
extern GFORTHOP( f2iBop ); extern GFORTHOP( f2dBop ); extern GFORTHOP( d2iBop ); extern GFORTHOP( d2fBop ); extern GFORTHOP( orBop ); extern GFORTHOP( andBop );
extern GFORTHOP( xorBop ); extern GFORTHOP( invertBop ); extern GFORTHOP( lshiftBop ); extern GFORTHOP( rshiftBop ); extern GFORTHOP( urshiftBop ); extern GFORTHOP( notBop );
extern GFORTHOP( trueBop ); extern GFORTHOP( falseBop ); extern GFORTHOP( nullBop ); extern GFORTHOP( dnullBop ); extern GFORTHOP( equalsBop ); extern GFORTHOP( notEqualsBop );
extern GFORTHOP( greaterThanBop ); extern GFORTHOP( greaterEqualsBop ); extern GFORTHOP( lessThanBop ); extern GFORTHOP( lessEqualsBop ); extern GFORTHOP( equalsZeroBop ); extern GFORTHOP( notEqualsZeroBop );
extern GFORTHOP( greaterThanZeroBop ); extern GFORTHOP( greaterEqualsZeroBop ); extern GFORTHOP( lessThanZeroBop ); extern GFORTHOP( lessEqualsZeroBop ); extern GFORTHOP( unsignedGreaterThanBop ); extern GFORTHOP( unsignedLessThanBop );
extern GFORTHOP( withinBop ); extern GFORTHOP( minBop ); extern GFORTHOP( maxBop ); extern GFORTHOP( rpushBop ); extern GFORTHOP( rpopBop ); extern GFORTHOP( rpeekBop );
extern GFORTHOP( rdropBop ); extern GFORTHOP( rpBop ); extern GFORTHOP( rzeroBop ); extern GFORTHOP( dupBop ); extern GFORTHOP( dupNonZeroBop ); extern GFORTHOP( swapBop );
extern GFORTHOP( overBop ); extern GFORTHOP( rotBop ); extern GFORTHOP( reverseRotBop ); extern GFORTHOP( nipBop ); extern GFORTHOP( tuckBop ); extern GFORTHOP( pickBop );
extern GFORTHOP( spBop ); extern GFORTHOP( szeroBop ); extern GFORTHOP( fpBop ); extern GFORTHOP( ipBop ); extern GFORTHOP( ddupBop );
extern GFORTHOP( dswapBop ); extern GFORTHOP( ddropBop ); extern GFORTHOP( doverBop ); extern GFORTHOP( drotBop ); extern GFORTHOP( startTupleBop ); extern GFORTHOP( endTupleBop );
extern GFORTHOP( storeBop ); extern GFORTHOP( storeNextBop ); extern GFORTHOP( fetchNextBop ); extern GFORTHOP( cstoreBop ); extern GFORTHOP( cfetchBop ); extern GFORTHOP( cstoreNextBop );
extern GFORTHOP( cfetchNextBop ); extern GFORTHOP( scfetchBop ); extern GFORTHOP( c2lBop ); extern GFORTHOP( wstoreBop ); extern GFORTHOP( wfetchBop ); extern GFORTHOP( wstoreNextBop );
extern GFORTHOP( wfetchNextBop ); extern GFORTHOP( swfetchBop ); extern GFORTHOP( w2lBop ); extern GFORTHOP( dstoreBop ); extern GFORTHOP( dstoreNextBop ); extern GFORTHOP( dfetchNextBop );
extern GFORTHOP( memcpyBop ); extern GFORTHOP( memsetBop ); extern GFORTHOP( setVarActionBop ); extern GFORTHOP( getVarActionBop ); extern GFORTHOP( byteVarActionBop ); extern GFORTHOP( ubyteVarActionBop );
extern GFORTHOP( shortVarActionBop ); extern GFORTHOP( ushortVarActionBop ); extern GFORTHOP( intVarActionBop ); extern GFORTHOP( longVarActionBop ); extern GFORTHOP( floatVarActionBop ); extern GFORTHOP( doubleVarActionBop );
extern GFORTHOP( stringVarActionBop ); extern GFORTHOP( opVarActionBop ); extern GFORTHOP( objectVarActionBop ); extern GFORTHOP( strcpyBop ); extern GFORTHOP( strncpyBop ); extern GFORTHOP( strlenBop );
extern GFORTHOP( strcatBop ); extern GFORTHOP( strncatBop ); extern GFORTHOP( strchrBop ); extern GFORTHOP( strrchrBop ); extern GFORTHOP( strcmpBop ); extern GFORTHOP( stricmpBop );
extern GFORTHOP( strstrBop ); extern GFORTHOP( strtokBop ); extern GFORTHOP( fopenBop ); extern GFORTHOP( fcloseBop ); extern GFORTHOP( fseekBop ); extern GFORTHOP( freadBop );
extern GFORTHOP( fwriteBop ); extern GFORTHOP( fgetcBop ); extern GFORTHOP( fputcBop ); extern GFORTHOP( feofBop ); extern GFORTHOP( fexistsBop ); extern GFORTHOP( ftellBop );
extern GFORTHOP( flenBop ); extern GFORTHOP( fgetsBop ); extern GFORTHOP( fputsBop );

baseDictionaryCompiledEntry baseCompiledDictionary[] =
{
	NATIVE_COMPILED_DEF(    abortBop,                "abort",			OP_ABORT ),
    NATIVE_COMPILED_DEF(    dropBop,                 "drop",			OP_DROP ),
    NATIVE_COMPILED_DEF(    doDoesBop,               "_doDoes",			OP_DO_DOES ),
    NATIVE_COMPILED_DEF(    litBop,                  "lit",				OP_INT_VAL ),
    NATIVE_COMPILED_DEF(    litBop,                  "flit",			OP_FLOAT_VAL ),
    NATIVE_COMPILED_DEF(    dlitBop,                 "dlit",			OP_DOUBLE_VAL ),
    NATIVE_COMPILED_DEF(    dlitBop,                 "2lit",			OP_LONG_VAL ),
    NATIVE_COMPILED_DEF(    doVariableBop,           "_doVariable",		OP_DO_VAR ),
    NATIVE_COMPILED_DEF(    doConstantBop,           "_doConstant",		OP_DO_CONSTANT ),
    NATIVE_COMPILED_DEF(    doDConstantBop,          "_doDConstant",	OP_DO_DCONSTANT ),
    NATIVE_COMPILED_DEF(    doneBop,                 "done",			OP_DONE ),
    NATIVE_COMPILED_DEF(    doByteBop,               "_doByte",			OP_DO_BYTE ),
    NATIVE_COMPILED_DEF(    doUByteBop,              "_doUByte",		OP_DO_UBYTE ),
    NATIVE_COMPILED_DEF(    doShortBop,              "_doShort",		OP_DO_SHORT ),
	NATIVE_COMPILED_DEF(    doUShortBop,             "_doUShort",		OP_DO_USHORT ),
    NATIVE_COMPILED_DEF(    doIntBop,                "_doInt",			OP_DO_INT ),
    NATIVE_COMPILED_DEF(    doIntBop,                "_doUInt",			OP_DO_UINT ),
    NATIVE_COMPILED_DEF(    doLongBop,               "_doLong",			OP_DO_LONG ),
    NATIVE_COMPILED_DEF(    doLongBop,               "_doULong",		OP_DO_ULONG ),
    NATIVE_COMPILED_DEF(    doFloatBop,              "_doFloat",		OP_DO_FLOAT ),
    NATIVE_COMPILED_DEF(    doDoubleBop,             "_doDouble",		OP_DO_DOUBLE ),
    NATIVE_COMPILED_DEF(    doStringBop,             "_doString",		OP_DO_STRING ),
    NATIVE_COMPILED_DEF(    doOpBop,                 "_doOp",			OP_DO_OP ),
    NATIVE_COMPILED_DEF(    doObjectBop,             "_doObject",		OP_DO_OBJECT ),
    NATIVE_COMPILED_DEF(    doExitBop,               "_exit",			OP_DO_EXIT ),      // exit normal op with no vars
    NATIVE_COMPILED_DEF(    doExitLBop,              "_exitL",			OP_DO_EXIT_L ),    // exit normal op with local vars
    NATIVE_COMPILED_DEF(    doExitMBop,              "_exitM",			OP_DO_EXIT_M ),    // exit method op with no vars
    NATIVE_COMPILED_DEF(    doExitMLBop,             "_exitML",			OP_DO_EXIT_ML ),   // exit method op with local vars
	NATIVE_COMPILED_DEF(    doByteArrayBop,          "_doByteArray",	OP_DO_BYTE_ARRAY ),
    NATIVE_COMPILED_DEF(    doUByteArrayBop,         "_doUByteArray",	OP_DO_UBYTE_ARRAY ),
    NATIVE_COMPILED_DEF(    doShortArrayBop,         "_doShortArray",	OP_DO_SHORT_ARRAY ),
    NATIVE_COMPILED_DEF(    doUShortArrayBop,        "_doUShortArray",	OP_DO_USHORT_ARRAY ),	// 32
    NATIVE_COMPILED_DEF(    doIntArrayBop,           "_doIntArray",		OP_DO_INT_ARRAY ),
    NATIVE_COMPILED_DEF(    doIntArrayBop,           "_doUIntArray",	OP_DO_UINT_ARRAY ),
    NATIVE_COMPILED_DEF(    doLongArrayBop,          "_doLongArray",	OP_DO_LONG_ARRAY ),
    NATIVE_COMPILED_DEF(    doLongArrayBop,          "_doULongArray",	OP_DO_ULONG_ARRAY ),	// 36
    NATIVE_COMPILED_DEF(    doFloatArrayBop,         "_doFloatArray",	OP_DO_FLOAT_ARRAY ),
    NATIVE_COMPILED_DEF(    doDoubleArrayBop,        "_doDoubleArray",	OP_DO_DOUBLE_ARRAY ),
    NATIVE_COMPILED_DEF(    doStringArrayBop,        "_doStringArray",	OP_DO_STRING_ARRAY ),
    NATIVE_COMPILED_DEF(    doOpArrayBop,            "_doOpArray",		OP_DO_OP_ARRAY ),		// 40
    NATIVE_COMPILED_DEF(    doObjectArrayBop,        "_doObjectArray",	OP_DO_OBJECT_ARRAY ),
	NATIVE_COMPILED_DEF(    initStringBop,           "initString",		OP_INIT_STRING ),
    NATIVE_COMPILED_DEF(    plusBop,                 "+",				OP_PLUS ),				// 44
    NATIVE_COMPILED_DEF(    fetchBop,                "@",				OP_FETCH ),
    NATIVE_COMPILED_DEF(    doStructBop,             "_doStruct",		OP_DO_STRUCT ),
    NATIVE_COMPILED_DEF(    doStructArrayBop,        "_doStructArray",	OP_DO_STRUCT_ARRAY ),	// 48
    NATIVE_COMPILED_DEF(    doDoBop,                 "_do",				OP_DO_DO ),				// 52
    NATIVE_COMPILED_DEF(    doLoopBop,               "_loop",			OP_DO_LOOP ),
    NATIVE_COMPILED_DEF(    doLoopNBop,              "_+loop",			OP_DO_LOOPN ),
    NATIVE_COMPILED_DEF(    dfetchBop,               "2@",				OP_DFETCH ),				// 56
    NATIVE_COMPILED_DEF(    vocabToClassBop,         "vocabToClass",	OP_VOCAB_TO_CLASS ),
    // the order of the next four opcodes has to match the order of kVarRef...kVarMinusStore
    NATIVE_COMPILED_DEF(    addressOfBop,            "ref",				OP_REF ),
	NATIVE_COMPILED_DEF(    intoBop,                 "->",				OP_INTO ),				// 60
    NATIVE_COMPILED_DEF(    addToBop,                "->+",				OP_INTO_PLUS ),
    NATIVE_COMPILED_DEF(    subtractFromBop,         "->-",				OP_INTO_MINUS ),
	NATIVE_COMPILED_DEF(    doCheckDoBop,            "_?do",			OP_DO_CHECKDO ),

	OP_COMPILED_DEF(		doVocabOp,              "_doVocab",			OP_DO_VOCAB ),
	OP_COMPILED_DEF(		initStringArrayOp,      "initStringArray",	OP_INIT_STRING_ARRAY ),
    OP_COMPILED_DEF(		badOpOp,                "badOp",			OP_BAD_OP ),
	OP_COMPILED_DEF(		doStructTypeOp,         "_doStructType",	OP_DO_STRUCT_TYPE ),
    OP_COMPILED_DEF(		doClassTypeOp,          "_doClassType",		OP_DO_CLASS_TYPE ),
    PRECOP_COMPILED_DEF(	doEnumOp,               "_doEnum",			OP_DO_ENUM ),
    OP_COMPILED_DEF(		doNewOp,                "_doNew",			OP_DO_NEW ),
    OP_COMPILED_DEF(		allocObjectOp,          "_allocObject",		OP_ALLOC_OBJECT ),
	OP_COMPILED_DEF(		superOp,                "super",			OP_SUPER ),
    OP_COMPILED_DEF(		endBuildsOp,            "_endBuilds",		OP_END_BUILDS ),
    // following must be last in table
    OP_COMPILED_DEF(		NULL,                   NULL,					-1 )
};



baseDictionaryEntry baseDictionary[] =
{
	NATIVE_DEF(    thisBop,                 "this" ),
    NATIVE_DEF(    thisDataBop,             "thisData" ),
    NATIVE_DEF(    thisMethodsBop,          "thisMethods" ),
    NATIVE_DEF(    executeBop,              "execute" ),
    NATIVE_DEF(    callBop,                 "call" ),
    NATIVE_DEF(    gotoBop,                 "goto" ),
    NATIVE_DEF(    iBop,                    "i" ),
    NATIVE_DEF(    jBop,                    "j" ),
    NATIVE_DEF(    unloopBop,               "unloop" ),
    NATIVE_DEF(    leaveBop,                "leave" ),
    NATIVE_DEF(    hereBop,                 "here" ),
    // vocabulary varActions
    NATIVE_DEF(    addressOfBop,            "getNewest" ),
    NATIVE_DEF(    intoBop,                 "findEntry" ),
    NATIVE_DEF(    addToBop,                "findEntryValue" ),
    NATIVE_DEF(    subtractFromBop,         "addEntry" ),
    NATIVE_DEF(    removeEntryBop,          "removeEntry" ),
    NATIVE_DEF(    entryLengthBop,          "entryLength" ),
    NATIVE_DEF(    numEntriesBop,           "numEntries" ),
    
    ///////////////////////////////////////////
    //  integer math
    ///////////////////////////////////////////
    NATIVE_DEF(    minusBop,                "-" ),
    NATIVE_DEF(    timesBop,                "*" ),
    NATIVE_DEF(    utimesBop,               "u*" ),
    NATIVE_DEF(    times2Bop,               "2*" ),
    NATIVE_DEF(    times4Bop,               "4*" ),
    NATIVE_DEF(    times8Bop,               "8*" ),
    NATIVE_DEF(    divideBop,               "/" ),
    NATIVE_DEF(    divide2Bop,              "2/" ),
    NATIVE_DEF(    divide4Bop,              "4/" ),
    NATIVE_DEF(    divide8Bop,              "8/" ),
    NATIVE_DEF(    divmodBop,               "/mod" ),
    NATIVE_DEF(    modBop,                  "mod" ),
    NATIVE_DEF(    negateBop,               "negate" ),
    
    ///////////////////////////////////////////
    //  single-precision fp math
    ///////////////////////////////////////////
    NATIVE_DEF(    fplusBop,                "f+" ),
    NATIVE_DEF(    fminusBop,               "f-" ),
    NATIVE_DEF(    ftimesBop,               "f*" ),
    NATIVE_DEF(    fdivideBop,              "f/" ),
    
    ///////////////////////////////////////////
    //  single-precision fp comparisons
    ///////////////////////////////////////////
    NATIVE_DEF(    fEqualsBop,               "f=" ),
    NATIVE_DEF(    fNotEqualsBop,            "f<>" ),
    NATIVE_DEF(    fGreaterThanBop,          "f>" ),
    NATIVE_DEF(    fGreaterEqualsBop,        "f>=" ),
    NATIVE_DEF(    fLessThanBop,             "f<" ),
    NATIVE_DEF(    fLessEqualsBop,           "f<=" ),
    NATIVE_DEF(    fEqualsZeroBop,           "f0=" ),
    NATIVE_DEF(    fNotEqualsZeroBop,        "f0<>" ),
    NATIVE_DEF(    fGreaterThanZeroBop,      "f0>" ),
    NATIVE_DEF(    fGreaterEqualsZeroBop,    "f0>=" ),
    NATIVE_DEF(    fLessThanZeroBop,         "f0<" ),
    NATIVE_DEF(    fLessEqualsZeroBop,       "f0<=" ),
    NATIVE_DEF(    fWithinBop,               "fwithin" ),
    NATIVE_DEF(    fMinBop,                  "fmin" ),
    NATIVE_DEF(    fMaxBop,                  "fmax" ),

    ///////////////////////////////////////////
    //  double-precision fp math
    ///////////////////////////////////////////
    NATIVE_DEF(    dplusBop,                "d+" ),
    NATIVE_DEF(    dminusBop,               "d-" ),
    NATIVE_DEF(    dtimesBop,               "d*" ),
    NATIVE_DEF(    ddivideBop,              "d/" ),


    ///////////////////////////////////////////
    //  double-precision fp comparisons
    ///////////////////////////////////////////
    NATIVE_DEF(    dEqualsBop,               "d=" ),
    NATIVE_DEF(    dNotEqualsBop,            "d<>" ),
    NATIVE_DEF(    dGreaterThanBop,          "d>" ),
    NATIVE_DEF(    dGreaterEqualsBop,        "d>=" ),
    NATIVE_DEF(    dLessThanBop,             "d<" ),
    NATIVE_DEF(    dLessEqualsBop,           "d<=" ),
    NATIVE_DEF(    dEqualsZeroBop,           "d0=" ),
    NATIVE_DEF(    dNotEqualsZeroBop,        "d0<>" ),
    NATIVE_DEF(    dGreaterThanZeroBop,      "d0>" ),
    NATIVE_DEF(    dGreaterEqualsZeroBop,    "d0>=" ),
    NATIVE_DEF(    dLessThanZeroBop,         "d0<" ),
    NATIVE_DEF(    dLessEqualsZeroBop,       "d0<=" ),
    NATIVE_DEF(    dWithinBop,               "dwithin" ),
    NATIVE_DEF(    dMinBop,                  "dmin" ),
    NATIVE_DEF(    dMaxBop,                  "dmax" ),

    ///////////////////////////////////////////
    //  double-precision fp functions
    ///////////////////////////////////////////
    NATIVE_DEF(    dsinBop,                 "dsin" ),
    NATIVE_DEF(    dasinBop,                "darcsin" ),
    NATIVE_DEF(    dcosBop,                 "dcos" ),
    NATIVE_DEF(    dacosBop,                "darccos" ),
    NATIVE_DEF(    dtanBop,                 "dtan" ),
    NATIVE_DEF(    datanBop,                "darctan" ),
    NATIVE_DEF(    datan2Bop,               "darctan2" ),
    NATIVE_DEF(    dexpBop,                 "dexp" ),
    NATIVE_DEF(    dlnBop,                  "dln" ),
    NATIVE_DEF(    dlog10Bop,               "dlog10" ),
    NATIVE_DEF(    dpowBop,                 "dpow" ),
    NATIVE_DEF(    dsqrtBop,                "dsqrt" ),
    NATIVE_DEF(    dceilBop,                "dceil" ),
    NATIVE_DEF(    dfloorBop,               "dfloor" ),
    NATIVE_DEF(    dabsBop,                 "dabs" ),
    NATIVE_DEF(    dldexpBop,               "dldexp" ),
    NATIVE_DEF(    dfrexpBop,               "dfrexp" ),
    NATIVE_DEF(    dmodfBop,                "dmodf" ),
    NATIVE_DEF(    dfmodBop,                "dfmod" ),
    
    ///////////////////////////////////////////
    //  integer/long/float/double conversion
    ///////////////////////////////////////////
    NATIVE_DEF(    i2fBop,                  "i2f" ), 
    NATIVE_DEF(    i2dBop,                  "i2d" ),
    NATIVE_DEF(    f2iBop,                  "f2i" ),
    NATIVE_DEF(    f2dBop,                  "f2d" ),
    NATIVE_DEF(    d2iBop,                  "d2i" ),
    NATIVE_DEF(    d2fBop,                  "d2f" ),
    
    ///////////////////////////////////////////
    //  bit-vector logic
    ///////////////////////////////////////////
    NATIVE_DEF(    orBop,                   "or" ),
    NATIVE_DEF(    andBop,                  "and" ),
    NATIVE_DEF(    xorBop,                  "xor" ),
    NATIVE_DEF(    invertBop,               "~" ),
    NATIVE_DEF(    lshiftBop,               "<<" ),
    NATIVE_DEF(    rshiftBop,               ">>" ),
    NATIVE_DEF(    urshiftBop,              "u>>" ),

    ///////////////////////////////////////////
    //  boolean logic
    ///////////////////////////////////////////
    NATIVE_DEF(    notBop,                  "not" ),
    NATIVE_DEF(    trueBop,                 "true" ),
    NATIVE_DEF(    falseBop,                "false" ),
    NATIVE_DEF(    nullBop,                 "null" ),
    NATIVE_DEF(    dnullBop,                "dnull" ),

    ///////////////////////////////////////////
    //  integer comparisons
    ///////////////////////////////////////////
    NATIVE_DEF(    equalsBop,               "=" ),
    NATIVE_DEF(    notEqualsBop,            "<>" ),
    NATIVE_DEF(    greaterThanBop,          ">" ),
    NATIVE_DEF(    greaterEqualsBop,        ">=" ),
    NATIVE_DEF(    lessThanBop,             "<" ),
    NATIVE_DEF(    lessEqualsBop,           "<=" ),
    NATIVE_DEF(    equalsZeroBop,           "0=" ),
    NATIVE_DEF(    notEqualsZeroBop,        "0<>" ),
    NATIVE_DEF(    greaterThanZeroBop,      "0>" ),
    NATIVE_DEF(    greaterEqualsZeroBop,    "0>=" ),
    NATIVE_DEF(    lessThanZeroBop,         "0<" ),
    NATIVE_DEF(    lessEqualsZeroBop,       "0<=" ),
    NATIVE_DEF(    unsignedGreaterThanBop,  "u>" ),
    NATIVE_DEF(    unsignedLessThanBop,     "u<" ),
    NATIVE_DEF(    withinBop,               "within" ),
    NATIVE_DEF(    minBop,                  "min" ),
    NATIVE_DEF(    maxBop,                  "max" ),
    
    ///////////////////////////////////////////
    //  stack manipulation
    ///////////////////////////////////////////
    NATIVE_DEF(    rpushBop,                ">r" ),
    NATIVE_DEF(    rpopBop,                 "r>" ),
    NATIVE_DEF(    rpeekBop,                "r@" ),
    NATIVE_DEF(    rdropBop,                "rdrop" ),
    NATIVE_DEF(    rpBop,                   "rp" ),
    NATIVE_DEF(    rzeroBop,                "r0" ),
    NATIVE_DEF(    dupBop,                  "dup" ),
    NATIVE_DEF(    checkDupBop,             "?dup" ),
    NATIVE_DEF(    swapBop,                 "swap" ),
    NATIVE_DEF(    overBop,                 "over" ),
    NATIVE_DEF(    rotBop,                  "rot" ),
    NATIVE_DEF(    reverseRotBop,           "-rot" ),
    NATIVE_DEF(    nipBop,                  "nip" ),
    NATIVE_DEF(    tuckBop,                 "tuck" ),
    NATIVE_DEF(    pickBop,                 "pick" ),
    NATIVE_DEF(    spBop,                   "sp" ),
    NATIVE_DEF(    szeroBop,                "s0" ),
    NATIVE_DEF(    fpBop,                   "fp" ),
    NATIVE_DEF(    ipBop,                   "ip" ),
    NATIVE_DEF(    ddupBop,                 "2dup" ),
    NATIVE_DEF(    dswapBop,                "2swap" ),
    NATIVE_DEF(    ddropBop,                "2drop" ),
    NATIVE_DEF(    doverBop,                "2over" ),
    NATIVE_DEF(    drotBop,                 "2rot" ),
    NATIVE_DEF(    startTupleBop,           "r[" ),
    NATIVE_DEF(    endTupleBop,             "]r" ),
#if 0
    NATIVE_DEF(    dreverseRotBop,          "-2rot" ),
    NATIVE_DEF(    dnipBop,                 "2nip" ),
    NATIVE_DEF(    dtuckBop,                "2tuck" ),
    NATIVE_DEF(    dpickBop,                "2pick" ),
    NATIVE_DEF(    drollBop,                "2roll" ),
    NATIVE_DEF(    ndropBop,                "ndrop" ),
    NATIVE_DEF(    ndupBop,                 "ndup" ),
    NATIVE_DEF(    npickBop,                "npick" ),
#endif

    ///////////////////////////////////////////
    //  memory store/fetch
    ///////////////////////////////////////////
    NATIVE_DEF(    storeBop,                "!" ),
    NATIVE_DEF(    storeNextBop,            "@!++" ),
    NATIVE_DEF(    fetchNextBop,            "@@++" ),
    NATIVE_DEF(    cstoreBop,               "c!" ),
    NATIVE_DEF(    cfetchBop,               "c@" ),
    NATIVE_DEF(    cstoreNextBop,           "c@!++" ),
    NATIVE_DEF(    cfetchNextBop,           "c@@++" ),
    NATIVE_DEF(    scfetchBop,              "sc@" ),
    NATIVE_DEF(    c2lBop,                  "c2l" ),
    NATIVE_DEF(    wstoreBop,               "w!" ),
    NATIVE_DEF(    wfetchBop,               "w@" ),
    NATIVE_DEF(    wstoreNextBop,           "w@!++" ),
    NATIVE_DEF(    wfetchNextBop,           "w@@++" ),
    NATIVE_DEF(    swfetchBop,              "sw@" ),
    NATIVE_DEF(    w2lBop,                  "w2l" ),
    NATIVE_DEF(    dstoreBop,               "2!" ),
    NATIVE_DEF(    dstoreNextBop,           "2@!++" ),
    NATIVE_DEF(    dfetchNextBop,           "2@@++" ),
    NATIVE_DEF(    memcpyBop,               "memcpy" ),
    NATIVE_DEF(    memsetBop,               "memset" ),
    NATIVE_DEF(    setVarActionBop,         "varAction!" ),
    NATIVE_DEF(    getVarActionBop,         "varAction@" ),
    NATIVE_DEF(    byteVarActionBop,        "byteVarAction" ),
    NATIVE_DEF(    ubyteVarActionBop,       "ubyteVarAction" ),
    NATIVE_DEF(    shortVarActionBop,       "shortVarAction" ),
    NATIVE_DEF(    ushortVarActionBop,      "ushortVarAction" ),
    NATIVE_DEF(    intVarActionBop,         "intVarAction" ),
    NATIVE_DEF(    longVarActionBop,        "longVarAction" ),
    NATIVE_DEF(    floatVarActionBop,       "floatVarAction" ),
    NATIVE_DEF(    doubleVarActionBop,      "doubleVarAction" ),
    NATIVE_DEF(    stringVarActionBop,      "stringVarAction" ),
    NATIVE_DEF(    opVarActionBop,          "opVarAction" ),
    NATIVE_DEF(    objectVarActionBop,      "objectVarAction" ),

    ///////////////////////////////////////////
    //  string manipulation
    ///////////////////////////////////////////
    NATIVE_DEF(    strcpyBop,               "strcpy" ),
    NATIVE_DEF(    strncpyBop,              "strncpy" ),
    NATIVE_DEF(    strlenBop,               "strlen" ),
    NATIVE_DEF(    strcatBop,               "strcat" ),
    NATIVE_DEF(    strncatBop,              "strncat" ),
    NATIVE_DEF(    strchrBop,               "strchr" ),
    NATIVE_DEF(    strrchrBop,              "strrchr" ),
    NATIVE_DEF(    strcmpBop,               "strcmp" ),
    NATIVE_DEF(    stricmpBop,              "stricmp" ),
    NATIVE_DEF(    strstrBop,               "strstr" ),
    NATIVE_DEF(    strtokBop,               "strtok" ),

    ///////////////////////////////////////////
    //  file manipulation
    ///////////////////////////////////////////
    NATIVE_DEF(    fopenBop,                "fopen" ),
    NATIVE_DEF(    fcloseBop,               "fclose" ),
    NATIVE_DEF(    fseekBop,                "fseek" ),
    NATIVE_DEF(    freadBop,                "fread" ),
    NATIVE_DEF(    fwriteBop,               "fwrite" ),
    NATIVE_DEF(    fgetcBop,                "fgetc" ),
    NATIVE_DEF(    fputcBop,                "fputc" ),
    NATIVE_DEF(    feofBop,                 "feof" ),
    NATIVE_DEF(    fexistsBop,              "fexists" ),
    NATIVE_DEF(    ftellBop,                "ftell" ),
    NATIVE_DEF(    flenBop,                 "flen" ),
    NATIVE_DEF(    fgetsBop,                "fgets" ),
    NATIVE_DEF(    fputsBop,                "fputs" ),

    // everything below this line does not have an assembler version

    OP_DEF(    stdinOp,                "stdin" ),
    OP_DEF(    stdoutOp,               "stdout" ),
    OP_DEF(    stderrOp,               "stderr" ),
    OP_DEF(    chdirOp,                "chdir" ),
    OP_DEF(    mkdirOp,                "mkdir" ),
    OP_DEF(    rmdirOp,                "rmdir" ),
    OP_DEF(    renameOp,               "rename" ),
	OP_DEF(    opendirOp,              "opendir" ),
	OP_DEF(    readdirOp,              "readdir" ),
	OP_DEF(    closedirOp,             "closedir" ),
	OP_DEF(    rewinddirOp,            "rewinddir" ),
    OP_DEF(    fflushOp,               "fflush" ),
    
    ///////////////////////////////////////////
    //  64-bit integer math & conversions
    ///////////////////////////////////////////
    OP_DEF(    ldivideOp,              "l/" ),
    OP_DEF(    lmodOp,                 "lmod" ),
    OP_DEF(    ldivmodOp,              "l/mod" ),
    OP_DEF(    udivmodOp,              "ud/mod" ),
    OP_DEF(    i2lOp,                  "i2l" ), 
    OP_DEF(    l2fOp,                  "l2f" ), 
    OP_DEF(    l2dOp,                  "l2d" ), 
    OP_DEF(    f2lOp,                  "f2l" ),
    OP_DEF(    d2lOp,                  "d2l" ),
    
    ///////////////////////////////////////////
    //  64-bit integer comparisons
    ///////////////////////////////////////////
    OP_DEF(    lEqualsOp,               "l=" ),
    OP_DEF(    lNotEqualsOp,            "l<>" ),
    OP_DEF(    lGreaterThanOp,          "l>" ),
    OP_DEF(    lGreaterEqualsOp,        "l>=" ),
    OP_DEF(    lLessThanOp,             "l<" ),
    OP_DEF(    lLessEqualsOp,           "l<=" ),
    OP_DEF(    lEqualsZeroOp,           "l0=" ),
    OP_DEF(    lNotEqualsZeroOp,        "l0<>" ),
    OP_DEF(    lGreaterThanZeroOp,      "l0>" ),
    OP_DEF(    lGreaterEqualsZeroOp,    "l0>=" ),
    OP_DEF(    lLessThanZeroOp,         "l0<" ),
    OP_DEF(    lLessEqualsZeroOp,       "l0<=" ),
    OP_DEF(    lWithinOp,               "lwithin" ),
    OP_DEF(    lMinOp,                  "lmin" ),
    OP_DEF(    lMaxOp,                  "lmax" ),

    ///////////////////////////////////////////
    //  control flow
    ///////////////////////////////////////////
    PRECOP_DEF(doOp,                   "do" ),
    PRECOP_DEF(checkDoOp,              "?do" ),
    PRECOP_DEF(loopOp,                 "loop" ),
    PRECOP_DEF(loopNOp,                "+loop" ),
    PRECOP_DEF(ifOp,                   "if" ),
    PRECOP_DEF(elseOp,                 "else" ),
    PRECOP_DEF(endifOp,                "endif" ),
    PRECOP_DEF(beginOp,                "begin" ),
    PRECOP_DEF(untilOp,                "until" ),
    PRECOP_DEF(whileOp,                "while" ),
    PRECOP_DEF(repeatOp,               "repeat" ),
    PRECOP_DEF(againOp,                "again" ),
    PRECOP_DEF(caseOp,                 "case" ),
    PRECOP_DEF(ofOp,                   "of" ),
    PRECOP_DEF(ofifOp,                 "ofif" ),
    PRECOP_DEF(endofOp,                "endof" ),
    PRECOP_DEF(endcaseOp,              "endcase" ),

    ///////////////////////////////////////////
    //  op definition
    ///////////////////////////////////////////
    OP_DEF(    buildsOp,               "builds" ),
    PRECOP_DEF(doesOp,                 "does" ),
    PRECOP_DEF(exitOp,                 "exit" ),
    PRECOP_DEF(semiOp,                 ";" ),
    OP_DEF(    colonOp,                ":" ),
    OP_DEF(    codeOp,                 "code" ),
    OP_DEF(    createOp,               "create" ),
    OP_DEF(    variableOp,             "variable" ),
    OP_DEF(    constantOp,             "constant" ),
    OP_DEF(    dconstantOp,            "dconstant" ),
    PRECOP_DEF(byteOp,                 "byte" ),
    PRECOP_DEF(ubyteOp,                "ubyte" ),
    PRECOP_DEF(shortOp,                "short" ),
    PRECOP_DEF(ushortOp,               "ushort" ),
    PRECOP_DEF(intOp,                  "int" ),
    PRECOP_DEF(uintOp,                 "uint" ),
    PRECOP_DEF(longOp,                 "long" ),
    PRECOP_DEF(ulongOp,                "ulong" ),
    PRECOP_DEF(floatOp,                "float" ),
    PRECOP_DEF(doubleOp,               "double" ),
    PRECOP_DEF(stringOp,               "string" ),
    PRECOP_DEF(opOp,                   "op" ),
    //PRECOP_DEF(objectOp,               "object" ),
    PRECOP_DEF(voidOp,                 "void" ),
    PRECOP_DEF(arrayOfOp,              "arrayOf" ),
    PRECOP_DEF(ptrToOp,                "ptrTo" ),
    OP_DEF(    structOp,               "struct:" ),
    OP_DEF(    endstructOp,            ";struct" ),
    OP_DEF(    classOp,                "class:" ),
    OP_DEF(    endclassOp,             ";class" ),
    OP_DEF(    methodOp,               "method:" ),
    PRECOP_DEF(endmethodOp,            ";method" ),
    PRECOP_DEF(returnsOp,              "returns" ),
    OP_DEF(    doMethodOp,             "doMethod" ),
    OP_DEF(    implementsOp,           "implements:" ),
    OP_DEF(    endimplementsOp,        ";implements" ),
    OP_DEF(    unionOp,                "union" ),
    OP_DEF(    extendsOp,              "extends" ),
    PRECOP_DEF(sizeOfOp,               "sizeOf" ),
    PRECOP_DEF(offsetOfOp,             "offsetOf" ),
    PRECOP_DEF(newOp,                  "new" ),
    PRECOP_DEF(initMemberStringOp,     "initMemberString" ),
    OP_DEF(    enumOp,                 "enum:" ),
    OP_DEF(    endenumOp,              ";enum" ),
    PRECOP_DEF(recursiveOp,            "recursive" ),
    OP_DEF(    precedenceOp,           "precedence" ),
    OP_DEF(    strLoadOp,              "$load" ),
    OP_DEF(    loadOp,                 "load" ),
    OP_DEF(    loadDoneOp,             "loaddone" ),
    OP_DEF(    requiresOp,             "requires" ),
    OP_DEF(    interpretOp,            "interpret" ),
    PRECOP_DEF(stateInterpretOp,       "[" ),
    OP_DEF(    stateCompileOp,         "]" ),
    OP_DEF(    stateOp,                "state" ),
    OP_DEF(    strTickOp,              "$\'" ),
    PRECOP_DEF(compileOp,              "[compile]" ),
    PRECOP_DEF(bracketTickOp,          "[\']" ),

    ///////////////////////////////////////////
    //  vocabulary/symbol
    ///////////////////////////////////////////
    OP_DEF(    forthVocabOp,           "forth" ),
    OP_DEF(    definitionsOp,          "definitions" ),
    OP_DEF(    vocabularyOp,           "vocabulary" ),
    OP_DEF(    alsoOp,                 "also" ),
    OP_DEF(    previousOp,             "previous" ),
    OP_DEF(    onlyOp,                 "only" ),
    OP_DEF(    forgetOp,               "forget" ),
    OP_DEF(    autoforgetOp,           "autoforget" ),
    OP_DEF(    vlistOp,                "vlist" ),
    OP_DEF(    findOp,                 "find" ),

    ///////////////////////////////////////////
    //  data compilation/allocation
    ///////////////////////////////////////////
    OP_DEF(    alignOp,                "align" ),
    OP_DEF(    allotOp,                "allot" ),
    OP_DEF(    commaOp,                "," ),
    OP_DEF(    cCommaOp,               "c," ),
    OP_DEF(    mallocOp,               "malloc" ),
    OP_DEF(    reallocOp,              "realloc" ),
    OP_DEF(    freeOp,                 "free" ),

    ///////////////////////////////////////////
    //  text display
    ///////////////////////////////////////////
    OP_DEF(    printNumOp,             "." ),
    OP_DEF(    printLongNumOp,         "l." ),
    OP_DEF(    printNumDecimalOp,      "%d" ),
    OP_DEF(    printNumHexOp,          "%x" ),
    OP_DEF(    printLongDecimalOp,     "%lld" ),
    OP_DEF(    printLongHexOp,         "%llx" ),
    OP_DEF(    printStrOp,             "%s" ),
    OP_DEF(    printCharOp,            "%c" ),
    OP_DEF(    printBlockOp,           "%block" ),
    OP_DEF(    printSpaceOp,           "%bl" ),
    OP_DEF(    printNewlineOp,         "%nl" ),
    OP_DEF(    printFloatOp,           "%f" ),
    OP_DEF(    printDoubleOp,          "%g" ),
    OP_DEF(    printFormattedOp,       "%fmt" ),
    OP_DEF(    fprintfOp,              "fprintf" ),
    OP_DEF(    sprintfOp,              "sprintf" ),
    OP_DEF(    fscanfOp,               "fscanf" ),
    OP_DEF(    sscanfOp,               "sscanf" ),
    OP_DEF(    baseOp,                 "base" ),
    OP_DEF(    octalOp,                "octal" ),
    OP_DEF(    decimalOp,              "decimal" ),
    OP_DEF(    hexOp,                  "hex" ),
    OP_DEF(    printDecimalSignedOp,   "printDecimalSigned" ),
    OP_DEF(    printAllSignedOp,       "printAllSigned" ),
    OP_DEF(    printAllUnsignedOp,     "printAllUnsigned" ),
    OP_DEF(    outToFileOp,            "outToFile" ),
    OP_DEF(    outToScreenOp,          "outToScreen" ),
    OP_DEF(    outToStringOp,          "outToString" ),
    OP_DEF(    outToOpOp,              "outToOp" ),
    OP_DEF(    getConOutFileOp,        "getConOutFile" ),

    ///////////////////////////////////////////
    //  input buffer
    ///////////////////////////////////////////
    OP_DEF(    blwordOp,               "blword" ),
    OP_DEF(    wordOp,                 "word" ),
    PRECOP_DEF(commentOp,              "/*" ),
    PRECOP_DEF(parenCommentOp,         "(" ),
    OP_DEF(    parenIsCommentOp,       "parenIsComment" ),
    OP_DEF(    getInBufferBaseOp,      "getInBufferBase" ),
    OP_DEF(    getInBufferPointerOp,   "getInBufferPointer" ),
    OP_DEF(    setInBufferPointerOp,   "setInBufferPointer" ),
    OP_DEF(    getInBufferLengthOp,    "getInBufferLength" ),
    OP_DEF(    fillInBufferOp,         "fillInBuffer" ),
    OP_DEF(    keyOp,                  "key" ),
    OP_DEF(    keyHitOp,               "key?" ),

    ///////////////////////////////////////////
    //  vocabulary ops
    ///////////////////////////////////////////
    OP_DEF(    vocNewestEntryOp,       "vocNewestEntry" ),
    OP_DEF(    vocNextEntryOp,         "vocNextEntry" ),
    OP_DEF(    vocNumEntriesOp,        "vocNumEntries" ),
    OP_DEF(    vocNameOp,              "vocName" ),
    OP_DEF(    vocChainHeadOp,         "vocChainHead" ),
    OP_DEF(    vocChainNextOp,         "vocChainNext" ),
    OP_DEF(    vocFindEntryOp,         "vocFindEntry" ),
    OP_DEF(    vocFindNextEntryOp,     "vocFindNextEntry" ),
    OP_DEF(    vocFindEntryByValueOp,  "vocFindEntryByValue" ),
    OP_DEF(    vocFindNextEntryByValueOp,  "vocFindNextEntryByValue" ),
    OP_DEF(    vocAddEntryOp,          "vocAddEntry" ),
    OP_DEF(    vocRemoveEntryOp,       "vocRemoveEntry" ),
    OP_DEF(    vocValueLengthOp,       "vocValueLength" ),

    ///////////////////////////////////////////
    //  DLL support
    ///////////////////////////////////////////
#ifdef WIN32
    OP_DEF(    DLLVocabularyOp,        "DLLVocabulary" ),
    OP_DEF(    addDLLEntryOp,          "addDLLEntry" ),
    OP_DEF(    DLLVoidOp,              "DLLVoid" ),
    OP_DEF(    DLLLongOp,              "DLLLong" ),
    OP_DEF(    DLLStdCallOp,           "DLLStdCall" ),
#endif

    ///////////////////////////////////////////
    //  time and date
    ///////////////////////////////////////////
    OP_DEF(    timeOp,                 "time" ),
    OP_DEF(    strftimeOp,             "strftime" ),
    OP_DEF(    millitimeOp,            "millitime" ),
    OP_DEF(    millisleepOp,           "millisleep" ),

    ///////////////////////////////////////////
    //  computation utilities - randoms, hashing, sorting and searcing
    ///////////////////////////////////////////
    OP_DEF(    randOp,                 "rand" ),
    OP_DEF(    srandOp,                "srand" ),
    OP_DEF(    hashOp,                 "hash" ),
    OP_DEF(    qsortOp,                "qsort" ),
    OP_DEF(    bsearchOp,              "bsearch" ),

    ///////////////////////////////////////////
    //  admin/debug/system
    ///////////////////////////////////////////
    OP_DEF(    dstackOp,               "dstack" ),
    OP_DEF(    drstackOp,              "drstack" ),
    OP_DEF(    removeOp,               "remove" ),
    OP_DEF(    _dupOp,                 "_dup" ),
    OP_DEF(    _dup2Op,                "_dup2" ),
    OP_DEF(    _filenoOp,              "_fileno" ),
    OP_DEF(    tmpnamOp,               "tmpnam" ),
    OP_DEF(    systemOp,               "system" ),
    OP_DEF(    byeOp,                  "bye" ),
    OP_DEF(    argvOp,                 "argv" ),
    OP_DEF(    argcOp,                 "argc" ),
    OP_DEF(    turboOp,                "turbo" ),
    OP_DEF(    statsOp,                "stats" ),
    OP_DEF(    describeOp,             "describe" ),
    OP_DEF(    errorOp,                "error" ),
    OP_DEF(    addErrorTextOp,         "addErrorText" ),
    OP_DEF(    unimplementedMethodOp,  "unimplementedMethod" ),
	OP_DEF(    setTraceOp,             "setTrace" ),
    NATIVE_DEF( intoBop,               "verbose" ),

    ///////////////////////////////////////////
    //  conditional compilation
    ///////////////////////////////////////////
    PRECOP_DEF( poundIfOp,              "#if" ),
    PRECOP_DEF( poundIfdefOp,           "#ifdef" ),
    PRECOP_DEF( poundIfndefOp,          "#ifndef" ),
    PRECOP_DEF( poundElseOp,            "#else" ),
    PRECOP_DEF( poundEndifOp,           "#endif" ),

    ///////////////////////////////////////////
    //  network/client/server
    ///////////////////////////////////////////
	OP_DEF(		serverOp,				"server" ),
	OP_DEF(		clientOp,				"client" ),
	OP_DEF(		shutdownOp,				"shutdown" ),

    ///////////////////////////////////////////
    //  threads
    ///////////////////////////////////////////
    OP_DEF( createThreadOp,             "createThread" ),
    OP_DEF( destroyThreadOp,            "destroyThread" ),
    OP_DEF( threadGetStateOp,           "threadGetState" ),
    OP_DEF( stepThreadOp,               "stepThread" ),
    OP_DEF( startThreadOp,              "startThread" ),
    OP_DEF( exitThreadOp,               "exitThread" ),

#ifdef WIN32
    ///////////////////////////////////////////
    //  Windows support
    ///////////////////////////////////////////
    OP_DEF( createEventOp,              "CreateEvent" ),
    OP_DEF( closeHandleOp,              "CloseHandle" ),
    OP_DEF( setEventOp,                 "SetEvent" ),
    OP_DEF( resetEventOp,               "ResetEvent" ),
    OP_DEF( pulseEventOp,               "PulseEvent" ),
    OP_DEF( getLastErrorOp,             "GetLastError" ),
    OP_DEF( waitForSingleObjectOp,      "WaitForSingleObject" ),
    OP_DEF( waitForMultipleObjectsOp,   "WaitForMultipleObjects" ),
    OP_DEF( initializeCriticalSectionOp, "InitializeCriticalSection" ),
    OP_DEF( deleteCriticalSectionOp,    "DeleteCriticalSection" ),
    OP_DEF( enterCriticalSectionOp,     "EnterCriticalSection" ),
    OP_DEF( leaveCriticalSectionOp,     "LeaveCriticalSection" ),
    OP_DEF( findFirstFileOp,            "FindFirstFile" ),
    OP_DEF( findNextFileOp,             "FindNextFile" ),
    OP_DEF( findCloseOp,                "FindClose" ),
    OP_DEF( windowsConstantsOp,         "windowsConstants" ),

	NATIVE_DEF( trueBop,				"WINDOWS" ),
#elif defined(LINUX)
	NATIVE_DEF( trueBop,				"LINUX" ),
#endif

    // following must be last in table
    OP_DEF(    NULL,                   NULL )
};


//############################################################################
//
//  The end of all things...
//
//############################################################################

void audioCallback(void *userdata, unsigned char *stream, int len)
{
    for ( int i = 0; i < len; i++ )
    {
        *stream++ = (i & 1) ? 127 : -128;
    }
}

void AddForthOps( ForthEngine* pEngine )
{
	baseDictionaryCompiledEntry* pCompEntry = baseCompiledDictionary;

	while ( pCompEntry->name != NULL )
	{
		long* pEntry = pEngine->AddBuiltinOp( pCompEntry->name, pCompEntry->flags, pCompEntry->value );
		gCompiledOps[ pCompEntry->index ] = *pEntry;
		pCompEntry++;
	}

	baseDictionaryEntry* pDictEntry = baseDictionary;

	while ( pDictEntry->name != NULL )
	{
		long* pEntry = pEngine->AddBuiltinOp( pDictEntry->name, pDictEntry->flags, pDictEntry->value );
		pDictEntry++;
	}
}

};  // end extern "C"


