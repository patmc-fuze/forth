//////////////////////////////////////////////////////////////////////
//
// ForthOps.cpp: forth builtin operator definitions
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <sys\timeb.h>
#include "Forth.h"
#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthVocabulary.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include "ForthStructs.h"

extern "C" {

// compiled token is 32-bits,
// top 8 bits are opcode type (opType)
// bottom 24 bits are immediate operand

//############################################################################
//
//   normal forth ops
//
//############################################################################


// done is used by the outer interpreter to make the inner interpreter
//  exit after interpreting a single opcode
FORTHOP( doneOp )
{
    SET_STATE( kResultDone );
}

// bye is a user command which causes the entire forth program to exit
FORTHOP( byeOp )
{
    SET_STATE( kResultExitShell );
}

// abort is a user command which causes the entire forth engine to exit,
//   and indicates that a fatal error has occured
FORTHOP( abortOp )
{
    SET_FATAL_ERROR( kForthErrorAbort );
}

// abort is a user command which causes the entire forth engine to exit,
//   and indicates that a fatal error has occured
FORTHOP( badOpOp )
{
    SET_ERROR( kForthErrorBadOpcode );
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

FORTHOP( plusOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a + b );
}

FORTHOP( minusOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a - b );
}

FORTHOP( timesOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a * b );
}

FORTHOP( times2Op )
{
    NEEDS(1);
    SPUSH( SPOP << 1 );
}

FORTHOP( times4Op )
{
    NEEDS(1);
    SPUSH( SPOP << 2 );
}

FORTHOP( divideOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a / b );
}

FORTHOP( divide2Op )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a >> 1 );
}

FORTHOP( divide4Op )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a >>2 );
}

FORTHOP( divmodOp )
{
    NEEDS(2);
    div_t v;
    long b = SPOP;
    long a = SPOP;
    v = div( a, b );
    SPUSH( v.rem );
    SPUSH( v.quot );
}

FORTHOP( modOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a % b );
}

FORTHOP( negateOp )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( -a );
}

FORTHOP( fplusOp )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( a + b );
}

FORTHOP( fminusOp )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( a - b );
}

FORTHOP( ftimesOp )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( a * b );
}

FORTHOP( fdivideOp )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( a / b );
}


FORTHOP( dplusOp )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a + b );
}

FORTHOP( dminusOp )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a - b );
}

FORTHOP( dtimesOp )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a * b );
}

FORTHOP( ddivideOp )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a / b );
}


FORTHOP( dsinOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( sin(a) );
}

FORTHOP( dasinOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( asin(a) );
}

FORTHOP( dcosOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( cos(a) );
}

FORTHOP( dacosOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( acos(a) );
}

FORTHOP( dtanOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( tan(a) );
}

FORTHOP( datanOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( atan(a) );
}

FORTHOP( datan2Op )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( atan2(a, b) );
}

FORTHOP( dexpOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( exp(a) );
}

FORTHOP( dlnOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( log(a) );
}

FORTHOP( dlog10Op )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( log10(a) );
}

FORTHOP( dpowOp )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( pow(a, b) );
}

FORTHOP( dsqrtOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( sqrt(a) );
}

FORTHOP( dceilOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( ceil(a) );
}

FORTHOP( dfloorOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( floor(a) );
}

FORTHOP( dabsOp )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( fabs(a) );
}

FORTHOP( dldexpOp )
{
    NEEDS(3);
    int n = SPOP;
    double a = DPOP;
    DPUSH( ldexp(a, n) );
}

FORTHOP( dfrexpOp )
{
    NEEDS(2);
    int n;
    double a = DPOP;
    DPUSH( frexp( a, &n ) );
    SPUSH( n );
}

FORTHOP( dmodfOp )
{
    NEEDS(2);
    double b;
    double a = DPOP;
    a = modf( a, &b );
    DPUSH( b );
    DPUSH( a );
}

FORTHOP( dfmodOp )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( fmod( a, b ) );
}

FORTHOP( i2fOp )
{
    NEEDS(1);
    float a = (float) SPOP;
    FPUSH( a );
}

FORTHOP( i2dOp )
{
    NEEDS(1);
    double a = SPOP;
    DPUSH( a );
}
FORTHOP( f2iOp )
{
    NEEDS(1);
    int a = (int) FPOP;
    SPUSH( a );
}

FORTHOP( f2dOp )
{
    NEEDS(1);
    double a = FPOP;
    DPUSH( a );
}

FORTHOP( d2iOp )
{
    NEEDS(2);
    int a = (int) DPOP;
    SPUSH( a );
}


FORTHOP( d2fOp )
{
    NEEDS(2);
    float a = (float) DPOP;
    FPUSH( a );
}


//##############################
//
// control ops
//
// TBD: replace branch, tbranch, fbranch with immediate ops

// exit normal op with no local vars
FORTHOP(doExitOp)
{
    // rstack: oldIP
    if ( GET_RDEPTH < 1 ) {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    } else {
        SET_IP( (long *) RPOP );
    }
}

// exit normal op with local vars
FORTHOP(doExitLOp)
{
    // rstack: local_var_storage oldFP oldIP
    // FP points to oldFP
    SET_RP( GET_FP );
    SET_FP( (long *) (RPOP) );
    if ( GET_RDEPTH < 1 ) {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    } else {
        SET_IP( (long *) RPOP );
    }
}

// exit method op with no local vars
FORTHOP(doExitMOp)
{
    // rstack: oldIP oldTP
    if ( GET_RDEPTH < 3 ) {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    } else {
        SET_IP( (long *) RPOP );
        SET_TPM( (long *) RPOP );
        SET_TPD( (long *) RPOP );
    }
}

// exit method op with local vars
FORTHOP(doExitMLOp)
{
    // rstack: local_var_storage oldFP oldIP oldTP
    // FP points to oldFP
    SET_RP( GET_FP );
    SET_FP( (long *) (RPOP) );
    if ( GET_RDEPTH < 3 ) {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    } else {
        SET_IP( (long *) RPOP );
        SET_TPM( (long *) RPOP );
        SET_TPD( (long *) RPOP );
    }
}

FORTHOP(callOp)
{
    RPUSH( (long) GET_IP );
    SET_IP( (long *) SPOP );
}

FORTHOP(gotoOp)
{
    SET_IP( (long *) SPOP );
}

// has precedence!
FORTHOP(doOp)
{
    NEEDS(2);
    ForthEngine *pEngine = GET_ENGINE;
    ForthShellStack *pShellStack = pEngine->GetShell()->GetShellStack();
    // save address for loop/+loop
    pShellStack->Push( (long)GET_DP );
    pShellStack->Push( kShellTagDo );
    // this will be fixed by loop/+loop
    pEngine->CompileOpcode( OP_ABORT );
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
    *pDoOp++ = OP_DO_DO;
    // compile the "_loop" opcode
    pEngine->CompileOpcode( OP_DO_LOOP );
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
    *pDoOp++ = OP_DO_DO;
    // compile the "_loop" opcode
    pEngine->CompileOpcode( OP_DO_LOOPN );
    // fill in the branch to after loop opcode
    *pDoOp = COMPILED_OP( kOpBranch, (GET_DP - pDoOp) - 1 );
}

FORTHOP(doDoOp)
{
    NEEDS(2);
    long startIndex = SPOP;
    // skip over loop exit IP right after this op
    long *newIP = (GET_IP) + 1;
    SET_IP( newIP );

    RPUSH( (long) newIP );
    RPUSH( SPOP );
    RPUSH( startIndex );
    // top of rstack is current index, next is end index,
    //  next is looptop IP
}

FORTHOP(doLoopOp)
{
    RNEEDS(3);

    long *pRP = GET_RP;
    long newIndex = (*pRP) + 1;

    if ( newIndex >= pRP[1] ) {
        // loop has ended, drop end, current indices, loopIP
        SET_RP( pRP + 3 );
    } else {
        // loop again
        *pRP = newIndex;
        SET_IP( (long *) (pRP[2]) );
    }
}

FORTHOP(doLoopNOp)
{
    RNEEDS(3);
    NEEDS(1);

    long *pRP = GET_RP;
    long increment = SPOP;
    long newIndex = (*pRP) + increment;
    bool done;

    done = (increment > 0) ? (newIndex >= pRP[1]) : (newIndex < pRP[1]);
    if ( done ) {
        // loop has ended, drop end, current indices, loopIP
        SET_RP( pRP + 3 );
    } else {
        // loop again
        *pRP = newIndex;
        SET_IP( (long *) (pRP[2]) );
    }
}

