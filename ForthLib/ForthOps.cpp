//////////////////////////////////////////////////////////////////////
//
// ForthOps.cpp: forth builtin operator definitions
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"


#ifdef _WINDOWS
#include <conio.h>
#include <direct.h>
#include <io.h>
#elif defined(_LINUX)
#include <unistd.h>
#endif

#ifdef ARM9
#include <nds.h>
#endif

#include <ctype.h>
#include <time.h>
#if defined(_WINDOWS)
#include <sys\timeb.h>
#endif
#include "Forth.h"
#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthVocabulary.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include "ForthStructs.h"

extern "C"
{

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

FORTHOP( utimesOp )
{
    NEEDS(2);
    unsigned long b = SPOP;
    unsigned long a = SPOP;
    unsigned long long prod = ((unsigned long long) a) * b;
    LPUSH( prod );
}

FORTHOP( times2Op )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a << 1 );
}

FORTHOP( times4Op )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a << 2 );
}

FORTHOP( times8Op )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a << 3 );
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
    SPUSH( a >> 2 );
}

FORTHOP( divide8Op )
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( a >> 3 );
}

FORTHOP( divmodOp )
{
    NEEDS(2);
    ldiv_t v;
    long b = SPOP;
    long a = SPOP;
#if defined(_WINDOWS)
    v = div( a, b );
#elif defined(_LINUX)
    v = ldiv( a, b );
#endif
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

FORTHOP( lplusOp )
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    LPUSH( a + b );
}

FORTHOP( lminusOp )
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    LPUSH( a - b );
}

FORTHOP( ltimesOp )
{
    NEEDS(4);
    long long b = LPOP;
    long long a = LPOP;
    LPUSH( a * b );
}

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
    LPUSH( a / b );
    LPUSH( a % b );
}

FORTHOP( lnegateOp )
{
    NEEDS(2);
    long long a = LPOP;
    LPUSH( -a );
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

FORTHOP( f2iOp )
{
    NEEDS(1);
    int a = (int) FPOP;
    SPUSH( a );
}

FORTHOP( f2lOp )
{
    NEEDS(1);
    float a = FPOP;
    long long b = (long long) a;
    LPUSH( b );
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

FORTHOP( d2lOp )
{
    NEEDS(2);
    double a = DPOP;
    long long b = (long long) a;
    LPUSH( b );
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
    if ( GET_RDEPTH < 1 )
    {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    }
    else
    {
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
    if ( GET_RDEPTH < 1 )
    {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    }
    else
    {
        SET_IP( (long *) RPOP );
    }
}

// exit method op with no local vars
FORTHOP(doExitMOp)
{
    // rstack: oldIP oldTP
    if ( GET_RDEPTH < 3 )
    {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    }
    else
    {
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
    if ( GET_RDEPTH < 3 )
    {
        SET_ERROR( kForthErrorReturnStackUnderflow );
    }
    else
    {
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

    if ( newIndex >= pRP[1] )
    {
        // loop has ended, drop end, current indices, loopIP
        SET_RP( pRP + 3 );
    }
    else
    {
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
    if ( done )
    {
        // loop has ended, drop end, current indices, loopIP
        SET_RP( pRP + 3 );
    }
    else
    {
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

    if ( ((GET_DP) - (long *)(pShellStack->Peek())) == 1 )
    {
        // there is no default case, we must compile a "drop" to
        //   dispose of the case selector on TOS
        pEngine->CompileOpcode( OP_DROP );
    }
    // patch branches from end-of-case to common exit point
    while ( (pEndofOp = (long *) (pShellStack->Pop())) != NULL )
    {
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


FORTHOP( urshiftOp )
{
    NEEDS(2);
    long b = SPOP;
    unsigned long a = (unsigned long) (SPOP);
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
// integer comparison ops
//

FORTHOP(equalsOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(notEqualsOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(greaterThanOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(greaterEqualsOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(lessThanOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(lessEqualsOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(equalsZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a == 0 ) ? -1L : 0 );
}

FORTHOP(notEqualsZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a != 0 ) ? -1L : 0 );
}

FORTHOP(greaterThanZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a > 0 ) ? -1L : 0 );
}

FORTHOP(greaterEqualsZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a >= 0 ) ? -1L : 0 );
}

FORTHOP(lessThanZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a < 0 ) ? -1L : 0 );
}

FORTHOP(lessEqualsZeroOp)
{
    NEEDS(1);
    long a = SPOP;
    SPUSH( ( a <= 0 ) ? -1L : 0 );
}

FORTHOP(unsignedGreaterThanOp)
{
    NEEDS(2);
    ulong b = (ulong) SPOP;
    ulong a = (ulong) SPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(unsignedLessThanOp)
{
    NEEDS(2);
    ulong b = (ulong) SPOP;
    ulong a = (ulong) SPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(withinOp)
{
    NEEDS(3);
    long hiLimit = SPOP;
    long loLimit = SPOP;
    long val = SPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(minOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a < b ) ? a : b );
}

FORTHOP(maxOp)
{
    NEEDS(2);
    long b = SPOP;
    long a = SPOP;
    SPUSH( ( a > b ) ? a : b );
}


//##############################
//
// single-precision fp comparison ops
//

FORTHOP(fEqualsOp)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(fNotEqualsOp)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(fGreaterThanOp)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(fGreaterEqualsOp)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(fLessThanOp)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(fLessEqualsOp)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(fEqualsZeroOp)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a == 0.0f ) ? -1L : 0 );
}

FORTHOP(fNotEqualsZeroOp)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a != 0.0f ) ? -1L : 0 );
}

FORTHOP(fGreaterThanZeroOp)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a > 0.0f ) ? -1L : 0 );
}

