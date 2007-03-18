//////////////////////////////////////////////////////////////////////
//
// ForthOps.cpp: forth builtin operator definitions
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Forth.h"
#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthVocabulary.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include <conio.h>
#include <direct.h>

extern "C" {

// Forth operator TBDs:
// - add dpi, fpi (pi constant)
// - add string lib operators

// compiled token is 32-bits,
// top 8 bits are "super" opcode (hiOp)
// bottom 24 bits are immediate operand
// hiOp == 0   built-in simple op, immediate operand = built-in opcode index
// hiOp == 1   user-defined simple op, immediate operand = user opcode index
//             dictionary entry .value is opcode IP
// hiOp == 2   user-defined op made by build-does word. immediate operand = user opcode index
//             dictionary entry .value points to lword holding new IP, .value + 4 is param address
// other values of hiOp are used to dispatch through a function table
// to get to built-in or user-defined ops which take immediate operand


// Idea: to speed dictionary search, pad all op names to be multiples of 4 in length,
//  and do string comparisons with longs

// TBD: where should "hard" addresses be used - right now, all addresses compiled into code
// or on stacks are relative
// -> hard addresses are never exposed


// TBD:
// - add immDataFuncs for built-in ops which take immediate data
// - init immDataFuncs at startup
// - add way for user to define ops which take immediate data
//  - have a single procedure which does all user immediate ops, and
//    have an array of IPs - the proc

//############################################################################
//
//   normal forth ops
//
//############################################################################


// done is used by the outer interpreter to make the inner interpreter
//  exit after interpreting a single opcode
FORTHOP(doneOp)
{
    SET_STATE( kResultDone );
}

// bye is a user command which causes the entire forth program to exit
FORTHOP(byeOp)
{
    SET_STATE( kResultExitShell );
}

// abort is a user command which causes the entire forth engine to exit,
//   and indicates that a fatal error has occured
FORTHOP( abortOp )
{
    SET_FATAL_ERROR( kForthErrorAbort );
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

FORTHOP(plusOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a + b );
}

FORTHOP(minusOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a - b );
}

FORTHOP(timesOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a * b );
}

FORTHOP(times2Op)
{
    NEEDS(1);
    SPUSH( SPOP << 1 );
}

FORTHOP(times4Op)
{
    NEEDS(1);
    SPUSH( SPOP << 2 );
}

FORTHOP(divideOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( a / b );
}

FORTHOP(divide2Op)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a >> 1 );
}

FORTHOP(divide4Op)
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
    // rstack: oldTP oldIP
    if ( GET_RDEPTH < 2 ) {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    } else {
        SET_TP( (long *) RPOP );
        SET_IP( (long *) RPOP );
    }
}