FORTHOP(iOp)
{
    SPUSH( *(GET_RP) );
}

FORTHOP(jOp)
{
    SPUSH( GET_RP[3] );
}


// discard stuff "do" put on rstack in preparation for an "exit"
//   of the current op from inside a do loop
FORTHOP( unloopOp )
{
    RNEEDS(3);
    // loop has ended, drop end, current indices, loopIP
    SET_RP( GET_RP + 3 );
}


// exit loop immediately
FORTHOP( leaveOp )
{
    RNEEDS(3);
    long *pRP = GET_RP;
    long *newIP = (long *) (pRP[2]) - 1;

    // loop has ended, drop end, current indices, loopIP
    SET_RP( pRP + 3 );
    // point IP back to the branch op just after the _do opcode
    SET_IP( newIP );
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
    pEngine->CompileOpcode( OP_ABORT );
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
    pEngine->CompileOpcode( OP_ABORT );
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
    if ( tag == kShellTagElse ) {
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
    pEngine->CompileOpcode( OP_ABORT );
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
    pShellStack->Push( kShellTagCase );
    // this will be set to a caseBranch by endof
    pEngine->CompileOpcode( OP_ABORT );
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
    pEngine->CompileOpcode( OP_ABORT );

    if ( !pShell->CheckSyntaxError( "endof", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }
    long *pOfOp = (long *) pShellStack->Pop();
    // fill in the branch taken when case doesn't match
    *pOfOp = COMPILED_OP( kOpCaseBranch, (GET_DP - pOfOp) - 1 );

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

    if ( ((GET_DP) - (long *)(pShellStack->Peek())) == 1 ) {
        // there is no default case, we must compile a "drop" to
        //   dispose of the case selector on TOS
        pEngine->CompileOpcode( OP_DROP );
    }
    // patch branches from end-of-case to common exit point
    while ( (pEndofOp = (long *) (pShellStack->Pop())) != NULL ) {
        *pEndofOp = COMPILED_OP( kOpBranch, (GET_DP - pEndofOp) - 1 );
    }
    SET_SP( pSP );
}

///////////////////////////////////////////
//  bit-vector logic ops
///////////////////////////////////////////
FORTHOP( orOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a | b );
}

FORTHOP( andOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a & b );
}

FORTHOP( xorOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a ^ b );
}

FORTHOP( invertOp )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ~a );
}

FORTHOP( lshiftOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a << b );
}

FORTHOP( rshiftOp )
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a >> b );
}


///////////////////////////////////////////
//  boolean ops
///////////////////////////////////////////
FORTHOP( notOp )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ~a );
}

FORTHOP( trueOp )
{
    SPUSH( ~0 );
}

FORTHOP( falseOp )
{
    SPUSH( 0 );
}

FORTHOP( nullOp )
{
    SPUSH( 0 );
}

FORTHOP( dnullOp )
{
    SPUSH( 0 );
    SPUSH( 0 );
}

//##############################
//
// comparison ops
//