FORTHOP(fGreaterEqualsZeroOp)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a >= 0.0f ) ? -1L : 0 );
}

FORTHOP(fLessThanZeroOp)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a < 0.0f ) ? -1L : 0 );
}

FORTHOP(fLessEqualsZeroOp)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a <= 0.0f ) ? -1L : 0 );
}

FORTHOP(fWithinOp)
{
    NEEDS(3);
    float hiLimit = FPOP;
    float loLimit = FPOP;
    float val = FPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(fMinOp)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( ( a < b ) ? a : b );
}

FORTHOP(fMaxOp)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( ( a > b ) ? a : b );
}

//##############################
//
// double-precision fp comparison ops
//

FORTHOP(dEqualsOp)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(dNotEqualsOp)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(dGreaterThanOp)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(dGreaterEqualsOp)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(dLessThanOp)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(dLessEqualsOp)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(dEqualsZeroOp)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a == 0.0 ) ? -1L : 0 );
}

FORTHOP(dNotEqualsZeroOp)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a != 0.0 ) ? -1L : 0 );
}

FORTHOP(dGreaterThanZeroOp)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a > 0.0 ) ? -1L : 0 );
}

FORTHOP(dGreaterEqualsZeroOp)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a >= 0.0 ) ? -1L : 0 );
}

FORTHOP(dLessThanZeroOp)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a < 0.0 ) ? -1L : 0 );
}

FORTHOP(dLessEqualsZeroOp)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a <= 0.0 ) ? -1L : 0 );
}

FORTHOP(dWithinOp)
{
    NEEDS(3);
    double hiLimit = DPOP;
    double loLimit = DPOP;
    double val = DPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(dMinOp)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    DPUSH( ( a < b ) ? a : b );
}

FORTHOP(dMaxOp)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    DPUSH( ( a > b ) ? a : b );
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

FORTHOP(dupNonZeroOp)
{
    NEEDS(1);
    long a = *(GET_SP);
    if ( a != 0 )
    {
        SPUSH( a );
    }
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

FORTHOP(reverseRotOp)
{
    NEEDS(3);
    int a, b, c;
    long *pSP = GET_SP;
    a = pSP[2];
    b = pSP[1];
    c = pSP[0];
    pSP[2] = c;
    pSP[1] = a;
    pSP[0] = b;
}

FORTHOP(nipOp)
{
    NEEDS(2);
    long a = SPOP;
    SPOP;
    SPUSH( a );
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

FORTHOP(startTupleOp)
{
    long pSP = (long) (GET_SP);
    RPUSH( pSP );
}

FORTHOP(endTupleOp)
{
    long* pSP = (GET_SP);
    long* pOldSP = (long *) (RPOP);
    long count = pOldSP - pSP;
    SPUSH( count );
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

FORTHOP( vocabToClassOp )
{
    SET_VAR_OPERATION( kVocabGetClass );
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
	int result = strcmp( pStr1, pStr2 );
	// only return 1, 0 or -1
	if ( result != 0 )
	{
		result = (result > 0) ? 1 : -1;
	}
	SPUSH( (long) result );
}

FORTHOP( stricmpOp )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
#if defined(_WINDOWS)
	int result = stricmp( pStr1, pStr2 );
#elif defined(_LINUX)
	int result = strcasecmp( pStr1, pStr2 );
#endif
	// only return 1, 0 or -1
	if ( result != 0 )
	{
		result = (result > 0) ? 1 : -1;
	}
	SPUSH( (long) result );
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

    switch ( flags & (kEngineFlagHasLocalVars | kEngineFlagIsMethod) )
    {
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
    *pEntry = COMPILED_OP( kOpUserCode, FORTH_OP_VALUE( newestOp ) );
}

FORTHOP( createOp )
{
    ForthEngine *pEngine = GET_ENGINE;
    // get next symbol, add it to vocabulary with type "user op"
    long* pEntry = pEngine->StartOpDefinition( NULL, false );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
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
    char* pVocabName = pEngine->GetNextSimpleToken();
    long* pEntry = pEngine->StartOpDefinition( pVocabName );
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
    pEngine->CompileOpcode( OP_DO_VOCAB );
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
        quit = ( ShowVocab( pCore, pVocab, false ) == 'q' );
        depth++;
    }
}

FORTHOP( vlistqOp )
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
        quit = ( ShowVocab( pCore, pVocab, true ) == 'q' );
        depth++;
    }
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
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
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
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
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
    int val = OP_BAD_OP;
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
    // IP points to data field
    ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (*pCore->IP++);
    SPUSH( (long) pClassVocab );
    ForthEngine *pEngine = GET_ENGINE;
    pEngine->ExecuteOneOp( pClassVocab->GetClassObject()->newOp );
}