// exit method op with local vars
FORTHOP(doExitMLOp)
{
    // rstack: local_var_storage oldFP oldTP oldIP
    // FP points to oldFP
    SET_RP( GET_FP );
    SET_FP( (long *) (RPOP) );
    if ( GET_RDEPTH < 2 ) {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    } else {
        SET_TP( (long *) RPOP );
        SET_IP( (long *) RPOP );
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
    pEngine->CompileLong( OP_ABORT );
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
    pEngine->CompileLong( OP_DO_LOOP );
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
    pEngine->CompileLong( OP_DO_LOOPN );
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
    pEngine->CompileLong( OP_ABORT );
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
    pEngine->CompileLong( OP_ABORT );
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
    pEngine->CompileLong( COMPILED_OP( kOpBranchZ, (pBeginOp - GET_DP) - 1 ) );
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
    pEngine->CompileLong( OP_ABORT );
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
    pEngine->CompileLong( COMPILED_OP( kOpBranch, (pOp - GET_DP) - 1 ) );
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
    pEngine->CompileLong( COMPILED_OP( kOpBranch, (pBeginOp - GET_DP) - 1 ) );
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
    pEngine->CompileLong( OP_ABORT );
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
    pEngine->CompileLong( OP_ABORT );

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
        pEngine->CompileLong( OP_DROP );
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
    a = (GET_SP)[2];
    b = (GET_SP)[1];
    c = (GET_SP)[0];
    (GET_SP)[2] = b;
    (GET_SP)[1] = c;
    (GET_SP)[0] = a;
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
    SET_SP( GET_SP - 2 );
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
    long newOp;
    ForthEngine *pEngine = GET_ENGINE;
    
    // compile dodoes opcode & dummy word
    pEngine->CompileLong( OP_END_BUILDS );
    pEngine->CompileLong( 0 );
    // create a nameless vocabulary entry for does-body opcode
    //newOp = pVocab->AddSymbol( NULL, kOpUserDef, (long) GET_DP );
    newOp = pEngine->AddOp( GET_DP, kOpUserDef );
    newOp = COMPILED_OP( kOpUserDef, newOp );
    pEngine->CompileLong( OP_DO_DOES );
    // stuff does-body opcode in dummy word
    GET_DP[-2] = newOp;
    // compile local vars allocation op (if needed)
    pEngine->EndOpDefinition();
}

extern char *newestSymbol;
FORTHOP( newestSymbolOp )
{
    SPUSH( (long) GET_ENGINE->GetDefinitionVocabulary()->NewestSymbol() );
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
    long flags = pEngine->GetCompileFlags();

    switch ( flags & (kFECompileFlagHasLocalVars | kFECompileFlagIsMethod) ) {
    case 0:
        // normal definition, no local vars, not a method
        pEngine->CompileLong( OP_DO_EXIT );
        break;
    case kFECompileFlagHasLocalVars:
        // normal definition with local vars
        pEngine->CompileLong( OP_DO_EXIT_L );
        break;
    case kFECompileFlagIsMethod:
        // method definition, no local vars
        pEngine->CompileLong( OP_DO_EXIT_M );
        break;
    case (kFECompileFlagHasLocalVars | kFECompileFlagIsMethod):
        // method definition, with local vars
        pEngine->CompileLong( OP_DO_EXIT_ML );
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
    pEngine->SetCompileFlags( 0 );
    // finish current symbol definition
    // compile local vars allocation op (if needed)
    pEngine->EndOpDefinition( true );
}

FORTHOP( colonOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition( true );
    // switch to compile mode
    pEngine->SetCompileState( 1 );
    pEngine->SetCompileFlags( 0 );
}

FORTHOP( createOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition( false );
    pEngine->CompileLong( OP_DO_VAR );
}

FORTHOP( forgetOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char* pSym = pEngine->GetNextSimpleToken();
    if ( !pEngine->ForgetSymbol( pSym ) )
    {
        TRACE( "Error - attempt to forget unknown op %s from %s\n", pSym, pEngine->GetForthVocabulary()->GetName() );
        printf( "Error - attempt to forget unknown op %s from %s\n", pSym, pEngine->GetForthVocabulary()->GetName() );
    }
    // reset search & definitions vocabs in case we deleted a vocab we were using
    pEngine->SetDefinitionVocabulary( pEngine->GetForthVocabulary() );
    pEngine->SetSearchVocabulary( pEngine->GetForthVocabulary() );
}

// just like forget, but no error message if symbol not found
FORTHOP( autoforgetOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->ForgetSymbol( pEngine->GetNextSimpleToken() );
    // reset search & definitions vocabs in case we deleted a vocab we were using
    pEngine->SetDefinitionVocabulary( pEngine->GetForthVocabulary() );
    pEngine->SetSearchVocabulary( pEngine->GetForthVocabulary() );
}

FORTHOP( definitionsOp )
{
    GET_ENGINE->SetDefinitionVocabulary( (ForthVocabulary *) (SPOP) );
}

FORTHOP( usesOp )
{
    GET_ENGINE->SetSearchVocabulary( (ForthVocabulary *) (SPOP) );
}

FORTHOP( forthVocabOp )
{
    SPUSH( (long) (GET_ENGINE->GetForthVocabulary()) );
}

FORTHOP( searchVocabOp )
{
    if ( GET_VAR_OPERATION == kVarFetch )
    {
        SPUSH( (long) (GET_ENGINE->GetSearchVocabulary()) );
    }
    else if ( GET_VAR_OPERATION == kVarStore )
    {
        GET_ENGINE->SetSearchVocabulary( (ForthVocabulary *) (SPOP) );
    }
}

FORTHOP( definitionsVocabOp )
{
    if ( GET_VAR_OPERATION == kVarFetch )
    {
        SPUSH( (long) (GET_ENGINE->GetDefinitionVocabulary()) );
    }
    else if ( GET_VAR_OPERATION == kVarStore )
    {
        GET_ENGINE->SetDefinitionVocabulary( (ForthVocabulary *) (SPOP) );
    }
}

FORTHOP( vocabularyOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    ForthVocabulary *pDefinitionsVocab = pEngine->GetDefinitionVocabulary();
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition();
    pEngine->CompileLong( OP_DO_VOCAB );
    ForthVocabulary* pVocab = new ForthVocabulary( pEngine,
                                                   pDefinitionsVocab->GetEntryName( pDefinitionsVocab->GetNewestEntry() ),
                                                   1,
                                                   512,
                                                   GET_DP,
                                                   ForthVocabulary::GetEntryValue( pDefinitionsVocab->GetNewestEntry() ) );
    pVocab->SetNextSearchVocabulary( pEngine->GetSearchVocabulary() );
    pEngine->CompileLong( (long) pVocab );
}

FORTHOP( variableOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition();
    pEngine->CompileLong( OP_DO_VAR );
    pEngine->CompileLong( 0 );
}

FORTHOP( varsOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->StartVarsDefinition();
    pEngine->SetCompileState( 0 );
}

FORTHOP( endvarsOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->EndVarsDefinition();
    pEngine->SetCompileState( 1 );
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
    pEngine->CompileLong( OP_DO_CONSTANT );
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
    pEngine->CompileLong( OP_DO_DCONSTANT );
    double d = DPOP;
    pEngine->CompileDouble( d );
}

FORTHOP( doDConstantOp )
{
    // IP points to data field
    DPUSH( *((double *) (GET_IP)) );
    SET_IP( (long *) (RPOP) );
}

FORTHOP( intOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pToken = pEngine->GetNextSimpleToken();

    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->IsCompiling() )
    {
        if ( !pEngine->HasLocalVars() )
        {
            // this is first local var declaration in this user op
            pEngine->StartVarsDefinition();
        }
        // define local variable
        pEngine->AddLocalVar( pToken, kOpLocalInt, sizeof(long) );
    }
    else
    {
        if ( pEngine->InVarsDefinition() )
        {
            // define local variable
            pEngine->AddLocalVar( pToken, kOpLocalInt, sizeof(long) );
        }
        else
        {
            // define global variable
            pEngine->AddUserOp( pToken );
            pEngine->CompileLong( OP_DO_INT );
            pEngine->CompileLong( 0 );
        }
    }
}

FORTHOP( floatOp )
{
    float t = 0.0;
    ForthEngine *pEngine = GET_ENGINE;
    char *pToken = pEngine->GetNextSimpleToken();

    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->IsCompiling() )
    {
        if ( !pEngine->HasLocalVars() )
        {
            // this is first local var declaration in this user op
            pEngine->StartVarsDefinition();
        }
        // define local variable
        pEngine->AddLocalVar( pToken, kOpLocalFloat, sizeof(float) );
    }
    else
    {
        if ( pEngine->InVarsDefinition() )
        {
            // define local variable
            pEngine->AddLocalVar( pToken, kOpLocalFloat, sizeof(float) );
        }
        else
        {
            // define global variable
            pEngine->AddUserOp( pToken );
            pEngine->CompileLong( OP_DO_FLOAT );
            pEngine->CompileLong( *(long *) &t );
        }
    }
}