FORTHOP(equalsOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    if ( a == b ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(notEqualsOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    if ( a != b ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(greaterThanOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    if ( a > b ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(greaterEqualsOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    if ( a >= b ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(lessThanOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    if ( a < b ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(lessEqualsOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    if ( a <= b ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(equalsZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    if ( a == 0 ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(notEqualsZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    if ( a != 0 ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(greaterThanZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    if ( a > 0 ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(greaterEqualsZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    if ( a >= 0 ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(lessThanZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    if ( a < 0 ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(lessEqualsZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    if ( a <= 0 ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(unsignedGreaterThanOp)
{
    NEEDS(2);
    ulong b = (ulong) SPOP;
    ulong a = (ulong) SPOP;
    if ( a > b ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}

FORTHOP(unsignedLessThanOp)
{
    NEEDS(2);
    ulong b = (ulong) SPOP;
    ulong a = (ulong) SPOP;
    if ( a < b ) {
        SPUSH( -1L );
    } else {
        SPUSH( 0 );
    }
}


//##############################
//
// stack manipulation
//

FORTHOP(rpushOp)
{
    NEEDS(1);
    RPUSH( SPOP );
}

FORTHOP(rpopOp)
{
    RNEEDS(1);
    SPUSH( RPOP );
}

FORTHOP(rdropOp)
{
    RNEEDS(1);
    RPOP;
}

FORTHOP(rpOp)
{
    long pRP = (long) (GET_RP);
    SPUSH( pRP );
}

FORTHOP(rzeroOp)
{
    long pR0 = (long) (pCore->RT);
    SPUSH( pR0 );
}

FORTHOP(dupOp)
{
    NEEDS(1);
    long a = *(GET_SP);
    SPUSH( a );
}

FORTHOP(swapOp)
{
    NEEDS(2);
    long a = *(GET_SP);
    *(GET_SP) = (GET_SP)[1];
    (GET_SP)[1] = a;
}

FORTHOP(dropOp)
{
    NEEDS(1);
    SPOP;
}

FORTHOP(overOp)
{
    NEEDS(1);
    long a = (GET_SP)[1];
    SPUSH( a );
}

FORTHOP(rotOp)
{
    NEEDS(3);
    int a, b, c;
    long *pSP = GET_SP;
    a = pSP[2];
    b = pSP[1];
    c = pSP[0];
    pSP[2] = b;
    pSP[1] = c;
    pSP[0] = a;
}

FORTHOP(tuckOp)
{
    NEEDS(2);
    long *pSP = GET_SP;
    long a = *pSP;
    long b = pSP[1];
    SPUSH( a );
    *pSP = b;
    pSP[1] = a;
}

FORTHOP(pickOp)
{
    NEEDS(1);
    long *pSP = GET_SP;
    long n = *pSP;
    long a = pSP[n + 1];
    *pSP = a;
}

FORTHOP(rollOp)
{
    // TBD: moves the Nth element to TOS
    // 1 roll is the same as swap
    // 2 roll is the same as rot
    long n = (SPOP);
    long *pSP = GET_SP;
    long a = pSP[n];
    for ( int i = n; i != 0; i-- )
    {
        pSP[i] = pSP[i - 1];
    }
    *pSP = a;
}

FORTHOP(spOp)
{
    long pSP = (long) (GET_SP);
    SPUSH( pSP );
}

FORTHOP(szeroOp)
{
    long pS0 = (long) (pCore->ST);
    SPUSH( pS0 );
}

FORTHOP(fpOp)
{
    long pFP = (long) (GET_FP);
    SPUSH( pFP );
}

FORTHOP(ddupOp)
{
    NEEDS(2);
    double *pDsp = (double *)(GET_SP);
    DPUSH( *pDsp );
}

FORTHOP(dswapOp)
{
    NEEDS(4);
    double *pDsp = (double *)(GET_SP);
    double a = *pDsp;
    *pDsp = pDsp[1];
    pDsp[1] = a;
}

FORTHOP(ddropOp)
{
    NEEDS(2);
    SET_SP( GET_SP + 2 );
}

FORTHOP(doverOp)
{
    NEEDS(2);
    double *pDsp = (double *)(GET_SP);
    DPUSH( pDsp[1] );
}

FORTHOP(drotOp)
{
    NEEDS(3);
    double *pDsp = (double *)(GET_SP);
    double a, b, c;
    a = pDsp[2];
    b = pDsp[1];
    c = pDsp[0];
    pDsp[2] = b;
    pDsp[1] = c;
    pDsp[0] = a;
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

FORTHOP( callotOp )
{
    char *pNewDP = ((char *)GET_DP) + SPOP;
    // NOTE: this could leave DP not longword aligned
    SET_DP( (long *) pNewDP );
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

FORTHOP(hereOp)
{
    SPUSH( (long) GET_DP );
}

FORTHOP( mallocOp )
{
    NEEDS(1);
    SPUSH(  (long) malloc( SPOP )  );
}

FORTHOP( freeOp )
{
    NEEDS(1);
    free( (void *) SPOP );
}

//##############################
//
// loads & stores
//

FORTHOP(storeOp)
{
    NEEDS(2);
    long *pB = (long *)(SPOP); 
    long a = SPOP;
    *pB = a;
}

FORTHOP(fetchOp)
{
    NEEDS(1);
    long *pA = (long *)(SPOP); 
    SPUSH( *pA );
}

FORTHOP(cstoreOp)
{
    NEEDS(2);
    char *pB = (char *) (SPOP);
    long a = SPOP;
    *pB = (char) a;
}

FORTHOP(cfetchOp)
{
    NEEDS(1);
    unsigned char *pA = (unsigned char *) (SPOP);
    SPUSH( (*pA) );
}

// signed char fetch
FORTHOP(scfetchOp)
{
    NEEDS(1);
    signed char *pA = (signed char *) (SPOP);
    SPUSH( (*pA) );
}

FORTHOP(c2lOp)
{
    NEEDS(1);
    signed char a = (signed char) (SPOP);
	long b = (long) a;
    SPUSH( b );
}

FORTHOP(wstoreOp)
{
    NEEDS(2);
    short *pB = (short *) (SPOP);
    long a = SPOP;
    *pB = (short) a;
}

FORTHOP(wfetchOp)
{
    NEEDS(1);
    unsigned short *pA = (unsigned short *) (SPOP);
    SPUSH( *pA );
}

// signed word fetch
FORTHOP(swfetchOp)
{
    NEEDS(1);
    signed short *pA = (signed short *) (SPOP);
    SPUSH( *pA );
}

FORTHOP(w2lOp)
{
    NEEDS(1);
    short a = (short) (SPOP);
	long b = (long) a;
    SPUSH( b );
}

FORTHOP(dstoreOp)
{
    NEEDS(3);
    double *pB = (double *) (SPOP); 
    double a= DPOP;
    *pB = a;
}

FORTHOP(dfetchOp)
{
    NEEDS(1);
    double *pA = (double *) (SPOP);
    DPUSH( *pA );
}

FORTHOP(memcpyOp)
{
    NEEDS(3);
    size_t nBytes = (size_t) (SPOP);
    const void* pSrc = (const void *) (SPOP);
    void* pDst = (void *) (SPOP);
    memcpy( pDst, pSrc, nBytes );
}

FORTHOP(memsetOp)
{
    NEEDS(3);
    size_t nBytes = (size_t) (SPOP);
    int byteVal = (int) (SPOP);
    void* pDst = (void *) (SPOP);
    memset( pDst, byteVal, nBytes );
}

FORTHOP(intoOp)
{
    SET_VAR_OPERATION( kVarStore );
}

FORTHOP(addToOp)
{
    SET_VAR_OPERATION( kVarPlusStore );
}

FORTHOP(subtractFromOp)
{
    SET_VAR_OPERATION( kVarMinusStore );
}

FORTHOP(addressOfOp)
{
    SET_VAR_OPERATION( kVarRef );
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

FORTHOP(setVarActionOp)
{
    SET_VAR_OPERATION( SPOP );
}

FORTHOP(getVarActionOp)
{
    SPUSH( GET_VAR_OPERATION );
}

//##############################
//
// string ops
//

FORTHOP( strcpyOp )
{
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strcpy( pDst, pSrc );
}

FORTHOP( strncpyOp )
{
	size_t maxBytes = (size_t) SPOP;
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strncpy( pDst, pSrc, maxBytes );
}

FORTHOP( strlenOp )
{
    char *pSrc = (char *) SPOP;
    SPUSH(  strlen( pSrc ) );
}

FORTHOP( strcatOp )
{
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strcat( pDst, pSrc );
}

FORTHOP( strncatOp )
{
	size_t maxBytes = (size_t) SPOP;
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strncat( pDst, pSrc, maxBytes );
}


FORTHOP( strchrOp )
{
    int c = (int) SPOP;
    char *pStr = (char *) SPOP;
    SPUSH( (long) strchr( pStr, c ) );
}

FORTHOP( strrchrOp )
{
    int c = (int) SPOP;
    char *pStr = (char *) SPOP;
    SPUSH( (long) strrchr( pStr, c ) );
}

FORTHOP( strcmpOp )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
    SPUSH( (long) strcmp( pStr1, pStr2 ) );
}

FORTHOP( stricmpOp )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
    SPUSH( (long) stricmp( pStr1, pStr2 ) );
}


FORTHOP( strstrOp )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
    SPUSH( (long) strstr( pStr1, pStr2 ) );
}


FORTHOP( strtokOp )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
    SPUSH( (long) strtok( pStr1, pStr2 ) );
}

FORTHOP( initStringOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long len;
    long* pStr;

    // TOS: ptr to first char, maximum length
    pStr = (long *) (SPOP);
    len = SPOP;
    pStr[-2] = len;
    pStr[-1] = 0;
    *((char *) pStr) = 0;
}


FORTHOP( initStringArrayOp )
{
    // TOS: ptr to first char of first element, maximum length, number of elements
    ForthEngine *pEngine = GET_ENGINE;
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


// push the immediately following literal 32-bit constant
FORTHOP(litOp)
{
    long *pV = GET_IP;
    SET_IP( pV + 1 );
    SPUSH( *pV );
}

// push the immediately following literal 64-bit constant
FORTHOP(dlitOp)
{
    double *pV = (double *) GET_IP;
    DPUSH( *pV++ );
    SET_IP( (long *) pV );
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

    // get next symbol, add it to vocabulary with type "builds/does"
    pEngine->AlignDP();
    pEngine->AddUserOp( pEngine->GetNextSimpleToken(), true );
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
    pEngine->CompileOpcode( OP_END_BUILDS );
    pEngine->CompileLong( 0 );
    // create a nameless vocabulary entry for does-body opcode
    newUserOp = pEngine->AddOp( GET_DP, kOpUserDef );
    newUserOp = COMPILED_OP( kOpUserDef, newUserOp );
    pEngine->CompileOpcode( OP_DO_DOES );
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

// doDoes
// - does not have precedence
// - is executed while executing the user word
// - it gets the parameter address of the user word
// 
FORTHOP( doDoesOp )
{
    SPUSH( RPOP );
}

// exit has precedence
FORTHOP( exitOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // compile exitOp
    long flags = pEngine->GetFlags();

    switch ( flags & (kEngineFlagHasLocalVars | kEngineFlagIsMethod) ) {
    case 0:
        // normal definition, no local vars, not a method
        pEngine->CompileOpcode( OP_DO_EXIT );
        break;
    case kEngineFlagHasLocalVars:
        // normal definition with local vars
        pEngine->CompileOpcode( OP_DO_EXIT_L );
        break;
    case kEngineFlagIsMethod:
        // method definition, no local vars
        pEngine->CompileOpcode( OP_DO_EXIT_M );
        break;
    case (kEngineFlagHasLocalVars | kEngineFlagIsMethod):
        // method definition, with local vars
        pEngine->CompileOpcode( OP_DO_EXIT_ML );
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
    pEngine->StartOpDefinition( NULL, true );
    // switch to compile mode
    pEngine->SetCompileState( 1 );
    pEngine->ClearFlag( kEngineFlagHasLocalVars );
}

FORTHOP( codeOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    ForthVocabulary* pVocab = pEngine->GetDefinitionVocabulary();
    pEngine->StartOpDefinition( NULL, false );
    long* pEntry = pVocab->GetNewestEntry();
    long newestOp = *pEntry;
    *pEntry = COMPILED_OP( kOpUserCode, FORTH_OP_VALUE( newestOp ) );
}

FORTHOP( createOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition( NULL, false );
    pEngine->CompileOpcode( OP_DO_VAR );
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
    pEngine->StartOpDefinition();
    pEngine->CompileOpcode( OP_DO_VOCAB );
    ForthVocabulary* pVocab = new ForthVocabulary( pDefinitionsVocab->GetEntryName( pDefinitionsVocab->GetNewestEntry() ),
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
    pEngine->ForgetSymbol( pSym );
    // reset search & definitions vocabs in case we deleted a vocab we were using
    pEngine->SetDefinitionVocabulary( pEngine->GetForthVocabulary() );
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pVocabStack->Clear();
}

// just like forget, but no error message if symbol not found
FORTHOP( autoforgetOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->ForgetSymbol( pEngine->GetNextSimpleToken() );
    // reset search & definitions vocabs in case we deleted a vocab we were using
    pEngine->SetDefinitionVocabulary( pEngine->GetForthVocabulary() );
    ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
    pVocabStack->Clear();
}

// return 'q' IFF user quit out
static char
ShowVocab( ForthCoreState   *pCore,
           ForthVocabulary  *pVocab )
{
    int i;
    char retVal = 0;
    ForthShell *pShell = GET_ENGINE->GetShell();
    int nEntries = pVocab->GetNumEntries();
    long *pEntry = pVocab->GetFirstEntry();

    for ( i = 0; i < nEntries; i++ )
    {
        pVocab->PrintEntry( pEntry );
        CONSOLE_STRING_OUT( "\n" );
        pEntry = pVocab->NextEntry( pEntry );
        if ( ((i % 22) == 21) || (i == (nEntries-1)) )
        {
            if ( (pShell != NULL) && pShell->GetInput()->InputStream()->IsInteractive() )
            {
                CONSOLE_STRING_OUT( "\nHit ENTER to continue, 'q' & ENTER to quit, 'n' & ENTER to do next vocabulary\n" );
                retVal = tolower( getchar() );
                if ( retVal == 'q' )
                {
                    getchar();
                    break;
                }
                else if ( retVal == 'n' )
                {
                    getchar();
                    break;
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
        quit = ( ShowVocab( pCore, pVocab ) == 'q' );
        depth++;
    }
}

FORTHOP( variableOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition();
    pEngine->CompileOpcode( OP_DO_VAR );
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
    pEngine->StartOpDefinition();
    pEngine->CompileOpcode( OP_DO_CONSTANT );
    pEngine->CompileLong( SPOP );
}

FORTHOP( doConstantOp )
{
    // IP points to data field
    SPUSH( *GET_IP );
    SET_IP( (long *) (RPOP) );
}

FORTHOP( dconstantOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->StartOpDefinition();
    pEngine->CompileOpcode( OP_DO_DCONSTANT );
    double d = DPOP;
    pEngine->CompileDouble( d );
}

FORTHOP( doDConstantOp )
{
    // IP points to data field
    DPUSH( *((double *) (GET_IP)) );
    SET_IP( (long *) (RPOP) );
}

FORTHOP( byteOp )
{
    char val = 0;
	gBaseTypeByte.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( shortOp )
{
    short val = 0;
	gBaseTypeShort.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( intOp )
{
    int val = 0;
	gBaseTypeInt.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( floatOp )
{
    float val = 0.0;
	gBaseTypeFloat.DefineInstance( GET_ENGINE, &val );
}


FORTHOP( doubleOp )
{
    double val = 0.0;
	gBaseTypeDouble.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( stringOp )
{
    gBaseTypeString.DefineInstance( GET_ENGINE, NULL );
}

FORTHOP( opOp )
{
    int val = OP_BAD_OP;
	gBaseTypeOp.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( objectOp )
{
    long val[2] = { 0, 0  };
	gBaseTypeObject.DefineInstance( GET_ENGINE, val );
}

FORTHOP( voidOp )
{
}

FORTHOP( arrayOfOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    long numElements;

    if ( pEngine->IsCompiling() )
    {
        // the symbol just before "string" should have been an integer constant
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
    pEngine->CompileOpcode( OP_DO_STRUCT_TYPE );
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
    pEngine->SetFlag( kEngineFlagInStructDefinition );
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthClassVocabulary* pVocab = pManager->StartClassDefinition( pEngine->GetNextSimpleToken() );
    pEngine->CompileOpcode( OP_DO_CLASS_TYPE );
    pEngine->CompileLong( (long) pVocab );
}

FORTHOP( endclassOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->ClearFlag( kEngineFlagInStructDefinition );
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    pManager->EndClassDefinition();
}

FORTHOP( methodOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    const char* pMethodName = pEngine->GetNextSimpleToken();
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    pEngine->StartOpDefinition( pMethodName, true );
    // switch to compile mode
    pEngine->SetCompileState( 1 );
    pEngine->ClearFlag( kEngineFlagHasLocalVars );
    pEngine->SetFlag( kEngineFlagIsMethod );
    ForthClassVocabulary* pVocab = pManager->GetNewestClass();
    if ( pVocab )
    {
        long* pEntry = pVocab->GetNewestEntry();
        if ( pEntry )
        {
            long methodIndex = pVocab->AddMethod( pMethodName, pEntry[0] );
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
            pManager->GetNewestStruct()->Extends( pParentVocab );
        }
        else
        {
            pEngine->AddErrorText( pSym );
            pEngine->SetError( kForthErrorUnknownSymbol, " is not a structure" );
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
            pEngine->AddErrorText( pSym );
            pEngine->SetError( kForthErrorUnknownSymbol, " is not a structure" );
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

FORTHOP( thisOp )
{
    SPUSH( ((long) GET_TPD) );
    SPUSH( ((long) GET_TPM) );
}

FORTHOP( thisDataOp )
{
    SPUSH( ((long) GET_TPD) );
}

FORTHOP( thisMethodsOp )
{
    SPUSH( ((long) GET_TPM) );
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
                pEngine->CompileOpcode( OP_DO_NEW );
                pEngine->CompileLong( (long) pClassVocab );
            }
            else
            {
                ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
                if ( pPrimaryInterface )
                {
                    long nBytes = pClassVocab->GetSize();
                    void* pData = malloc( nBytes );
                    SPUSH( (long) pData );
                    SPUSH( (long) (pPrimaryInterface->GetMethods()) );
                }
                else
                {
                    pEngine->AddErrorText( pSym );
                    pEngine->SetError( kForthErrorBadParameter, " failure in new - has no primary interface" );
                }
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
    // IP points to data field
    ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (*pCore->IP++);

    ForthInterface* pPrimaryInterface = pClassVocab->GetInterface( 0 );
    if ( pPrimaryInterface )
    {
        long nBytes = pClassVocab->GetSize();
        void* pData = malloc( nBytes );
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
        pEngine->SetError( kForthErrorBadSyntax, "initMemberStringOp can only be used inside a method" );
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
        pEngine->SetError( kForthErrorBadSyntax, "initMemberStringOp can only be used on a simple string" );
        return;
    }
    long len = pEntry[2] - 9;
    long varOffset = (pEntry[0] << 10) | len;

    pEngine->CompileOpcode( COMPILED_OP( kOpInitMemberString, varOffset ) );
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
    pEngine->StartOpDefinition();
    pEngine->CompileOpcode( OP_DO_ENUM );
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
    pVocab->DefineInstance();
    SET_IP( (long *) (RPOP) );
}

// doStructOp is compiled at the start of each user-defined global structure instance
FORTHOP( doStructOp )
{
    // IP points to data field
    SPUSH( (long) GET_IP );
    SET_IP( (long *) (RPOP) );
}

// doStructArrayOp is compiled as the only op of each user-defined global structure array instance
// the word after this op is the padded element length
// after that word is storage for structure 0 of the array
FORTHOP( doStructArrayOp )
{
    // IP points to data field
    long *pA = GET_IP;
    long nBytes = (*pA++) * SPOP;

    SPUSH( ((long) pA) + nBytes );
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
    
    if ( pEntry ) {
        long op = *pEntry;
        ulong opVal = FORTH_OP_VALUE( op );
        switch ( FORTH_OP_TYPE( op ) )
        {
            case kOpBuiltIn:
                *pEntry = COMPILED_OP( kOpBuiltInImmediate, opVal );
                break;

            case kOpUserDef:
                *pEntry = COMPILED_OP( kOpUserDefImmediate, opVal );
                break;

            case kOpUserCode:
                *pEntry = COMPILED_OP( kOpUserCodeImmediate, opVal );
                break;

            default:
                CONSOLE_STRING_OUT( "!!!! Can\'t set precedence for " );
                CONSOLE_STRING_OUT( pSym );
                CONSOLE_STRING_OUT( "s - wrong type !!!!\n" );
                TRACE( "!!!! Can\'t set precedence for %s - wrong type !!!!\n", pSym );
                break;
        }
    } else {
        CONSOLE_STRING_OUT( "!!!! Failure finding symbol " );
        CONSOLE_STRING_OUT( pSym );
        TRACE( "!!!! Failure finding symbol %s !!!!\n", pSym );
    }
}

FORTHOP( loadStrOp )
{
    FILE *pInFile;
    ForthEngine *pEngine = GET_ENGINE;
    char *pFileName = ((char *) (SPOP));

    if ( pFileName != NULL ) {
        pInFile = fopen( pFileName, "r" );
        if ( pInFile != NULL ) {
            pEngine->PushInputFile( pInFile );
        } else {
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
    loadStrOp( pCore );
}

FORTHOP( loadDoneOp )
{
    GET_ENGINE->PopInputStream();
}

FORTHOP( interpretOp )
{
    char* pStr = (char *) SPOP;
    if ( pStr != NULL )
    {
        int len = strlen( pStr );
        ForthEngine *pEngine = GET_ENGINE;
        pEngine->PushInputBuffer( pStr, len );
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

FORTHOP( tickOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pToken = pEngine->GetNextSimpleToken();
    long *pSymbol = pEngine->FindSymbol( pToken );
    if ( pSymbol != NULL ) {
        SPUSH( *pSymbol );
    } else {
        SET_ERROR( kForthErrorUnknownSymbol );
    }
}

FORTHOP( executeOp )
{
    long prog[2];
    long *oldIP = GET_IP;

    // execute the opcode
    prog[0] = SPOP;
    prog[1] = BUILTIN_OP( OP_DONE );
    
    SET_IP( prog );
    InnerInterpreter( pCore );
    SET_IP( oldIP );
}

// has precedence!
FORTHOP( compileOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pToken = pEngine->GetNextSimpleToken();
    long *pSymbol = pEngine->FindSymbol( pToken );
    if ( pSymbol != NULL ) {
        pEngine->CompileOpcode( *pSymbol );
    } else {
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
    if ( pSymbol != NULL ) {
        pEngine->CompileOpcode( OP_INT_VAL );
        pEngine->CompileLong( *pSymbol );
    } else {
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
    if ( pOutFile != NULL ) {
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
    if ( pOutStr != NULL ) {
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
    div_t v;
    long base;
    bool bIsNegative, bPrintUnsigned;
    ulong urem;
    ePrintSignedMode signMode;

    if ( val == 0 ) {
        strcpy( buff, "0" );
        pNext = buff;
    } else {
        base = *(GET_BASE_REF);

        signMode = (ePrintSignedMode) GET_PRINT_SIGNED_NUM_MODE;
        if ( (base == 10) && (signMode == kPrintSignedDecimal) ) {

            // most common case - print signed decimal
            sprintf( buff, "%d", val );
            pNext = buff;

        } else {

            // unsigned or any base other than 10

            *--pNext = 0;
            bPrintUnsigned = !(signMode == kPrintAllSigned);
            if ( bPrintUnsigned ) {
                // since div is defined as signed divide/mod, make sure
                //   that the number is not negative by generating the bottom digit
                bIsNegative = false;
                urem = ((ulong) val) % ((ulong) base);
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                val = ((ulong) val) / ((ulong) base);
            } else {
                bIsNegative = ( val < 0 );
                if ( bIsNegative ) {
                    val = (-val);
                }
            }
            while ( val != 0 ) {
                v = div( val, base );
                *--pNext = (char) ( (v.rem < 10) ? (v.rem + '0') : ((v.rem - 10) + 'a') );
                val = v.quot;
            }
            if ( bIsNegative ) {
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

extern long fprintfSub( ForthCoreState* pCore );
extern long sprintfSub( ForthCoreState* pCore );
extern long fscanfSub( ForthCoreState* pCore );
extern long sscanfSub( ForthCoreState* pCore );


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
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printCharOp )
{
    NEEDS(1);
    //long oof = SPOP;
    char buff[2];
//    buff[0] = 59;
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

	pEngine->ResetConsoleOut( pCore->pThread );
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
FORTHOP( fopenOp )
{
    NEEDS(2);
    char *pAccess = (char *) SPOP;
    char *pName = (char *) SPOP;
    
    FILE *pFP = fopen( pName, pAccess );
    SPUSH( (long) pFP );
}

FORTHOP( fcloseOp )
{
    NEEDS(1);
    
    int result = fclose( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( fseekOp )
{
    int ctrl = SPOP;
    int offset = SPOP;
    FILE *pFP = (FILE *) SPOP;
    
    int result = fseek( pFP, offset, ctrl );
    SPUSH( result );
}

FORTHOP( freadOp )
{
    NEEDS(4);
    
    FILE *pFP = (FILE *) SPOP;
    int itemSize = SPOP;
    int numItems = SPOP;
    void *pDst = (void *) SPOP;
    
    int result = fread( pDst, numItems, itemSize, pFP );
    SPUSH( result );
}

FORTHOP( fwriteOp )
{
    
    NEEDS(4);
    FILE *pFP = (FILE *) SPOP;
    int itemSize = SPOP;
    int numItems = SPOP;
    void *pSrc = (void *) SPOP;
    
    int result = fwrite( pSrc, numItems, itemSize, pFP );
    SPUSH( result );
}

FORTHOP( fgetcOp )
{    
    NEEDS(1);
    FILE *pFP = (FILE *) SPOP;
    
    int result = fgetc( pFP );
    SPUSH( result );
}

FORTHOP( fputcOp )
{    
    NEEDS(2);
    FILE *pFP = (FILE *) SPOP;
    int outChar = SPOP;
    
    int result = fputc( outChar, pFP );
    SPUSH( result );
}

FORTHOP( feofOp )
{
    NEEDS(1);
    int result = feof( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( ftellOp )
{
    NEEDS(1);
    int result = ftell( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( flenOp )
{
    NEEDS(1);
    FILE* pFile = (FILE *) SPOP;
    int oldPos = ftell( pFile );
    fseek( pFile, 0, SEEK_END );
    int result = ftell( pFile );
    fseek( pFile, oldPos, SEEK_SET );
    SPUSH( result );
}

FORTHOP( systemOp )
{
    NEEDS(1);
    int result = -1;
    ForthEngine *pEngine = GET_ENGINE;

    // open temp file which will take standard output
    FILE* outStream = fopen( "_system_stdout.txt", "w" );
    if ( outStream == NULL )
    {
        SET_ERROR( kForthErrorFileOpen );
        pEngine->AddErrorText( "system: failure opening _system_stdout.txt\n" );
        SPUSH( result );
        return;
    }

    // open temp file which will take standard error
    FILE* errStream = fopen( "_system_stderr.txt", "w" );
    if ( errStream == NULL )
    {
        SET_ERROR( kForthErrorFileOpen );
        pEngine->AddErrorText( "system: failure opening _system_stderr.txt\n" );
        fclose( outStream );
        SPUSH( result );
        return;
    }

    // dup standard output file descriptor so we can restore it on exit
    int oldStdOut = _dup( 1 );
    if( oldStdOut == -1 )
    {
        SET_ERROR( kForthErrorFileOpen );
        pEngine->AddErrorText( "system: failure dup-ing stdout\n" );
        fclose( outStream );
        fclose( errStream );
        SPUSH( result );
        return;
    }

    // dup standard error file descriptor so we can restore it on exit
    int oldStdErr = _dup( 2 );
    if( oldStdErr == -1 )
    {
        SET_ERROR( kForthErrorFileOpen );
        pEngine->AddErrorText( "system: failure dup-ing stderr\n" );
        fclose( outStream );
        fclose( errStream );
        _dup2( oldStdOut, 1 );
        SPUSH( result );
        return;
    }

    // redirect standard out to temp output file
    if ( _dup2( _fileno( outStream ), 1 ) == -1 )
    {
        SET_ERROR( kForthErrorFileOpen );
        pEngine->AddErrorText( "system: failure redirecting stdout to _system_stdout.txt\n" );
    }
    else
    {
        // redirect standard error to temp error file
        if ( _dup2( _fileno( errStream ), 2 ) == -1 )
        {
            SET_ERROR( kForthErrorFileOpen );
            pEngine->AddErrorText( "system: failure redirecting stderr to _system_stderr.txt\n" );
        }
        else
        {
            // have DOS shell execute command line string pointed to by TOS
            result = system( (char *) SPOP );
            fflush( stdout );
            fflush( stderr );
            // close standard error stream, then reopen it on oldStdError (console output)
            _dup2( oldStdErr, 2 );
        }
         // close standard output stream, then reopen it on oldStdOut (console output)
        _dup2( oldStdOut, 1 );
    }
    fclose( outStream );
    fclose( errStream );

    // dump contents of output and error files using forth console IO routines
    char buff[2];
    buff[1] = 0;
    outStream = fopen( "_system_stdout.txt", "r" );
    if ( outStream )
    {
        int ch;
        while ( (ch = fgetc( outStream )) != EOF )
        {
            buff[0] = (char) ch;
            CONSOLE_STRING_OUT( buff );
        }
        fclose( outStream );
        errStream = fopen( "_system_stderr.txt", "r" );
        if ( errStream )
        {
            while ( (ch = fgetc( errStream )) != EOF )
            {
                buff[0] = (char) ch;
                CONSOLE_STRING_OUT( buff );
            }
            fclose( errStream );
        }
        else
        {
            SET_ERROR( kForthErrorFileOpen );
            pEngine->AddErrorText( "system: failure reopening _system_stderr.txt\n" );
        }
    }
    else
    {
        SET_ERROR( kForthErrorFileOpen );
        pEngine->AddErrorText( "system: failure reopening _system_stdout.txt\n" );
    }

    SPUSH( result );
}

FORTHOP( chdirOp )
{
    NEEDS(1);
    int result = chdir( (const char *) SPOP );
    SPUSH( result );
}



//##############################
//
// Default input/output file ops
//

FORTHOP( stdinOp )
{
    SPUSH( (long) stdin );
}

FORTHOP( stdoutOp )
{
    SPUSH( (long) stdout );
}

FORTHOP( stderrOp )
{
    SPUSH( (long) stderr );
}

FORTHOP( dstackOp )
{
    long *pSP = GET_SP;
    int nItems = GET_SDEPTH;
    int i;
    char buff[64];

    sprintf( buff, "stack[%d]:", nItems );
    CONSOLE_STRING_OUT( buff );
    for ( i = 0; i < nItems; i++ ) {
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
    for ( i = 0; i < nItems; i++ ) {
        CONSOLE_STRING_OUT( " " );
        printNumInCurrentBase( pCore, *pRP++ );
    }
    CONSOLE_STRING_OUT( "\n" );
}


FORTHOP( statsOp )
{
    char buff[512];

    sprintf( buff, "pCore %p pEngine %p pThread %p     DP %p DBase %p    IP %p\n",
             pCore, pCore->pEngine, pCore->pThread, pCore->DP, pCore->DBase, pCore->IP );
    CONSOLE_STRING_OUT( buff );
    sprintf( buff, "SP %p ST %p SLen %d    RP %p RT %p RLen %d\n",
             pCore->SP, pCore->ST, pCore->SLen,
             pCore->RP, pCore->RT, pCore->RLen );
    CONSOLE_STRING_OUT( buff );
    sprintf( buff, "%d builtins    %d userops\n", pCore->numBuiltinOps, pCore->numUserOps );
    CONSOLE_STRING_OUT( buff );
}

FORTHOP( describeOp )
{
    char buff[512];

    ForthEngine *pEngine = GET_ENGINE;
    char* pSym = pEngine->GetNextSimpleToken();
    ForthTypesManager* pManager = ForthTypesManager::GetInstance();
    ForthStructVocabulary* pVocab = pManager->GetStructVocabulary( pSym );
    if ( pVocab )
    {
        // show structure vocabulary entries
        while ( pVocab )
        {
            sprintf( buff, "%s vocabulary %s:\n", ((pVocab->IsClass() ? "class" : "struct")), pVocab->GetName() );
            CONSOLE_STRING_OUT( buff );
            char quit = ShowVocab( pEngine->GetCoreState(), pVocab );
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
    else
    {
        pEngine->DescribeSymbol( pSym );
    }
}

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
    pEngine->CompileOpcode( OP_DO_VOCAB );
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
        pEngine->SetError( kForthErrorBadParameter, " is not a DLL vocabulary" );
    }
    ulong numArgs = SPOP;
    pVocab->AddEntry( pProcName, numArgs );
}

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
	char *pSrc = pShell->GetToken( ')' );
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

FORTHOP( strtimeOp )
{
    _strtime( (char *) (SPOP) );
}

FORTHOP( strdateOp )
{
    _strdate( (char *) (SPOP) );
}

FORTHOP( millitimeOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    SPUSH( (long) pEngine->GetElapsedTime() );
}

void AddBuiltinClasses( ForthEngine* pEngine )
{
    // TBD: add all builtin classes
}

#define OP( func, funcName )  { funcName, kOpBuiltIn, (ulong) func }

// ops which have precedence (execute at compile time)
#define PRECOP( func, funcName )  { funcName, kOpBuiltInImmediate, (ulong) func }

// NOTE: the order of the first few entries in this table must agree
// with the list near the top of the file!  (look for COMPILED_OP)

extern GFORTHOP( doByteOp );
extern GFORTHOP( doShortOp );
extern GFORTHOP( doIntOp );
extern GFORTHOP( doIntOp );
extern GFORTHOP( doFloatOp );
extern GFORTHOP( doDoubleOp );
extern GFORTHOP( doStringOp );
extern GFORTHOP( doOpOp );
extern GFORTHOP( doObjectOp );
extern GFORTHOP( doByteArrayOp );
extern GFORTHOP( doShortArrayOp );
extern GFORTHOP( doIntArrayOp );
extern GFORTHOP( doIntArrayOp );
extern GFORTHOP( doFloatArrayOp );
extern GFORTHOP( doDoubleArrayOp );
extern GFORTHOP( doStringArrayOp );
extern GFORTHOP( doOpArrayOp );
extern GFORTHOP( doObjectArrayOp );

baseDictEntry baseDict[] =
{
    ///////////////////////////////////////////
    //  STUFF WHICH IS COMPILED BY OTHER WORDS
    //   DO NOT REARRANGE UNDER PAIN OF DEATH
    ///////////////////////////////////////////
    OP(     abortOp,                "abort" ),
    OP(     dropOp,                 "drop" ),
    OP(     doDoesOp,               "_doDoes" ),
    OP(     litOp,                  "lit" ),
    OP(     litOp,                  "flit" ),
    OP(     dlitOp,                 "dlit" ),
    OP(     doVariableOp,           "_doVariable" ),
    OP(     doConstantOp,           "_doConstant" ),
    OP(     doDConstantOp,          "_doDConstant" ),
    OP(     endBuildsOp,            "_endBuilds" ),
    OP(     doneOp,                 "done" ),
    OP(     doByteOp,               "_doByte" ),
    OP(     doShortOp,              "_doShort" ),
    OP(     doIntOp,                "_doInt" ),
    OP(     doFloatOp,              "_doFloat" ),
    OP(     doDoubleOp,             "_doDouble" ),
    OP(     doStringOp,             "_doString" ),
    OP(     doOpOp,                 "_doOp" ),
    OP(     doObjectOp,             "_doObject" ),
    // the order of the next four opcodes has to match the order of kVarRef...kVarMinusStore
    OP(     addressOfOp,            "addressOf" ),
    OP(     intoOp,                 "->" ),
    OP(     addToOp,                "->+" ),
    OP(     subtractFromOp,         "->-" ),
    OP(     doExitOp,               "_exit" ),      // exit normal op with no vars
    OP(     doExitLOp,              "_exitL" ),     // exit normal op with local vars
    OP(     doExitMOp,              "_exitM" ),     // exit method op with no vars
    OP(     doExitMLOp,             "_exitML" ),    // exit method op with local vars
    OP(     doVocabOp,              "_doVocab" ),
    OP(     doByteArrayOp,          "_doByteArray" ),
    OP(     doShortArrayOp,         "_doShortArray" ),
    OP(     doIntArrayOp,           "_doIntArray" ),
    OP(     doFloatArrayOp,         "_doFloatArray" ),
    OP(     doDoubleArrayOp,        "_doDoubleArray" ),
    OP(     doStringArrayOp,        "_doStringArray" ),
    OP(     doOpArrayOp,            "_doOpArray" ),
    OP(     doObjectArrayOp,        "_doObjectArray" ),
    OP(     initStringOp,           "initString" ),
    OP(     initStringArrayOp,      "initStringArray" ),
    OP(     plusOp,                 "+" ),
    OP(     fetchOp,                "@" ),
    OP(     badOpOp,                "badOp" ),
    OP(     doStructOp,             "_doStruct" ),
    OP(     doStructArrayOp,        "_doStructArray" ),
    OP(     doStructTypeOp,         "_doStructType" ),
    OP(     doClassTypeOp,          "_doClassType" ),
    PRECOP( doEnumOp,               "_doEnum" ),
    OP(     doDoOp,                 "_do" ),
    OP(     doLoopOp,               "_loop" ),
    OP(     doLoopNOp,              "_+loop" ),
    OP(     doNewOp,                "_doNew" ),
    OP(     dfetchOp,               "d@" ),

    // stuff below this line can be rearranged
    
    ///////////////////////////////////////////
    //  integer math
    ///////////////////////////////////////////
    OP(     minusOp,                "-" ),
    OP(     timesOp,                "*" ),
    OP(     times2Op,               "2*" ),
    OP(     times4Op,               "4*" ),
    OP(     divideOp,               "/" ),
    OP(     divide2Op,              "2/" ),
    OP(     divide4Op,              "4/" ),
    OP(     divmodOp,               "/mod" ),
    OP(     modOp,                  "mod" ),
    OP(     negateOp,               "negate" ),
    
    ///////////////////////////////////////////
    //  single-precision fp math
    ///////////////////////////////////////////
    OP(     fplusOp,                "f+" ),
    OP(     fminusOp,               "f-" ),
    OP(     ftimesOp,               "f*" ),
    OP(     fdivideOp,              "f/" ),
    
    ///////////////////////////////////////////
    //  double-precision fp math
    ///////////////////////////////////////////
    OP(     dplusOp,                "d+" ),
    OP(     dminusOp,               "d-" ),
    OP(     dtimesOp,               "d*" ),
    OP(     ddivideOp,              "d/" ),


    ///////////////////////////////////////////
    //  double-precision fp functions
    ///////////////////////////////////////////
    OP(     dsinOp,                 "dsin" ),
    OP(     dasinOp,                "darcsin" ),
    OP(     dcosOp,                 "dcos" ),
    OP(     dacosOp,                "darccos" ),
    OP(     dtanOp,                 "dtan" ),
    OP(     datanOp,                "darctan" ),
    OP(     datan2Op,               "darctan2" ),
    OP(     dexpOp,                 "dexp" ),
    OP(     dlnOp,                  "dln" ),
    OP(     dlog10Op,               "dlog10" ),
    OP(     dpowOp,                 "dpow" ),
    OP(     dsqrtOp,                "dsqrt" ),
    OP(     dceilOp,                "dceil" ),
    OP(     dfloorOp,               "dfloor" ),
    OP(     dabsOp,                 "dabs" ),
    OP(     dldexpOp,               "dldexp" ),
    OP(     dfrexpOp,               "dfrexp" ),
    OP(     dmodfOp,                "dmodf" ),
    OP(     dfmodOp,                "dfmod" ),
    
    ///////////////////////////////////////////
    //  integer/float/double conversion
    ///////////////////////////////////////////
    OP(     i2fOp,                  "i2f" ), 
    OP(     i2dOp,                  "i2d" ),
    OP(     f2iOp,                  "f2i" ),
    OP(     f2dOp,                  "f2d" ),
    OP(     d2iOp,                  "d2i" ),
    OP(     d2fOp,                  "d2f" ),
    
    ///////////////////////////////////////////
    //  control flow
    ///////////////////////////////////////////
    OP(     callOp,                 "call" ),
    OP(     gotoOp,                 "goto" ),
    PRECOP( doOp,                   "do" ),
    PRECOP( loopOp,                 "loop" ),
    PRECOP( loopNOp,                "+loop" ),
    OP(     iOp,                    "i" ),
    OP(     jOp,                    "j" ),
    OP(     unloopOp,               "unloop" ),
    OP(     leaveOp,                "leave" ),
    PRECOP( ifOp,                   "if" ),
    PRECOP( elseOp,                 "else" ),
    PRECOP( endifOp,                "endif" ),
    PRECOP( beginOp,                "begin" ),
    PRECOP( untilOp,                "until" ),
    PRECOP( whileOp,                "while" ),
    PRECOP( repeatOp,               "repeat" ),
    PRECOP( againOp,                "again" ),
    PRECOP( caseOp,                 "case" ),
    PRECOP( ofOp,                   "of" ),
    PRECOP( endofOp,                "endof" ),
    PRECOP( endcaseOp,              "endcase" ),

    ///////////////////////////////////////////
    //  bit-vector logic
    ///////////////////////////////////////////
    OP(     orOp,                   "or" ),
    OP(     andOp,                  "and" ),
    OP(     xorOp,                  "xor" ),
    OP(     invertOp,               "~" ),
    OP(     lshiftOp,               "<<" ),
    OP(     rshiftOp,               ">>" ),

    ///////////////////////////////////////////
    //  boolean logic
    ///////////////////////////////////////////
    OP(     notOp,                  "not" ),
    OP(     trueOp,                 "true" ),
    OP(     falseOp,                "false" ),
    OP(     nullOp,                 "null" ),
    OP(     dnullOp,                "dnull" ),

    ///////////////////////////////////////////
    //  integer comparisons
    ///////////////////////////////////////////
    OP(     equalsOp,               "==" ),
    OP(     notEqualsOp,            "!=" ),
    OP(     greaterThanOp,          ">" ),
    OP(     greaterEqualsOp,        ">=" ),
    OP(     lessThanOp,             "<" ),
    OP(     lessEqualsOp,           "<=" ),
    OP(     equalsZeroOp,           "0==" ),
    OP(     notEqualsZeroOp,        "0!=" ),
    OP(     greaterThanZeroOp,      "0>" ),
    OP(     greaterEqualsZeroOp,    "0>=" ),
    OP(     lessThanZeroOp,         "0<" ),
    OP(     lessEqualsZeroOp,       "0<=" ),
    OP(     unsignedGreaterThanOp,  "u>" ),
    OP(     unsignedLessThanOp,     "u<" ),
    
    ///////////////////////////////////////////
    //  stack manipulation
    ///////////////////////////////////////////
    OP(     rpushOp,                "r<" ),
    OP(     rpopOp,                 "r>" ),
    OP(     rdropOp,                "rdrop" ),
    OP(     rpOp,                   "rp" ),
    OP(     rzeroOp,                "r0" ),
    OP(     dupOp,                  "dup" ),
    OP(     swapOp,                 "swap" ),
    OP(     overOp,                 "over" ),
    OP(     rotOp,                  "rot" ),
    OP(     tuckOp,                 "tuck" ),
    OP(     pickOp,                 "pick" ),
    OP(     rollOp,                 "roll" ),
    OP(     spOp,                   "sp" ),
    OP(     szeroOp,                "s0" ),
    OP(     fpOp,                   "fp" ),
    OP(     ddupOp,                 "ddup" ),
    OP(     dswapOp,                "dswap" ),
    OP(     ddropOp,                "ddrop" ),
    OP(     doverOp,                "dover" ),
    OP(     drotOp,                 "drot" ),
    
    ///////////////////////////////////////////
    //  data compilation/allocation
    ///////////////////////////////////////////
    OP(     alignOp,                "align" ),
    OP(     allotOp,                "allot" ),
    OP(     callotOp,               "callot" ),
    OP(     commaOp,                "," ),
    OP(     cCommaOp,               "c," ),
    OP(     hereOp,                 "here" ),
    OP(     mallocOp,               "malloc" ),
    OP(     freeOp,                 "free" ),

    ///////////////////////////////////////////
    //  memory store/fetch
    ///////////////////////////////////////////
    OP(     storeOp,                "!" ),
    OP(     cstoreOp,               "c!" ),
    OP(     cfetchOp,               "c@" ),
    OP(     scfetchOp,              "sc@" ),
    OP(     c2lOp,                  "c2l" ),
    OP(     wstoreOp,               "w!" ),
    OP(     wfetchOp,               "w@" ),
    OP(     swfetchOp,              "sw@" ),
    OP(     w2lOp,                  "w2l" ),
    OP(     dstoreOp,               "d!" ),
    OP(     memcpyOp,               "memcpy" ),
    OP(     memsetOp,               "memset" ),
    OP(     setVarActionOp,         "varAction!" ),
    OP(     getVarActionOp,         "varAction@" ),

    ///////////////////////////////////////////
    //  string manipulation
    ///////////////////////////////////////////
    OP(     strcpyOp,               "strcpy" ),
    OP(     strncpyOp,              "strncpy" ),
    OP(     strlenOp,               "strlen" ),
    OP(     strcatOp,               "strcat" ),
    OP(     strncatOp,              "strncat" ),
    OP(     strchrOp,               "strchr" ),
    OP(     strrchrOp,              "strrchr" ),
    OP(     strcmpOp,               "strcmp" ),
    OP(     stricmpOp,              "stricmp" ),
    OP(     strstrOp,               "strstr" ),
    OP(     strtokOp,               "strtok" ),

    ///////////////////////////////////////////
    //  op definition
    ///////////////////////////////////////////
    OP(     buildsOp,               "builds" ),
    PRECOP( doesOp,                 "does" ),
    PRECOP( exitOp,                 "exit" ),
    PRECOP( semiOp,                 ";" ),
    OP(     colonOp,                ":" ),
    OP(     codeOp,                 "code" ),
    OP(     createOp,               "create" ),
    OP(     variableOp,             "variable" ),
    OP(     constantOp,             "constant" ),
    OP(     dconstantOp,            "dconstant" ),
    PRECOP( byteOp,                 "byte" ),
    PRECOP( shortOp,                "short" ),
    PRECOP( intOp,                  "int" ),
    PRECOP( floatOp,                "float" ),
    PRECOP( doubleOp,               "double" ),
    PRECOP( stringOp,               "string" ),
    PRECOP( opOp,                   "op" ),
    PRECOP( objectOp,               "object" ),
    PRECOP( voidOp,                 "void" ),
    PRECOP( arrayOfOp,              "arrayOf" ),
    PRECOP( ptrToOp,                "ptrTo" ),
    OP(     structOp,               "struct:" ),
    OP(     endstructOp,            ";struct" ),
    OP(     classOp,                "class:" ),
    OP(     endclassOp,             ";class" ),
    OP(     methodOp,               "method:" ),
    PRECOP( endmethodOp,            ";method" ),
    PRECOP( returnsOp,              "returns" ),
    OP(     doMethodOp,             "doMethod" ),
    OP(     implementsOp,           "implements:" ),
    OP(     endimplementsOp,        ";implements" ),
    OP(     unionOp,                "union" ),
    OP(     extendsOp,              "extends" ),
    PRECOP( sizeOfOp,               "sizeOf" ),
    PRECOP( offsetOfOp,             "offsetOf" ),
    OP(     thisOp,                 "this" ),
    OP(     thisDataOp,             "thisData" ),
    OP(     thisMethodsOp,          "thisMethods" ),
    PRECOP( newOp,                  "new" ),
    PRECOP( initMemberStringOp,     "initMemberString" ),
    OP(     enumOp,                 "enum:" ),
    OP(     endenumOp,              ";enum" ),
    PRECOP( recursiveOp,            "recursive" ),
    OP(     precedenceOp,           "precedence" ),
    OP(     loadStrOp,              "load$" ),
    OP(     loadOp,                 "load" ),
    OP(     loadDoneOp,             "loaddone" ),
    OP(     interpretOp,            "interpret" ),
    PRECOP( stateInterpretOp,       "[" ),
    OP(     stateCompileOp,         "]" ),
    OP(     stateOp,                "state" ),
    OP(     tickOp,                 "\'" ),
    OP(     executeOp,              "execute" ),
    PRECOP( compileOp,              "[compile]" ),
    PRECOP( bracketTickOp,          "[\']" ),

    ///////////////////////////////////////////
    //  vocabulary/symbol
    ///////////////////////////////////////////
    OP(     forthVocabOp,           "forth" ),
    OP(     definitionsOp,          "definitions" ),
    OP(     vocabularyOp,           "vocabulary" ),
    OP(     alsoOp,                 "also" ),
    OP(     previousOp,             "previous" ),
    OP(     onlyOp,                 "only" ),
    OP(     forgetOp,               "forget" ),
    OP(     autoforgetOp,           "autoforget" ),
    OP(     vlistOp,                "vlist" ),
    OP(     addressOfOp,            "getNewest" ),
    OP(     intoOp,                 "findEntry" ),
    OP(     addToOp,                "findEntryValue" ),
    OP(     subtractFromOp,         "addEntry" ),
    OP(     removeEntryOp,          "removeEntry" ),
    OP(     entryLengthOp,          "entryLength" ),
    OP(     numEntriesOp,           "numEntries" ),

    ///////////////////////////////////////////
    //  text display
    ///////////////////////////////////////////
    OP(     printNumOp,             "." ),
    OP(     printNumDecimalOp,      "%d" ),
    OP(     printNumHexOp,          "%x" ),
    OP(     printStrOp,             "%s" ),
    OP(     printCharOp,            "%c" ),
    OP(     printSpaceOp,           "%bl" ),
    OP(     printNewlineOp,         "%nl" ),
    OP(     printFloatOp,           "%f" ),
    OP(     printDoubleOp,          "%g" ),
    OP(     printFormattedOp,       "%fmt" ),
    OP(     fprintfOp,              "fprintf" ),
    OP(     sprintfOp,              "sprintf" ),
    OP(     fscanfOp,               "fscanf" ),
    OP(     sscanfOp,               "sscanf" ),
    OP(     baseOp,                 "base" ),
    OP(     decimalOp,              "decimal" ),
    OP(     hexOp,                  "hex" ),
    OP(     printDecimalSignedOp,   "printDecimalSigned" ),
    OP(     printAllSignedOp,       "printAllSigned" ),
    OP(     printAllUnsignedOp,     "printAllUnsigned" ),
    OP(     outToFileOp,            "outToFile" ),
    OP(     outToScreenOp,          "outToScreen" ),
    OP(     outToStringOp,          "outToString" ),
    OP(     outToOpOp,              "outToOp" ),
    OP(     getConOutFileOp,        "getConOutFile" ),

    ///////////////////////////////////////////
    //  file manipulation
    ///////////////////////////////////////////
    OP(     fopenOp,                "fopen" ),
    OP(     fcloseOp,               "fclose" ),
    OP(     fseekOp,                "fseek" ),
    OP(     freadOp,                "fread" ),
    OP(     fwriteOp,               "fwrite" ),
    OP(     fgetcOp,                "fgetc" ),
    OP(     fputcOp,                "fputc" ),
    OP(     feofOp,                 "feof" ),
    OP(     ftellOp,                "ftell" ),
    OP(     flenOp,                 "flen" ),
    OP(     stdinOp,                "stdin" ),
    OP(     stdoutOp,               "stdout" ),
    OP(     stderrOp,               "stderr" ),
    
    ///////////////////////////////////////////
    //  input buffer
    ///////////////////////////////////////////
    OP(     blwordOp,               "blword" ),
    OP(     wordOp,                 "word" ),
    PRECOP( commentOp,              "/*" ),
    PRECOP( parenCommentOp,         "(" ),
    OP(     parenIsCommentOp,       "parenIsComment" ),
    OP(     getInBufferBaseOp,      "getInBufferBase" ),
    OP(     getInBufferPointerOp,   "getInBufferPointer" ),
    OP(     setInBufferPointerOp,   "setInBufferPointer" ),
    OP(     getInBufferLengthOp,    "getInBufferLength" ),
    OP(     fillInBufferOp,         "fillInBuffer" ),

    ///////////////////////////////////////////
    //  DLL support
    ///////////////////////////////////////////
    OP(     DLLVocabularyOp,        "DLLVocabulary" ),
    OP(     addDLLEntryOp,          "addDLLEntry" ),

    ///////////////////////////////////////////
    //  time and date
    ///////////////////////////////////////////
    OP(     strtimeOp,              "strtime" ),
    OP(     strdateOp,              "strdate" ),
    OP(     millitimeOp,            "millitime" ),

    ///////////////////////////////////////////
    //  admin/debug/system
    ///////////////////////////////////////////
    OP(     dstackOp,               "dstack" ),
    OP(     drstackOp,              "drstack" ),
    OP(     systemOp,               "system" ),
    OP(     chdirOp,                "chdir" ),
    OP(     byeOp,                  "bye" ),
    OP(     argvOp,                 "argv" ),
    OP(     argcOp,                 "argc" ),
    OP(     turboOp,                "turbo" ),
    OP(     statsOp,                "stats" ),
    OP(     describeOp,             "describe" ),
    OP(     errorOp,                "error" ),
    OP(     addErrorTextOp,         "addErrorText" ),

    // following must be last in table
    OP(     NULL,                   "" )
};


//############################################################################
//
//  The end of all things...
//
//############################################################################

};  // end extern "C"