FORTHOP( allocObjectOp )
{
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
    long* pEntry = pEngine->StartOpDefinition();
    pEntry[1] = BASE_TYPE_TO_CODE( kBaseTypeUserDefinition );
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
    
    if ( pEntry )
    {
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
    }
    else
    {
        CONSOLE_STRING_OUT( "!!!! Failure finding symbol " );
        CONSOLE_STRING_OUT( pSym );
        TRACE( "!!!! Failure finding symbol %s !!!!\n", pSym );
    }
}

FORTHOP( loadStrOp )
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
    loadStrOp( pCore );
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
    }
}

FORTHOP( executeOp )
{
	long op = SPOP;
    ForthEngine *pEngine = GET_ENGINE;
	pEngine->ExecuteOneOp( op );
#if 0
    long prog[2];
    long *oldIP = GET_IP;

	// execute the opcode
    prog[0] = SPOP;
    prog[1] = BUILTIN_OP( OP_DONE );
    
    SET_IP( prog );
    InnerInterpreter( pCore );
    SET_IP( oldIP );
#endif
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
        pEngine->CompileOpcode( OP_INT_VAL );
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
#if defined(_WINDOWS)
                v = div( val, base );
#elif defined(_LINUX)
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
    ldiv_t v;
    ulong base;
    bool bIsNegative, bPrintUnsigned;
    ulong urem;
	unsigned long long uval;
	unsigned long long quotient;
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

#ifdef _WINDOWS
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
FORTHOP( fopenOp )
{
    NEEDS(2);
    char *pAccess = (char *) SPOP;
    char *pName = (char *) SPOP;

    FILE *pFP = pCore->pFileFuncs->fileOpen( pName, pAccess );
    SPUSH( (long) pFP );
}

FORTHOP( fcloseOp )
{
    NEEDS(1);
    
    int result = pCore->pFileFuncs->fileClose( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( fseekOp )
{
    int ctrl = SPOP;
    int offset = SPOP;
    FILE *pFP = (FILE *) SPOP;
    
    int result = pCore->pFileFuncs->fileSeek( pFP, offset, ctrl );
    SPUSH( result );
}

FORTHOP( freadOp )
{
    NEEDS(4);
    
    FILE *pFP = (FILE *) SPOP;
    int numItems = SPOP;
    int itemSize = SPOP;
    void *pDst = (void *) SPOP;
    
    int result = pCore->pFileFuncs->fileRead( pDst, itemSize, numItems, pFP );
    SPUSH( result );
}

FORTHOP( fwriteOp )
{
    
    NEEDS(4);
    FILE *pFP = (FILE *) SPOP;
    int numItems = SPOP;
    int itemSize = SPOP;
    void *pSrc = (void *) SPOP;
    
    int result = pCore->pFileFuncs->fileWrite( pSrc, itemSize, numItems, pFP );
    SPUSH( result );
}

FORTHOP( fgetcOp )
{    
    NEEDS(1);
    FILE *pFP = (FILE *) SPOP;
    
    int result = pCore->pFileFuncs->fileGetChar( pFP );
    SPUSH( result );
}

FORTHOP( fputcOp )
{    
    NEEDS(2);
    FILE *pFP = (FILE *) SPOP;
    int outChar = SPOP;
    
    int result = pCore->pFileFuncs->filePutChar( outChar, pFP );
    SPUSH( result );
}

FORTHOP( feofOp )
{
    NEEDS(1);

    int result = pCore->pFileFuncs->fileAtEnd( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( fexistsOp )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->fileExists( (const char*) SPOP );
    SPUSH( result );
}

FORTHOP( ftellOp )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->fileTell( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( flenOp )
{
    NEEDS(1);
    FILE* pFile = (FILE *) SPOP;
    int result = pCore->pFileFuncs->fileGetLength( pFile );
    SPUSH( result );
}

FORTHOP( fgetsOp )
{
    NEEDS( 3 );
    FILE* pFile = (FILE *) SPOP;
    int maxChars = SPOP;
    char* pBuffer = (char *) SPOP;
    int result = (int) pCore->pFileFuncs->fileGetString( pBuffer, maxChars, pFile );
    SPUSH( result );
}

FORTHOP( fputsOp )
{
    NEEDS( 2 );
    FILE* pFile = (FILE *) SPOP;
    char* pBuffer = (char *) SPOP;
    int result = pCore->pFileFuncs->filePutString( pBuffer, pFile );
    SPUSH( result );
}


FORTHOP( removeOp )
{
	const char* pFilename = (const char *) (SPOP);
	int result = remove( pFilename );
    SPUSH( result );
}

FORTHOP( _dupOp )
{
	int fileHandle = SPOP;
#if defined( _WINDOWS )
	int result = _dup( fileHandle );
#else
	int result = dup( fileHandle );
#endif
    SPUSH( result );
}

FORTHOP( _dup2Op )
{
	int dstFileHandle = SPOP;
	int srcFileHandle = SPOP;
#if defined( _WINDOWS )
	int result = _dup2( srcFileHandle, dstFileHandle );
#else
	int result = dup2( srcFileHandle, dstFileHandle );
#endif
    SPUSH( result );
}

FORTHOP( _filenoOp )
{
	FILE* pFile = (FILE *) SPOP;
#if defined( _WINDOWS )
	int result = _fileno( pFile );
#else
	int result = fileno( pFile );
#endif
    SPUSH( result );
}

FORTHOP( tmpnamOp )
{
	char* pOutname = (char *) malloc( L_tmpnam );
    ForthEngine *pEngine = GET_ENGINE;
	if ( tmpnam( pOutname ) == NULL )
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
	int result = fflush( pFile );
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
            char quit = ShowVocab( pEngine->GetCoreState(), pVocab, true );
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

#ifdef _WINDOWS

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

#ifdef _WINDOWS
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

FORTHOP( sleepOp )
{
    DWORD dwMilliseconds = (DWORD)(SPOP);
    ::Sleep( dwMilliseconds );
}

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

baseDictionaryEntry baseDictionary[] =
{
    ///////////////////////////////////////////
    //  STUFF WHICH IS COMPILED BY OTHER WORDS
    //   DO NOT REARRANGE UNDER PAIN OF DEATH
    ///////////////////////////////////////////
    OP_DEF(    abortOp,                "abort" ),           // 0
    OP_DEF(    dropOp,                 "drop" ),
    OP_DEF(    doDoesOp,               "_doDoes" ),
    OP_DEF(    litOp,                  "lit" ),
    OP_DEF(    litOp,                  "flit" ),            // 4
    OP_DEF(    dlitOp,                 "dlit" ),
    OP_DEF(    doVariableOp,           "_doVariable" ),
    OP_DEF(    doConstantOp,           "_doConstant" ),
    OP_DEF(    doDConstantOp,          "_doDConstant" ),    // 8
    OP_DEF(    endBuildsOp,            "_endBuilds" ),
    OP_DEF(    doneOp,                 "done" ),
    OP_DEF(    doByteOp,               "_doByte" ),
    OP_DEF(    doUByteOp,              "_doUByte" ),
    OP_DEF(    doShortOp,              "_doShort" ),        // 12
    OP_DEF(    doUShortOp,             "_doUShort" ),
    OP_DEF(    doIntOp,                "_doInt" ),
    OP_DEF(    doIntOp,                "_doUInt" ),
    OP_DEF(    doLongOp,               "_doLong" ),
    OP_DEF(    doLongOp,               "_doULong" ),
    OP_DEF(    doFloatOp,              "_doFloat" ),
    OP_DEF(    doDoubleOp,             "_doDouble" ),
    OP_DEF(    doStringOp,             "_doString" ),       // 16
    OP_DEF(    doOpOp,                 "_doOp" ),
    OP_DEF(    doObjectOp,             "_doObject" ),
    OP_DEF(    doExitOp,               "_exit" ),      // exit normal op with no vars
    OP_DEF(    doExitLOp,              "_exitL" ),     // exit normal op with local vars
    OP_DEF(    doExitMOp,              "_exitM" ),     // exit method op with no vars
    OP_DEF(    doExitMLOp,             "_exitML" ),    // exit method op with local vars
    OP_DEF(    doVocabOp,              "_doVocab" ),
    OP_DEF(    doByteArrayOp,          "_doByteArray" ),
    OP_DEF(    doUByteArrayOp,         "_doUByteArray" ),
    OP_DEF(    doShortArrayOp,         "_doShortArray" ),
    OP_DEF(    doUShortArrayOp,        "_doUShortArray" ),
    OP_DEF(    doIntArrayOp,           "_doIntArray" ),
    OP_DEF(    doIntArrayOp,           "_doUIntArray" ),
    OP_DEF(    doLongArrayOp,          "_doLongArray" ),
    OP_DEF(    doLongArrayOp,          "_doULongArray" ),
    OP_DEF(    doFloatArrayOp,         "_doFloatArray" ),
    OP_DEF(    doDoubleArrayOp,        "_doDoubleArray" ),
    OP_DEF(    doStringArrayOp,        "_doStringArray" ),
    OP_DEF(    doOpArrayOp,            "_doOpArray" ),
    OP_DEF(    doObjectArrayOp,        "_doObjectArray" ),
    OP_DEF(    initStringOp,           "initString" ),
    OP_DEF(    initStringArrayOp,      "initStringArray" ),
    OP_DEF(    plusOp,                 "+" ),
    OP_DEF(    fetchOp,                "@" ),
    OP_DEF(    badOpOp,                "badOp" ),
    OP_DEF(    doStructOp,             "_doStruct" ),
    OP_DEF(    doStructArrayOp,        "_doStructArray" ),
    OP_DEF(    doStructTypeOp,         "_doStructType" ),
    OP_DEF(    doClassTypeOp,          "_doClassType" ),
    PRECOP_DEF(doEnumOp,               "_doEnum" ),
    OP_DEF(    doDoOp,                 "_do" ),
    OP_DEF(    doLoopOp,               "_loop" ),
    OP_DEF(    doLoopNOp,              "_+loop" ),
    OP_DEF(    doNewOp,                "_doNew" ),
    OP_DEF(    dfetchOp,               "d@" ),
    OP_DEF(    allocObjectOp,          "_allocObject" ),
    OP_DEF(    vocabToClassOp,         "vocabToClass" ),
    // the order of the next four opcodes has to match the order of kVarRef...kVarMinusStore
    OP_DEF(    addressOfOp,            "ref" ),
    OP_DEF(    intoOp,                 "->" ),
    OP_DEF(    addToOp,                "->+" ),
    OP_DEF(    subtractFromOp,         "->-" ),

    // stuff below this line can be rearranged
    OP_DEF(    thisOp,                 "this" ),
    OP_DEF(    thisDataOp,             "thisData" ),
    OP_DEF(    thisMethodsOp,          "thisMethods" ),
    OP_DEF(    executeOp,              "execute" ),
    OP_DEF(    callOp,                 "call" ),
    OP_DEF(    gotoOp,                 "goto" ),
    OP_DEF(    iOp,                    "i" ),
    OP_DEF(    jOp,                    "j" ),
    OP_DEF(    unloopOp,               "unloop" ),
    OP_DEF(    leaveOp,                "leave" ),
    OP_DEF(    hereOp,                 "here" ),
    // vocabulary varActions
    OP_DEF(    addressOfOp,            "getNewest" ),
    OP_DEF(    intoOp,                 "findEntry" ),
    OP_DEF(    addToOp,                "findEntryValue" ),
    OP_DEF(    subtractFromOp,         "addEntry" ),
    OP_DEF(    removeEntryOp,          "removeEntry" ),
    OP_DEF(    entryLengthOp,          "entryLength" ),
    OP_DEF(    numEntriesOp,           "numEntries" ),
    
    ///////////////////////////////////////////
    //  integer math
    ///////////////////////////////////////////
    OP_DEF(    minusOp,                "-" ),
    OP_DEF(    timesOp,                "*" ),
    OP_DEF(    utimesOp,               "u*" ),
    OP_DEF(    times2Op,               "2*" ),
    OP_DEF(    times4Op,               "4*" ),
    OP_DEF(    times8Op,               "8*" ),
    OP_DEF(    divideOp,               "/" ),
    OP_DEF(    divide2Op,              "2/" ),
    OP_DEF(    divide4Op,              "4/" ),
    OP_DEF(    divide8Op,              "8/" ),
    OP_DEF(    divmodOp,               "/mod" ),
    OP_DEF(    modOp,                  "mod" ),
    OP_DEF(    negateOp,               "negate" ),
    
    ///////////////////////////////////////////
    //  single-precision fp math
    ///////////////////////////////////////////
    OP_DEF(    fplusOp,                "f+" ),
    OP_DEF(    fminusOp,               "f-" ),
    OP_DEF(    ftimesOp,               "f*" ),
    OP_DEF(    fdivideOp,              "f/" ),
    
    ///////////////////////////////////////////
    //  single-precision fp comparisons
    ///////////////////////////////////////////
    OP_DEF(    fEqualsOp,               "f==" ),
    OP_DEF(    fNotEqualsOp,            "f!=" ),
    OP_DEF(    fGreaterThanOp,          "f>" ),
    OP_DEF(    fGreaterEqualsOp,        "f>=" ),
    OP_DEF(    fLessThanOp,             "f<" ),
    OP_DEF(    fLessEqualsOp,           "f<=" ),
    OP_DEF(    fEqualsZeroOp,           "f0==" ),
    OP_DEF(    fNotEqualsZeroOp,        "f0!=" ),
    OP_DEF(    fGreaterThanZeroOp,      "f0>" ),
    OP_DEF(    fGreaterEqualsZeroOp,    "f0>=" ),
    OP_DEF(    fLessThanZeroOp,         "f0<" ),
    OP_DEF(    fLessEqualsZeroOp,       "f0<=" ),
    OP_DEF(    fWithinOp,               "fwithin" ),
    OP_DEF(    fMinOp,                  "fmin" ),
    OP_DEF(    fMaxOp,                  "fmax" ),

    ///////////////////////////////////////////
    //  double-precision fp math
    ///////////////////////////////////////////
    OP_DEF(    dplusOp,                "d+" ),
    OP_DEF(    dminusOp,               "d-" ),
    OP_DEF(    dtimesOp,               "d*" ),
    OP_DEF(    ddivideOp,              "d/" ),


    ///////////////////////////////////////////
    //  double-precision fp comparisons
    ///////////////////////////////////////////
    OP_DEF(    dEqualsOp,               "d==" ),
    OP_DEF(    dNotEqualsOp,            "d!=" ),
    OP_DEF(    dGreaterThanOp,          "d>" ),
    OP_DEF(    dGreaterEqualsOp,        "d>=" ),
    OP_DEF(    dLessThanOp,             "d<" ),
    OP_DEF(    dLessEqualsOp,           "d<=" ),
    OP_DEF(    dEqualsZeroOp,           "d0==" ),
    OP_DEF(    dNotEqualsZeroOp,        "d0!=" ),
    OP_DEF(    dGreaterThanZeroOp,      "d0>" ),
    OP_DEF(    dGreaterEqualsZeroOp,    "d0>=" ),
    OP_DEF(    dLessThanZeroOp,         "d0<" ),
    OP_DEF(    dLessEqualsZeroOp,       "d0<=" ),
    OP_DEF(    dWithinOp,               "dwithin" ),
    OP_DEF(    dMinOp,                  "dmin" ),
    OP_DEF(    dMaxOp,                  "dmax" ),

    ///////////////////////////////////////////
    //  double-precision fp functions
    ///////////////////////////////////////////
    OP_DEF(    dsinOp,                 "dsin" ),
    OP_DEF(    dasinOp,                "darcsin" ),
    OP_DEF(    dcosOp,                 "dcos" ),
    OP_DEF(    dacosOp,                "darccos" ),
    OP_DEF(    dtanOp,                 "dtan" ),
    OP_DEF(    datanOp,                "darctan" ),
    OP_DEF(    datan2Op,               "darctan2" ),
    OP_DEF(    dexpOp,                 "dexp" ),
    OP_DEF(    dlnOp,                  "dln" ),
    OP_DEF(    dlog10Op,               "dlog10" ),
    OP_DEF(    dpowOp,                 "dpow" ),
    OP_DEF(    dsqrtOp,                "dsqrt" ),
    OP_DEF(    dceilOp,                "dceil" ),
    OP_DEF(    dfloorOp,               "dfloor" ),
    OP_DEF(    dabsOp,                 "dabs" ),
    OP_DEF(    dldexpOp,               "dldexp" ),
    OP_DEF(    dfrexpOp,               "dfrexp" ),
    OP_DEF(    dmodfOp,                "dmodf" ),
    OP_DEF(    dfmodOp,                "dfmod" ),
    
    ///////////////////////////////////////////
    //  integer/long/float/double conversion
    ///////////////////////////////////////////
    OP_DEF(    i2fOp,                  "i2f" ), 
    OP_DEF(    i2dOp,                  "i2d" ),
    OP_DEF(    f2iOp,                  "f2i" ),
    OP_DEF(    f2dOp,                  "f2d" ),
    OP_DEF(    d2iOp,                  "d2i" ),
    OP_DEF(    d2fOp,                  "d2f" ),
    
    ///////////////////////////////////////////
    //  bit-vector logic
    ///////////////////////////////////////////
    OP_DEF(    orOp,                   "or" ),
    OP_DEF(    andOp,                  "and" ),
    OP_DEF(    xorOp,                  "xor" ),
    OP_DEF(    invertOp,               "~" ),
    OP_DEF(    lshiftOp,               "<<" ),
    OP_DEF(    rshiftOp,               ">>" ),
    OP_DEF(    urshiftOp,              "u>>" ),

    ///////////////////////////////////////////
    //  boolean logic
    ///////////////////////////////////////////
    OP_DEF(    notOp,                  "not" ),
    OP_DEF(    trueOp,                 "true" ),
    OP_DEF(    falseOp,                "false" ),
    OP_DEF(    nullOp,                 "null" ),
    OP_DEF(    dnullOp,                "dnull" ),

    ///////////////////////////////////////////
    //  integer comparisons
    ///////////////////////////////////////////
    OP_DEF(    equalsOp,               "==" ),
    OP_DEF(    notEqualsOp,            "!=" ),
    OP_DEF(    greaterThanOp,          ">" ),
    OP_DEF(    greaterEqualsOp,        ">=" ),
    OP_DEF(    lessThanOp,             "<" ),
    OP_DEF(    lessEqualsOp,           "<=" ),
    OP_DEF(    equalsZeroOp,           "0==" ),
    OP_DEF(    notEqualsZeroOp,        "0!=" ),
    OP_DEF(    greaterThanZeroOp,      "0>" ),
    OP_DEF(    greaterEqualsZeroOp,    "0>=" ),
    OP_DEF(    lessThanZeroOp,         "0<" ),
    OP_DEF(    lessEqualsZeroOp,       "0<=" ),
    OP_DEF(    unsignedGreaterThanOp,  "u>" ),
    OP_DEF(    unsignedLessThanOp,     "u<" ),
    OP_DEF(    withinOp,               "within" ),
    OP_DEF(    minOp,                  "min" ),
    OP_DEF(    maxOp,                  "max" ),
    
    ///////////////////////////////////////////
    //  stack manipulation
    ///////////////////////////////////////////
    OP_DEF(    rpushOp,                "r<" ),
    OP_DEF(    rpopOp,                 "r>" ),
    OP_DEF(    rdropOp,                "rdrop" ),
    OP_DEF(    rpOp,                   "rp" ),
    OP_DEF(    rzeroOp,                "r0" ),
    OP_DEF(    dupOp,                  "dup" ),
    OP_DEF(    dupNonZeroOp,           "?dup" ),
    OP_DEF(    swapOp,                 "swap" ),
    OP_DEF(    overOp,                 "over" ),
    OP_DEF(    rotOp,                  "rot" ),
    OP_DEF(    reverseRotOp,           "-rot" ),
    OP_DEF(    nipOp,                  "nip" ),
    OP_DEF(    tuckOp,                 "tuck" ),
    OP_DEF(    pickOp,                 "pick" ),
    OP_DEF(    rollOp,                 "roll" ),
    OP_DEF(    spOp,                   "sp" ),
    OP_DEF(    szeroOp,                "s0" ),
    OP_DEF(    fpOp,                   "fp" ),
    OP_DEF(    ddupOp,                 "ddup" ),
    OP_DEF(    dswapOp,                "dswap" ),
    OP_DEF(    ddropOp,                "ddrop" ),
    OP_DEF(    doverOp,                "dover" ),
    OP_DEF(    drotOp,                 "drot" ),
    OP_DEF(    startTupleOp,           "r[" ),
    OP_DEF(    endTupleOp,             "]r" ),

    ///////////////////////////////////////////
    //  memory store/fetch
    ///////////////////////////////////////////
    OP_DEF(    storeOp,                "!" ),
    OP_DEF(    cstoreOp,               "c!" ),
    OP_DEF(    cfetchOp,               "c@" ),
    OP_DEF(    scfetchOp,              "sc@" ),
    OP_DEF(    c2lOp,                  "c2l" ),
    OP_DEF(    wstoreOp,               "w!" ),
    OP_DEF(    wfetchOp,               "w@" ),
    OP_DEF(    swfetchOp,              "sw@" ),
    OP_DEF(    w2lOp,                  "w2l" ),
    OP_DEF(    dstoreOp,               "d!" ),
    OP_DEF(    memcpyOp,               "memcpy" ),
    OP_DEF(    memsetOp,               "memset" ),
    OP_DEF(    setVarActionOp,         "varAction!" ),
    OP_DEF(    getVarActionOp,         "varAction@" ),

    ///////////////////////////////////////////
    //  string manipulation
    ///////////////////////////////////////////
    OP_DEF(    strcpyOp,               "strcpy" ),
    OP_DEF(    strncpyOp,              "strncpy" ),
    OP_DEF(    strlenOp,               "strlen" ),
    OP_DEF(    strcatOp,               "strcat" ),
    OP_DEF(    strncatOp,              "strncat" ),
    OP_DEF(    strchrOp,               "strchr" ),
    OP_DEF(    strrchrOp,              "strrchr" ),
    OP_DEF(    strcmpOp,               "strcmp" ),
    OP_DEF(    stricmpOp,              "stricmp" ),
    OP_DEF(    strstrOp,               "strstr" ),
    OP_DEF(    strtokOp,               "strtok" ),

    ///////////////////////////////////////////
    //  file manipulation
    ///////////////////////////////////////////
    OP_DEF(    fopenOp,                "fopen" ),
    OP_DEF(    fcloseOp,               "fclose" ),
    OP_DEF(    fseekOp,                "fseek" ),
    OP_DEF(    freadOp,                "fread" ),
    OP_DEF(    fwriteOp,               "fwrite" ),
    OP_DEF(    fgetcOp,                "fgetc" ),
    OP_DEF(    fputcOp,                "fputc" ),
    OP_DEF(    feofOp,                 "feof" ),
    OP_DEF(    fexistsOp,              "fexists" ),
    OP_DEF(    ftellOp,                "ftell" ),
    OP_DEF(    flenOp,                 "flen" ),
    OP_DEF(    fgetsOp,                "fgets" ),
    OP_DEF(    fputsOp,                "fputs" ),

    // everything below this line does not have an assembler version

    OP_DEF(    stdinOp,                "stdin" ),
    OP_DEF(    stdoutOp,               "stdout" ),
    OP_DEF(    stderrOp,               "stderr" ),
    
    ///////////////////////////////////////////
    //  64-bit integer math & conversions
    ///////////////////////////////////////////
    OP_DEF(    lplusOp,                "l+" ),
    OP_DEF(    lminusOp,               "l-" ),
    OP_DEF(    ltimesOp,               "l*" ),
    OP_DEF(    ldivideOp,              "l/" ),
    OP_DEF(    lmodOp,                 "lmod" ),
    OP_DEF(    ldivmodOp,              "l/mod" ),
    OP_DEF(    lnegateOp,              "lnegate" ),
    OP_DEF(    i2lOp,                  "i2l" ), 
    OP_DEF(    l2fOp,                  "l2f" ), 
    OP_DEF(    l2dOp,                  "l2d" ), 
    OP_DEF(    f2lOp,                  "f2l" ),
    OP_DEF(    d2lOp,                  "d2l" ),
    
    ///////////////////////////////////////////
    //  64-bit integer comparisons
    ///////////////////////////////////////////
    OP_DEF(    lEqualsOp,               "l==" ),
    OP_DEF(    lNotEqualsOp,            "l!=" ),
    OP_DEF(    lGreaterThanOp,          "l>" ),
    OP_DEF(    lGreaterEqualsOp,        "l>=" ),
    OP_DEF(    lLessThanOp,             "l<" ),
    OP_DEF(    lLessEqualsOp,           "l<=" ),
    OP_DEF(    lEqualsZeroOp,           "l0==" ),
    OP_DEF(    lNotEqualsZeroOp,        "l0!=" ),
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
    OP_DEF(    loadStrOp,              "load$" ),
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
    OP_DEF(    vlistqOp,               "vlistq" ),
    OP_DEF(    findOp,                 "find" ),

    ///////////////////////////////////////////
    //  data compilation/allocation
    ///////////////////////////////////////////
    OP_DEF(    alignOp,                "align" ),
    OP_DEF(    allotOp,                "allot" ),
    OP_DEF(    callotOp,               "callot" ),
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
#ifdef _WINDOWS
    OP_DEF(    DLLVocabularyOp,        "DLLVocabulary" ),
    OP_DEF(    addDLLEntryOp,          "addDLLEntry" ),
#endif

    ///////////////////////////////////////////
    //  time and date
    ///////////////////////////////////////////
    OP_DEF(    timeOp,                 "time" ),
    OP_DEF(    strftimeOp,             "strftime" ),
    OP_DEF(    millitimeOp,            "millitime" ),

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
    OP_DEF(    chdirOp,                "chdir" ),
    OP_DEF(    fflushOp,               "fflush" ),
    OP_DEF(    byeOp,                  "bye" ),
    OP_DEF(    argvOp,                 "argv" ),
    OP_DEF(    argcOp,                 "argc" ),
    OP_DEF(    turboOp,                "turbo" ),
    OP_DEF(    statsOp,                "stats" ),
    OP_DEF(    describeOp,             "describe" ),
    OP_DEF(    errorOp,                "error" ),
    OP_DEF(    addErrorTextOp,         "addErrorText" ),
	OP_DEF(    setTraceOp,             "setTrace" ),

    ///////////////////////////////////////////
    //  conditional compilation
    ///////////////////////////////////////////
    PRECOP_DEF( poundIfOp,              "#if" ),
    PRECOP_DEF( poundIfdefOp,           "#ifdef" ),
    PRECOP_DEF( poundIfndefOp,          "#ifndef" ),
    PRECOP_DEF( poundElseOp,            "#else" ),
    PRECOP_DEF( poundEndifOp,           "#endif" ),

#ifdef _WINDOWS
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
    OP_DEF( sleepOp,                    "Sleep" ),
    OP_DEF( createThreadOp,             "createThread" ),
    OP_DEF( destroyThreadOp,            "destroyThread" ),
    OP_DEF( startThreadOp,              "startThread" ),
    OP_DEF( exitThreadOp,               "exitThread" ),
    OP_DEF( findFirstFileOp,            "FindFirstFile" ),
    OP_DEF( findNextFileOp,             "FindNextFile" ),
    OP_DEF( findCloseOp,                "FindClose" ),
    OP_DEF( windowsConstantsOp,         "windowsConstants" ),

	OP_DEF( trueOp,						"WINDOWS" ),
#elif defined(_LINUX)
	OP_DEF( trueOp,						"LINUX" ),
#endif

    // following must be last in table
    OP_DEF(    NULL,                   "" )
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
};  // end extern "C"