FORTHOP( doubleOp )
{
    double *pDT;
    ForthEngine *pEngine = GET_ENGINE;
    char *pToken = pEngine->GetNextSimpleToken();

    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->IsCompiling() )
    {
        if ( !pEngine->HasLocalVars() )
        {
            // this is first local var declaration in this user op
            pEngine->StartVarsDefinition();
        }
        // define local variable
        pEngine->AddLocalVar( pToken, kOpLocalDouble, sizeof(double) );
    }
    else
    {
        if ( pEngine->InVarsDefinition() )
        {
            // define local variable
            pEngine->AddLocalVar( pToken, kOpLocalDouble, sizeof(double) );
        }
        else
        {
            // define global variable
            pEngine->AddUserOp( pToken );
            pEngine->CompileLong( OP_DO_DOUBLE );
            pDT = (double *) GET_DP;
            *pDT++ = 0.0;
            pEngine->SetDP( (long *) pDT );
        }
    }
}

FORTHOP( stringOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    char *pToken = pEngine->GetNextSimpleToken();
    long len;

    if ( pEngine->IsCompiling() )
    {
        // the symbol just before "string" should have been an integer constant
        if ( pEngine->GetLastConstant( len ) )
        {
            // uncompile the integer contant opcode
            pEngine->UncompileLastOpcode();
            if ( !pEngine->HasLocalVars() )
            {
                // this is first local var declaration in this user op
                pEngine->StartVarsDefinition();
            }
            // define local variable
            pEngine->AddLocalVar( pToken, kOpLocalString, len );
        }
        else
        {
            SET_ERROR( kForthErrorMissingSize );
        }
    }
    else
    {
        len = SPOP;
        if ( pEngine->InVarsDefinition() )
        {
            // define local variable
            pEngine->AddLocalVar( pToken, kOpLocalString, len );
        }
        else
        {
            // define global variable
            pEngine->AddUserOp( pToken );
            pEngine->CompileLong( OP_DO_STRING );
            pEngine->CompileLong( len );
            pEngine->CompileLong( 0 );
            pEngine->AllotLongs( ((len  + 3) & ~3) >> 2 );
        }
    }
}

FORTHOP( doVocabOp )
{
    // IP points to data field
    SPUSH( *GET_IP );
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
    char *pEntry = (char *) pEngine->GetDefinitionVocabulary()->FindSymbol( pSym );
    
    if ( pEntry ) {
        pEngine->GetPrecedenceVocabulary()->CopyEntry( pEntry );
        pEngine->GetDefinitionVocabulary()->DeleteEntry( pEntry );
    } else {
        printf( "!!!! Failure finding symbol %s !!!!\n", pSym );
        TRACE( "!!!! Failure finding symbol %s !!!!\n", pSym );
    }
}

FORTHOP( loadOp )
{
    FILE *pInFile;
    ForthEngine *pEngine = GET_ENGINE;
    char *pFileName = pEngine->GetNextSimpleToken();

    if ( pFileName != NULL ) {
        pInFile = fopen( pFileName, "r" );
        if ( pInFile != NULL ) {
            pEngine->PushInputFile( pInFile );
        } else {
            printf( "!!!! Failure opening source file %s !!!!\n", pFileName );
            TRACE( "!!!! Failure opening source file %s !!!!\n", pFileName );
        }

    }
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
    long *pSymbol = (long *) (pEngine->FindSymbol( pToken ));
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
    long *pSymbol = (long *) (pEngine->FindSymbol( pToken ));
    if ( pSymbol != NULL ) {
        pEngine->CompileLong( *pSymbol );
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
    long *pSymbol = (long *) (pEngine->FindSymbol( pToken ));
    if ( pSymbol != NULL ) {
        pEngine->CompileLong( OP_INT_VAL );
        pEngine->CompileLong( *pSymbol );
    } else {
        SET_ERROR( kForthErrorUnknownSymbol );
    }
}


//##############################
//
//  output ops
//

FORTHOP( ConOutOpInvoke )
{
    // TBD: hook this so that user can specify string out routine
    // TOS is null terminated string
    char *pStr = (char *) SPOP;
    char *pOutStr = GET_CON_OUT_STRING;
    if ( pOutStr != NULL ) {
        strcat( pOutStr, pStr );
    }
}
static void
stringOut( ForthCoreState   *pCore,
           const char       *buff )
{    
    FILE *pOutFile = GET_CON_OUT_FILE;
    if ( pOutFile != NULL ) {
        fprintf(pOutFile, "%s", buff );
    } else {
        SPUSH( (long) buff );
        ConOutOpInvoke( pCore );
    }
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

    stringOut( pCore, pNext );
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

    stringOut( pCore, buff );
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

    stringOut( pCore, buff );
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

    stringOut( pCore, buff );
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

    stringOut( pCore, buff );
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

    stringOut( pCore, buff );
}

FORTHOP( printStrOp )
{
    NEEDS(1);
    char *buff = (char *) SPOP;
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    stringOut( pCore, buff );
}

FORTHOP( printCharOp )
{
    NEEDS(1);
    char buff[4];
    char c = (char) SPOP;
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %c\n", c );
#endif

    FILE *pOutFile = GET_CON_OUT_FILE;
    if ( pOutFile != NULL ) {
        fputc( c, pOutFile );
    } else {
        buff[0] = c;
        buff[1] = 0;
        SPUSH( (long) buff );
        ConOutOpInvoke( pCore );
    }
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
    SET_CON_OUT_FILE( stdout );
}

FORTHOP( outToFileOp )
{
    NEEDS( 1 );
    SET_CON_OUT_FILE( (FILE *) SPOP );
}

FORTHOP( outToStringOp )
{
    NEEDS( 1 );
    SET_CON_OUT_STRING( (char *) SPOP );
    SET_CON_OUT_FILE( NULL );
}

FORTHOP( getConOutFileOp )
{
    SPUSH( (long) GET_CON_OUT_FILE );
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


FORTHOP( systemOp )
{
    NEEDS(1);
    int result = system( (char *) SPOP );
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

    stringOut( pCore, "stack:" );
    for ( i = 0; i < nItems; i++ ) {
        stringOut( pCore, " " );
        printNumInCurrentBase( pCore, *pSP++ );
    }
    stringOut( pCore, "\n" );
}


FORTHOP( drstackOp )
{
    long *pRP = GET_RP;
    int nItems = GET_RDEPTH;
    int i;

    stringOut( pCore, "rstack:" );
    for ( i = 0; i < nItems; i++ ) {
        stringOut( pCore, " " );
        printNumInCurrentBase( pCore, *pRP++ );
    }
    stringOut( pCore, "\n" );
}


// return true IFF user quit out
static bool
ShowVocab( ForthCoreState   *pCore,
           ForthVocabulary  *pVocab )
{
#define BUFF_SIZE 256
    char buff[BUFF_SIZE];
    int i, len;
    bool retVal = false;
    ForthShell *pShell = GET_ENGINE->GetShell();
    int nEntries = pVocab->GetNumEntries();
    void *pEntry = pVocab->GetFirstEntry();

    for ( i = 0; i < nEntries; i++ ) {
        sprintf( buff, "%02x %06x ", ForthVocabulary::GetEntryType( pEntry ), ForthVocabulary::GetEntryValue( pEntry ) );
        stringOut( pCore, buff );
        len = pVocab->GetEntryNameLength( pEntry );
        if ( len > (BUFF_SIZE - 1)) {
            len = BUFF_SIZE - 1;
        }
        memcpy( buff, pVocab->GetEntryName( pEntry ), len );
        buff[len] = '\0';
        stringOut( pCore, buff );
        stringOut( pCore, "\n" );
        pEntry = pVocab->NextEntry( pEntry );
        if ( ((i % 22) == 21) || (i == (nEntries-1)) ) {
            if ( (pShell != NULL) && pShell->GetInput()->InputStream()->IsInteractive() ) {
                stringOut( pCore, "Hit ENTER to continue, 'q' & ENTER to quit\n" );
                char c = getchar();
                if ( (c == 'q') || (c == 'Q') ) {
                    c = getchar();
                    retVal = true;
                    break;
                }
            }
        }
    }

    return retVal;
}

FORTHOP( vlistOp )
{
    stringOut( pCore, "Definitions Vocabulary:\n" );
    if ( ShowVocab( pCore, GET_ENGINE->GetDefinitionVocabulary() ) == false ) {
        stringOut( pCore, "Precedence Vocabulary:\n" );
        ShowVocab( pCore, GET_ENGINE->GetPrecedenceVocabulary() );
    }
}

FORTHOP( statsOp )
{
    printf( "pCore %p pEngine %p pThread %p     DP %p DBase %p    IP %p\n",
            pCore, pCore->pEngine, pCore->pThread, pCore->DP, pCore->DBase, pCore->IP );
    printf( "SP %p ST %p SLen %d    RP %p RT %p RLen %d\n",
                pCore->SP, pCore->ST, pCore->SLen,
                pCore->RP, pCore->RT, pCore->RLen );
    printf( "%d builtins    %d userops\n", pCore->numBuiltinOps, pCore->numUserOps );
}

FORTHOP( loadLibraryOp )
{
    NEEDS( 1 );
    char* pDLLName = (char *) SPOP;
    HINSTANCE hDLL = LoadLibrary( pDLLName );
    SPUSH( (long) hDLL );
}

FORTHOP( freeLibraryOp )
{
    NEEDS( 1 );
    HINSTANCE hDLL = (HINSTANCE) SPOP;
    FreeLibrary( hDLL );
}

FORTHOP( getProcAddressOp )
{
    NEEDS( 2 );
    char* pProcName = (char *) SPOP;
    HINSTANCE hDLL = (HINSTANCE) SPOP;
    long pFunc = (long) GetProcAddress( hDLL, pProcName );

    SPUSH( pFunc );
}

typedef long (*proc0Args)();
typedef long (*proc1Args)( long arg1 );
typedef long (*proc2Args)( long arg1, long arg2 );
typedef long (*proc3Args)( long arg1, long arg2, long arg3 );
typedef long (*proc4Args)( long arg1, long arg2, long arg3, long arg4 );
typedef long (*proc5Args)( long arg1, long arg2, long arg3, long arg4, long arg5 );
typedef long (*proc6Args)( long arg1, long arg2, long arg3, long arg4, long arg5, long arg6 );
typedef long (*proc7Args)( long arg1, long arg2, long arg3, long arg4, long arg5, long arg6, long arg7 );
typedef long (*proc8Args)( long arg1, long arg2, long arg3, long arg4, long arg5, long arg6, long arg7, long arg8 );

FORTHOP( callProc0Op )
{
    NEEDS( 1 );
    proc0Args pFunc = (proc0Args) SPOP;
    SPUSH( pFunc() );
}

FORTHOP( callProc1Op )
{
    NEEDS( 1 );
    proc1Args pFunc = (proc1Args) SPOP;
    long a1 = SPOP;
    SPUSH( pFunc( a1 ) );
}

FORTHOP( callProc2Op )
{
    NEEDS( 2 );
    proc2Args pFunc = (proc2Args) SPOP;
    long a2 = SPOP;
    long a1 = SPOP;
    SPUSH( pFunc( a1, a2 ) );
}

FORTHOP( callProc3Op )
{
    NEEDS( 3 );
    proc3Args pFunc = (proc3Args) SPOP;
    long a3 = SPOP;
    long a2 = SPOP;
    long a1 = SPOP;
    SPUSH( pFunc( a1, a2, a3 ) );
}

FORTHOP( callProc4Op )
{
    NEEDS( 4 );
    proc4Args pFunc = (proc4Args) SPOP;
    long a4 = SPOP;
    long a3 = SPOP;
    long a2 = SPOP;
    long a1 = SPOP;
    SPUSH( pFunc( a1, a2, a3, a4 ) );
}

FORTHOP( callProc5Op )
{
    NEEDS( 5 );
    proc5Args pFunc = (proc5Args) SPOP;
    long a5 = SPOP;
    long a4 = SPOP;
    long a3 = SPOP;
    long a2 = SPOP;
    long a1 = SPOP;
    SPUSH( pFunc( a1, a2, a3, a4, a5 ) );
}

FORTHOP( callProc6Op )
{
    NEEDS( 6 );
    proc6Args pFunc = (proc6Args) SPOP;
    long a6 = SPOP;
    long a5 = SPOP;
    long a4 = SPOP;
    long a3 = SPOP;
    long a2 = SPOP;
    long a1 = SPOP;
    SPUSH( pFunc( a1, a2, a3, a4, a5, a6 ) );
}

FORTHOP( callProc7Op )
{
    NEEDS( 7 );
    proc7Args pFunc = (proc7Args) SPOP;
    long a7 = SPOP;
    long a6 = SPOP;
    long a5 = SPOP;
    long a4 = SPOP;
    long a3 = SPOP;
    long a2 = SPOP;
    long a1 = SPOP;
    SPUSH( pFunc( a1, a2, a3, a4, a5, a6, a7 ) );
}

FORTHOP( callProc8Op )
{
    NEEDS( 8 );
    proc8Args pFunc = (proc8Args) SPOP;
    long a8 = SPOP;
    long a7 = SPOP;
    long a6 = SPOP;
    long a5 = SPOP;
    long a4 = SPOP;
    long a3 = SPOP;
    long a2 = SPOP;
    long a1 = SPOP;
    SPUSH( pFunc( a1, a2, a3, a4, a5, a6, a7, a8 ) );
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

#define OP( func, funcName )  { funcName, kOpBuiltIn, (ulong) func, 0 }

// ops which have precedence (execute at compile time)
#define PRECOP( func, funcName )  { funcName, ((ulong) kOpBuiltIn) | BASE_DICT_PRECEDENCE_FLAG, (ulong) func, 1 }

// NOTE: the order of the first few entries in this table must agree
// with the list near the top of the file!  (look for COMPILED_OP)

extern GFORTHOP( doIntOp );
extern GFORTHOP( doFloatOp );
extern GFORTHOP( doDoubleOp );
extern GFORTHOP( doStringOp );

baseDictEntry baseDict[] = {
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
    OP(     doIntOp,                "_doInt" ),
    OP(     doFloatOp,              "_doFloat" ),
    OP(     doDoubleOp,             "_doDouble" ),
    OP(     doStringOp,             "_doString" ),
    OP(     intoOp,                 "->" ),
    OP(     doDoOp,                 "_do" ),
    OP(     doLoopOp,               "_loop" ),
    OP(     doLoopNOp,              "_+loop" ),
    OP(     doExitOp,               "_exit" ),      // exit normal op with no vars
    OP(     doExitLOp,              "_exitL" ),     // exit normal op with local vars
    OP(     doExitMOp,              "_exitM" ),     // exit method op with no vars
    OP(     doExitMLOp,             "_exitML" ),    // exit method op with local vars
    OP(     doVocabOp,              "_doVocab" ),
    OP(     plusOp,                 "+" ),

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
    //  single-precision floating point math
    ///////////////////////////////////////////
    OP(     fplusOp,                "f+" ),
    OP(     fminusOp,               "f-" ),
    OP(     ftimesOp,               "f*" ),
    OP(     fdivideOp,              "f/" ),
    
    ///////////////////////////////////////////
    //  double-precision floating point math
    ///////////////////////////////////////////
    OP(     dplusOp,                "d+" ),
    OP(     dminusOp,               "d-" ),
    OP(     dtimesOp,               "d*" ),
    OP(     ddivideOp,              "d/" ),


    ///////////////////////////////////////////
    //  double-precision floating point functions
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
    //  integer/float/double conversions
    ///////////////////////////////////////////
    OP(     i2fOp,                  "i2f" ), 
    OP(     i2dOp,                  "i2d" ),
    OP(     f2iOp,                  "f2i" ),
    OP(     f2dOp,                  "f2d" ),
    OP(     d2iOp,                  "d2i" ),
    OP(     d2fOp,                  "d2f" ),
    
    ///////////////////////////////////////////
    //  control flow ops
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
    //  bit-vector logic ops
    ///////////////////////////////////////////
    OP(     orOp,                   "or" ),
    OP(     andOp,                  "and" ),
    OP(     xorOp,                  "xor" ),
    OP(     invertOp,               "~" ),
    OP(     lshiftOp,               "<<" ),
    OP(     rshiftOp,               ">>" ),

    ///////////////////////////////////////////
    //  boolean ops
    ///////////////////////////////////////////
    OP(     notOp,                  "not" ),
    OP(     trueOp,                 "true" ),
    OP(     falseOp,                "false" ),
    OP(     nullOp,                 "null" ),

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
    
    ///////////////////////////////////////////
    //  stack manipulation
    ///////////////////////////////////////////
    OP(     rpushOp,                "r<" ),
    OP(     rpopOp,                 "r>" ),
    OP(     rdropOp,                "rdrop" ),
    OP(     dupOp,                  "dup" ),
    OP(     swapOp,                 "swap" ),
    OP(     overOp,                 "over" ),
    OP(     rotOp,                  "rot" ),
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
    OP(     fetchOp,                "@" ),
    OP(     cstoreOp,               "c!" ),
    OP(     cfetchOp,               "c@" ),
    OP(     scfetchOp,              "sc@" ),
    OP(     c2lOp,                  "c2l" ),
    OP(     wstoreOp,               "w!" ),
    OP(     wfetchOp,               "w@" ),
    OP(     swfetchOp,              "sw@" ),
    OP(     w2lOp,                  "w2l" ),
    OP(     dstoreOp,               "d!" ),
    OP(     dfetchOp,               "d@" ),
    OP(     memcpyOp,               "memcpy" ),
    OP(     memsetOp,               "memset" ),
    OP(     addToOp,                "->+" ),
    OP(     subtractFromOp,         "->-" ),
    OP(     addressOfOp,            "addressOf" ),
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
    //  defining words
    ///////////////////////////////////////////
    OP(     buildsOp,               "builds" ),
    PRECOP( doesOp,                 "does" ),
    OP(     newestSymbolOp,         "newestSymbol" ),
    PRECOP( exitOp,                 "exit" ),
    PRECOP( semiOp,                 ";" ),
    OP(     colonOp,                ":" ),
    OP(     createOp,               "create" ),
    OP(     forgetOp,               "forget" ),
    OP(     autoforgetOp,           "autoforget" ),
    OP(     definitionsOp,          "definitions" ),
    OP(     usesOp,                 "uses" ),
    OP(     forthVocabOp,           "forth" ),
    OP(     searchVocabOp,          "searchVocab" ),
    OP(     definitionsVocabOp,     "definitionsVocab" ),
    OP(     vocabularyOp,           "vocabulary" ),
    OP(     variableOp,             "variable" ),
    OP(     constantOp,             "constant" ),
    OP(     dconstantOp,            "dconstant" ),
    PRECOP( varsOp,                 "vars" ),
    PRECOP( endvarsOp,              "endvars" ),
    PRECOP( intOp,                  "int" ),
    PRECOP( floatOp,                "float" ),
    PRECOP( doubleOp,               "double" ),
    PRECOP( stringOp,               "string" ),

    PRECOP( recursiveOp,            "recursive" ),
    OP(     precedenceOp,           "precedence" ),
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
    //  text display words
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
    OP(     printFormattedOp,        "%fmt" ),
    OP(     baseOp,                 "base" ),
    OP(     decimalOp,              "decimal" ),
    OP(     hexOp,                  "hex" ),
    OP(     printDecimalSignedOp,   "printDecimalSigned" ),
    OP(     printAllSignedOp,       "printAllSigned" ),
    OP(     printAllUnsignedOp,     "printAllUnsigned" ),
    OP(     outToFileOp,            "outToFile" ),
    OP(     outToScreenOp,          "outToScreen" ),
    OP(     outToStringOp,          "outToString" ),
    OP(     getConOutFileOp,        "getConOutFile" ),

    ///////////////////////////////////////////
    //  file ops
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
    OP(     stdinOp,                "stdin" ),
    OP(     stdoutOp,               "stdout" ),
    OP(     stderrOp,               "stderr" ),
    
    OP(     dstackOp,               "dstack" ),
    OP(     drstackOp,              "drstack" ),
    OP(     vlistOp,                "vlist" ),

    OP(     systemOp,               "system" ),
    OP(     chdirOp,                "chdir" ),
    OP(     byeOp,                  "bye" ),
    OP(     argvOp,                 "argv" ),
    OP(     argcOp,                 "argc" ),

    ///////////////////////////////////////////
    //  DLL support words
    ///////////////////////////////////////////
    OP(     loadLibraryOp,          "loadLibrary" ),
    OP(     freeLibraryOp,          "freeLibrary" ),
    OP(     getProcAddressOp,       "getProcAddress" ),
    OP(     callProc0Op,            "callProc0" ),
    OP(     callProc1Op,            "callProc1" ),
    OP(     callProc2Op,            "callProc2" ),
    OP(     callProc3Op,            "callProc3" ),
    OP(     callProc4Op,            "callProc4" ),
    OP(     callProc5Op,            "callProc5" ),
    OP(     callProc6Op,            "callProc6" ),
    OP(     callProc7Op,            "callProc7" ),
    OP(     callProc8Op,            "callProc8" ),

    ///////////////////////////////////////////
    //  input buffer words
    ///////////////////////////////////////////
    OP(     blwordOp,               "blword" ),
    OP(     wordOp,                 "word" ),
    OP(     getInBufferBaseOp,      "getInBufferBase" ),
    OP(     getInBufferPointerOp,   "getInBufferPointer" ),
    OP(     setInBufferPointerOp,   "setInBufferPointer" ),
    OP(     getInBufferLengthOp,    "getInBufferLength" ),
    OP(     fillInBufferOp,         "fillInBuffer" ),

    OP(     turboOp,                "turbo" ),
    OP(     statsOp,                "stats" ),

    // following must be last in table
    OP(     NULL,                   "" )
};


//############################################################################
//
//  The end of all things...
//
//############################################################################

};  // end extern "C"


