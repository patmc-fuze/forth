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

//////////////////////////////////////////////////////////////////////
////
///     built-in forth ops are implemented with static C-style routines
//      which take a pointer to the ForthThread they are being run in
//      the thread is accesed through "g->" in the code

#define FORTHOP(NAME) static void NAME(ForthThread *g)


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
    g->SetState( kResultDone );
}

// bye is a user command which causes the entire forth program to exit
FORTHOP(byeOp)
{
    g->SetState( kResultExitShell );
}

// abort is a user command which causes the entire forth engine to exit,
//   and indicates that a fatal error has occured
FORTHOP( abortOp )
{
    g->SetFatalError( kForthErrorAbort );
}

FORTHOP( argvOp )
{
    NEEDS( 1 );
    g->Push( (long) (g->GetEngine()->GetShell()->GetArg( g->Pop() )) );
}

FORTHOP( argcOp )
{
    g->Push( g->GetEngine()->GetShell()->GetArgCount() );
}

//##############################
//
// math ops
//

FORTHOP(plusOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a + b );
}

FORTHOP(plus1Op)
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( a + 1 );
}

FORTHOP(plus2Op)
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( a + 2 );
}

FORTHOP(plus4Op)
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( a + 4 );
}

FORTHOP(minusOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a - b );
}

FORTHOP(minus1Op)
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( a - 1 );
}

FORTHOP(minus2Op)
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( a - 2 );
}

FORTHOP(minus4Op)
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( a - 4 );
}

FORTHOP(timesOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a * b );
}

FORTHOP(times2Op)
{
    NEEDS(1);
    g->Push( g->Pop() << 1 );
}

FORTHOP(times4Op)
{
    NEEDS(1);
    g->Push( g->Pop() << 2 );
}

FORTHOP(divideOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a / b );
}

FORTHOP(divide2Op)
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( a >> 1 );
}

FORTHOP(divide4Op)
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( a >>2 );
}

FORTHOP( divmodOp )
{
    NEEDS(2);
    div_t v;
    long b = g->Pop();
    long a = g->Pop();
    v = div( a, b );
    g->Push( v.rem );
    g->Push( v.quot );
}

FORTHOP( modOp )
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a % b );
}

FORTHOP( negateOp )
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( -a );
}

FORTHOP( fplusOp )
{
    NEEDS(2);
    float b = g->FPop();
    float a = g->FPop();
    g->FPush( a + b );
}

FORTHOP( fminusOp )
{
    NEEDS(2);
    float b = g->FPop();
    float a = g->FPop();
    g->FPush( a - b );
}

FORTHOP( ftimesOp )
{
    NEEDS(2);
    float b = g->FPop();
    float a = g->FPop();
    g->FPush( a * b );
}

FORTHOP( fdivideOp )
{
    NEEDS(2);
    float b = g->FPop();
    float a = g->FPop();
    g->FPush( a / b );
}


FORTHOP( dplusOp )
{
    NEEDS(4);
    double b = g->DPop();
    double a = g->DPop();
    g->DPush( a + b );
}

FORTHOP( dminusOp )
{
    NEEDS(4);
    double b = g->DPop();
    double a = g->DPop();
    g->DPush( a - b );
}

FORTHOP( dtimesOp )
{
    NEEDS(4);
    double b = g->DPop();
    double a = g->DPop();
    g->DPush( a * b );
}

FORTHOP( ddivideOp )
{
    NEEDS(4);
    double b = g->DPop();
    double a = g->DPop();
    g->DPush( a / b );
}


FORTHOP( dsinOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( sin(a) );
}

FORTHOP( dasinOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( asin(a) );
}

FORTHOP( dcosOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( cos(a) );
}

FORTHOP( dacosOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( acos(a) );
}

FORTHOP( dtanOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( tan(a) );
}

FORTHOP( datanOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( atan(a) );
}

FORTHOP( datan2Op )
{
    NEEDS(4);
    double b = g->DPop();
    double a = g->DPop();
    g->DPush( atan2(a, b) );
}

FORTHOP( dexpOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( exp(a) );
}

FORTHOP( dlnOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( log(a) );
}

FORTHOP( dlog10Op )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( log10(a) );
}

FORTHOP( dpowOp )
{
    NEEDS(4);
    double b = g->DPop();
    double a = g->DPop();
    g->DPush( pow(a, b) );
}

FORTHOP( dsqrtOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( sqrt(a) );
}

FORTHOP( dceilOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( ceil(a) );
}

FORTHOP( dfloorOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( floor(a) );
}

FORTHOP( dabsOp )
{
    NEEDS(2);
    double a = g->DPop();
    g->DPush( fabs(a) );
}

FORTHOP( dldexpOp )
{
    NEEDS(3);
    int n = g->Pop();
    double a = g->DPop();
    g->DPush( ldexp(a, n) );
}

FORTHOP( dfrexpOp )
{
    NEEDS(2);
    int n;
    double a = g->DPop();
    g->DPush( frexp( a, &n ) );
    g->Push( n );
}

FORTHOP( dmodfOp )
{
    NEEDS(2);
    double b;
    double a = g->DPop();
    g->DPush( modf( a, &b ) );
    g->DPush( b );
}

FORTHOP( dfmodOp )
{
    NEEDS(4);
    double b = g->DPop();
    double a = g->DPop();
    g->DPush( fmod( a, b ) );
}

FORTHOP( i2fOp )
{
    NEEDS(1);
    float a = (float) g->Pop();
    g->FPush( a );
}

FORTHOP( i2dOp )
{
    NEEDS(1);
    double a = g->Pop();
    g->DPush( a );
}
FORTHOP( f2iOp )
{
    NEEDS(1);
    int a = (int) g->FPop();
    g->Push( a );
}

FORTHOP( f2dOp )
{
    NEEDS(1);
    double a = g->FPop();
    g->DPush( a );
}

FORTHOP( d2iOp )
{
    NEEDS(2);
    int a = (int) g->DPop();
    g->Push( a );
}


FORTHOP( d2fOp )
{
    NEEDS(2);
    float a = (float) g->DPop();
    g->FPush( a );
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
    if ( g->GetRDepth() < 1 ) {
        g->SetError( kForthErrorReturnStackUnderflow );
    } else {
        g->SetIP( (long *) g->RPop() );
    }
}

// exit normal op with local vars
FORTHOP(doExitLOp)
{
    // rstack: local_var_storage oldFP oldIP
    // FP points to oldFP
    g->SetRP( g->GetFP() );
    g->SetFP( (long *) (g->RPop()) );
    if ( g->GetRDepth() < 1 ) {
        g->SetError( kForthErrorReturnStackUnderflow );
    } else {
        g->SetIP( (long *) g->RPop() );
    }
}

// exit method op with no local vars
FORTHOP(doExitMOp)
{
    // rstack: oldTP oldIP
    if ( g->GetRDepth() < 2 ) {
        g->SetError( kForthErrorReturnStackUnderflow );
    } else {
        g->SetTP( (long *) g->RPop() );
        g->SetIP( (long *) g->RPop() );
    }
}

// exit method op with local vars
FORTHOP(doExitMLOp)
{
    // rstack: local_var_storage oldFP oldTP oldIP
    // FP points to oldFP
    g->SetRP( g->GetFP() );
    g->SetFP( (long *) (g->RPop()) );
    if ( g->GetRDepth() < 2 ) {
        g->SetError( kForthErrorReturnStackUnderflow );
    } else {
        g->SetTP( (long *) g->RPop() );
        g->SetIP( (long *) g->RPop() );
    }
}

FORTHOP(callOp)
{
    g->RPush( (long) g->GetIP() );
    g->SetIP( (long *) g->Pop() );
}

FORTHOP(gotoOp)
{
    g->SetIP( (long *) g->Pop() );
}

// has precedence!
FORTHOP(doOp)
{
    NEEDS(2);
    ForthEngine *pEngine = g->GetEngine();
    ForthShellStack *pShellStack = pEngine->GetShell()->GetShellStack();
    // save address for loop/+loop
    pShellStack->Push( (long)pEngine->GetDP() );
    pShellStack->Push( kShellTagDo );
    // this will be fixed by loop/+loop
    pEngine->CompileLong( OP_ABORT );
    pEngine->CompileLong( 0 );
}

// has precedence!
FORTHOP(loopOp)
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
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
    *pDoOp = COMPILED_OP( kOpBranch, (g->GetEngine()->GetDP() - pDoOp) - 1 );
}

// has precedence!
FORTHOP(loopNOp)
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
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
    *pDoOp = COMPILED_OP( kOpBranch, (g->GetEngine()->GetDP() - pDoOp) - 1 );
}

FORTHOP(doDoOp)
{
    NEEDS(2);
    long startIndex = g->Pop();
    // skip over loop exit IP right after this op
    long *newIP = (g->GetIP()) + 1;
    g->SetIP( newIP );

    g->RPush( (long) newIP );
    g->RPush( g->Pop() );
    g->RPush( startIndex );
    // top of rstack is current index, next is end index,
    //  next is looptop IP
}

FORTHOP(doLoopOp)
{
    RNEEDS(3);

    long *pRP = g->GetRP();
    long newIndex = (*pRP) + 1;

    if ( newIndex >= pRP[1] ) {
        // loop has ended, drop end, current indices, loopIP
        g->SetRP( pRP + 3 );
    } else {
        // loop again
        *pRP = newIndex;
        g->SetIP( (long *) (pRP[2]) );
    }
}

FORTHOP(doLoopNOp)
{
    RNEEDS(3);
    NEEDS(1);

    long *pRP = g->GetRP();
    long increment = g->Pop();
    long newIndex = (*pRP) + increment;
    bool done;

    done = (increment > 0) ? (newIndex >= pRP[1]) : (newIndex < pRP[1]);
    if ( done ) {
        // loop has ended, drop end, current indices, loopIP
        g->SetRP( pRP + 3 );
    } else {
        // loop again
        *pRP = newIndex;
        g->SetIP( (long *) (pRP[2]) );
    }
}

FORTHOP(iOp)
{
    g->Push( *(g->GetRP()) );
}

FORTHOP(jOp)
{
    g->Push( g->GetRP()[3] );
}


// discard stuff "do" put on rstack in preparation for an "exit"
//   of the current op from inside a do loop
FORTHOP( unloopOp )
{
    RNEEDS(3);
    // loop has ended, drop end, current indices, loopIP
    g->SetRP( g->GetRP() + 3 );
}


// exit loop immediately
FORTHOP( leaveOp )
{
    RNEEDS(3);
    long *pRP = g->GetRP();
    long *newIP = (long *) (pRP[2]) - 1;

    // loop has ended, drop end, current indices, loopIP
    g->SetRP( pRP + 3 );
    // point IP back to the branch op just after the _do opcode
    g->SetIP( newIP );
}


// if - has precedence
FORTHOP( ifOp )
{
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // save address for else/endif
    pShellStack->Push( (long)pEngine->GetDP() );
    // flag that this is the "if" branch
    pShellStack->Push( kShellTagIf );
    // this will be fixed by else/endif
    pEngine->CompileLong( OP_ABORT );
}


// else - has precedence
FORTHOP( elseOp )
{
    NEEDS(2);
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "else", pShellStack->Pop(), kShellTagIf ) )
    {
        return;
    }
    long *pIfOp = (long *) pShellStack->Pop();
    // save address for endif
    pShellStack->Push( (long) pEngine->GetDP() );
    // flag that this is the "else" branch
    pShellStack->Push( kShellTagElse );
    // this will be fixed by endif
    pEngine->CompileLong( OP_ABORT );
    // fill in the branch taken when "if" arg is false
    *pIfOp = COMPILED_OP( kOpBranchZ, (pEngine->GetDP() - pIfOp) - 1 );
}


// endif - has precedence
FORTHOP( endifOp )
{
    NEEDS(2);
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    long tag = pShellStack->Pop();
    long *pOp = (long *) pShellStack->Pop();
    if ( tag == kShellTagElse ) {
        // else branch
        // fill in the branch at end of path taken when "if" arg is true
        *pOp = COMPILED_OP( kOpBranch, (g->GetEngine()->GetDP() - pOp) - 1 );
    }
    else if ( pShell->CheckSyntaxError( "endif", tag, kShellTagIf ) )
    {
        // if branch
        // fill in the branch taken when "if" arg is false
        *pOp = COMPILED_OP( kOpBranchZ, (g->GetEngine()->GetDP() - pOp) - 1 );
    }
}



// begin - has precedence
FORTHOP( beginOp )
{
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // save address for repeat/until/again
    pShellStack->Push( (long)pEngine->GetDP() );
    pShellStack->Push( kShellTagBegin );
}


// until - has precedence
FORTHOP( untilOp )
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "until", pShellStack->Pop(), kShellTagBegin ) )
    {
        return;
    }
    long *pBeginOp =  (long *) pShellStack->Pop();
    pEngine->CompileLong( COMPILED_OP( kOpBranchZ, (pBeginOp - pEngine->GetDP()) - 1 ) );
}



// while - has precedence
FORTHOP( whileOp )
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "while", pShellStack->Pop(), kShellTagBegin ) )
    {
        return;
    }
    pShellStack->Push( (long) pEngine->GetDP() );
    pShellStack->Push( kShellTagWhile );
    // repeat will fill this in
    pEngine->CompileLong( OP_ABORT );
}


// repeat - has precedence
FORTHOP( repeatOp )
{
    NEEDS(2);
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "repeat", pShellStack->Pop(), kShellTagWhile ) )
    {
        return;
    }
    // get address of "while"
    long *pOp =  (long *) pShellStack->Pop();
    *pOp = COMPILED_OP( kOpBranchZ, (pEngine->GetDP() - pOp) );
    // get address of "begin"
    pOp =  (long *) pShellStack->Pop();
    pEngine->CompileLong( COMPILED_OP( kOpBranch, (pOp - pEngine->GetDP()) - 1 ) );
}

// again - has precedence
FORTHOP( againOp )
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "again", pShellStack->Pop(), kShellTagBegin ) )
    {
        return;
    }
    long *pBeginOp =  (long *) pShellStack->Pop();
    pEngine->CompileLong( COMPILED_OP( kOpBranch, (pBeginOp - pEngine->GetDP()) - 1 ) );
}

// case - has precedence
FORTHOP( caseOp )
{
    // leave marker for end of list of case-exit branches for endcase
   ForthEngine *pEngine = g->GetEngine();
   ForthShell *pShell = pEngine->GetShell();
   ForthShellStack *pShellStack = pShell->GetShellStack();
   pShellStack->Push( 0 );
   pShellStack->Push( kShellTagCase );
}


// of - has precedence
FORTHOP( ofOp )
{
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "of", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }

    // save address for endof
    pShellStack->Push( (long)pEngine->GetDP() );
    pShellStack->Push( kShellTagCase );
    // this will be set to a caseBranch by endof
    pEngine->CompileLong( OP_ABORT );
}


// endof - has precedence
FORTHOP( endofOp )
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    long *pDP = pEngine->GetDP();

    // this will be fixed by endcase
    pEngine->CompileLong( OP_ABORT );

    if ( !pShell->CheckSyntaxError( "endof", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }
    long *pOfOp = (long *) pShellStack->Pop();
    // fill in the branch taken when case doesn't match
    *pOfOp = COMPILED_OP( kOpCaseBranch, (pEngine->GetDP() - pOfOp) - 1 );

    // save address for endcase
    pShellStack->Push( (long) pDP );
    pShellStack->Push( kShellTagCase );
}

// endcase - has precedence
FORTHOP( endcaseOp )
{
    long *pSP = g->GetSP();
    long *pEndofOp;

    ForthEngine *pEngine = g->GetEngine();
    ForthShell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "endcase", pShellStack->Pop(), kShellTagCase ) )
    {
        return;
    }

    if ( ((pEngine->GetDP()) - (long *)(pShellStack->Peek())) == 1 ) {
        // there is no default case, we must compile a "drop" to
        //   dispose of the case selector on TOS
        pEngine->CompileLong( OP_DROP );
    }
    // patch branches from end-of-case to common exit point
    while ( (pEndofOp = (long *) (pShellStack->Pop())) != NULL ) {
        *pEndofOp = COMPILED_OP( kOpBranch, (g->GetEngine()->GetDP() - pEndofOp) - 1 );
    }
    g->SetSP( pSP );
}

///////////////////////////////////////////
//  bit-vector logic ops
///////////////////////////////////////////
FORTHOP( orOp )
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a | b );
}

FORTHOP( andOp )
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a & b );
}

FORTHOP( xorOp )
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a ^ b );
}

FORTHOP( invertOp )
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( ~a );
}

FORTHOP( lshiftOp )
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a << b );
}

FORTHOP( rshiftOp )
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a >> b );
}


///////////////////////////////////////////
//  boolean ops
///////////////////////////////////////////
FORTHOP( notOp )
{
    NEEDS(1);
    long a = g->Pop();
    g->Push( ~a );
}

FORTHOP( trueOp )
{
    g->Push( ~0 );
}

FORTHOP( falseOp )
{
    g->Push( 0 );
}

FORTHOP( nullOp )
{
    g->Push( 0 );
}

//##############################
//
// comparison ops
//

FORTHOP(equalsOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    if ( a == b ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(notEqualsOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    if ( a != b ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(greaterThanOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    if ( a > b ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(greaterEqualsOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    if ( a >= b ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(lessThanOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    if ( a < b ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(lessEqualsOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    if ( a <= b ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(equalsZeroOp)
{
    NEEDS(1);
    long a = g->Pop();
    if ( a == 0 ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(notEqualsZeroOp)
{
    NEEDS(1);
    long a = g->Pop();
    if ( a != 0 ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(greaterThanZeroOp)
{
    NEEDS(1);
    long a = g->Pop();
    if ( a > 0 ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(greaterEqualsZeroOp)
{
    NEEDS(1);
    long a = g->Pop();
    if ( a >= 0 ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(lessThanZeroOp)
{
    NEEDS(1);
    long a = g->Pop();
    if ( a < 0 ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}

FORTHOP(lessEqualsZeroOp)
{
    NEEDS(1);
    long a = g->Pop();
    if ( a <= 0 ) {
        g->Push( -1L );
    } else {
        g->Push( 0 );
    }
}


//##############################
//
// stack manipulation
//

FORTHOP(rpushOp)
{
    NEEDS(1);
    g->RPush( g->Pop() );
}

FORTHOP(rpopOp)
{
    RNEEDS(1);
    g->Push( g->RPop() );
}

FORTHOP(rdropOp)
{
    RNEEDS(1);
    g->RPop();
}

FORTHOP(dupOp)
{
    NEEDS(1);
    g->Push( *(g->GetSP()) );
}

FORTHOP(swapOp)
{
    NEEDS(2);
    long a = *(g->GetSP());
    *(g->GetSP()) = (g->GetSP())[1];
    (g->GetSP())[1] = a;
}

FORTHOP(dropOp)
{
    NEEDS(1);
    g->Pop();
}

FORTHOP(overOp)
{
    NEEDS(1);
    g->Push( (g->GetSP())[1] );
}

FORTHOP(rotOp)
{
    NEEDS(3);
    int a, b, c;
    a = (g->GetSP())[2];
    b = (g->GetSP())[1];
    c = (g->GetSP())[0];
    (g->GetSP())[2] = b;
    (g->GetSP())[1] = c;
    (g->GetSP())[0] = a;
}

FORTHOP(ddupOp)
{
    NEEDS(2);
    double *pDsp = (double *)(g->GetSP());
    g->DPush( *pDsp );
}

FORTHOP(dswapOp)
{
    NEEDS(4);
    double *pDsp = (double *)(g->GetSP());
    double a = *pDsp;
    *pDsp = pDsp[1];
    pDsp[1] = a;
}

FORTHOP(ddropOp)
{
    NEEDS(2);
    g->SetSP( g->GetSP() - 2 );
}

FORTHOP(doverOp)
{
    NEEDS(2);
    double *pDsp = (double *)(g->GetSP());
    g->DPush( pDsp[1] );
}

FORTHOP(drotOp)
{
    NEEDS(3);
    double *pDsp = (double *)(g->GetSP());
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
    g->GetEngine()->AlignDP();
}        

FORTHOP( allotOp )
{
    g->GetEngine()->AllotLongs( g->Pop() );
}        

FORTHOP( commaOp )
{
    g->GetEngine()->CompileLong( g->Pop() );
}        

FORTHOP( cCommaOp )
{
    ForthEngine *pEngine = g->GetEngine();
    char *pChar = (char *)pEngine->GetDP();
    *pChar++ = (char) g->Pop();
    pEngine->SetDP( (long *) pChar);
}        

FORTHOP(hereOp)
{
    g->Push( (long) g->GetEngine()->GetDP() );
}

FORTHOP( mallocOp )
{
    NEEDS(1);
    g->Push(  (long) malloc( g->Pop() )  );
}

FORTHOP( freeOp )
{
    NEEDS(1);
    free( (void *) g->Pop() );
}

//##############################
//
// loads & stores
//

FORTHOP(storeOp)
{
    NEEDS(2);
    long *pB = (long *)(g->Pop()); 
    long a = g->Pop();
    *pB = a;
}

FORTHOP(fetchOp)
{
    NEEDS(1);
    long *pA = (long *)(g->Pop()); 
    g->Push( *pA );
}

FORTHOP(cstoreOp)
{
    NEEDS(2);
    char *pB = (char *) (g->Pop());
    long a = g->Pop();
    *pB = (char) a;
}

FORTHOP(cfetchOp)
{
    NEEDS(1);
    char *pA = (char *) (g->Pop());
    g->Push( (*pA) & 0xFF );
}

FORTHOP(wstoreOp)
{
    NEEDS(2);
    short *pB = (short *) (g->Pop());
    long a = g->Pop();
    *pB = (short) a;
}

FORTHOP(wfetchOp)
{
    NEEDS(1);
    short *pA = (short *) (g->Pop());
    g->Push( *pA );
}

FORTHOP(dstoreOp)
{
    NEEDS(3);
    double *pB = (double *) (g->Pop()); 
    double a= g->DPop();
    *pB = a;
}

FORTHOP(dfetchOp)
{
    NEEDS(1);
    double *pA = (double *) (g->Pop());
    g->DPush( *pA );
}

FORTHOP(intoOp)
{
    g->SetVarOperation( kVarStore );
}

FORTHOP(addToOp)
{
    g->SetVarOperation( kVarPlusStore );
}

FORTHOP(subtractFromOp)
{
    g->SetVarOperation( kVarMinusStore );
}

FORTHOP(addressOfOp)
{
    g->SetVarOperation( kVarRef );
}

//##############################
//
// string ops
//

FORTHOP( strcpyOp )
{
    char *pSrc = (char *) g->Pop();
    char *pDst = (char *) g->Pop();
    strcpy( pDst, pSrc );
}

FORTHOP( strlenOp )
{
    char *pSrc = (char *) g->Pop();
    g->Push(  strlen( pSrc ) );
}

FORTHOP( strcatOp )
{
    char *pSrc = (char *) g->Pop();
    char *pDst = (char *) g->Pop();
    strcat( pDst, pSrc );
}


FORTHOP( strchrOp )
{
    int c = (int) g->Pop();
    char *pStr = (char *) g->Pop();
    g->Push( (long) strchr( pStr, c ) );
}

FORTHOP( strcmpOp )
{
    char *pStr2 = (char *) g->Pop();
    char *pStr1 = (char *) g->Pop();
    g->Push( (long) strcmp( pStr1, pStr2 ) );
}


FORTHOP( strstrOp )
{
    char *pStr2 = (char *) g->Pop();
    char *pStr1 = (char *) g->Pop();
    g->Push( (long) strstr( pStr1, pStr2 ) );
}


FORTHOP( strtokOp )
{
    char *pStr2 = (char *) g->Pop();
    char *pStr1 = (char *) g->Pop();
    g->Push( (long) strtok( pStr1, pStr2 ) );
}


// push the immediately following literal 32-bit constant
FORTHOP(litOp)
{
    long *pV = g->GetIP();
    g->SetIP( pV + 1 );
    g->Push( *pV );
}

// push the immediately following literal 64-bit constant
FORTHOP(dlitOp)
{
    double *pV = (double *) g->GetIP();
    g->DPush( *pV++ );
    g->SetIP( (long *) pV );
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
    ForthEngine *pEngine = g->GetEngine();

    // get next symbol, add it to vocabulary with type "builds/does"
    pEngine->AlignDP();
    pEngine->AddUserOp( pEngine->GetNextSimpleToken(), true );
    // remember current DP (for does)
    gpSavedDP = pEngine->GetDP();
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
    ForthEngine *pEngine = g->GetEngine();
    
    // compile dodoes opcode & dummy word
    pEngine->CompileLong( OP_END_BUILDS );
    pEngine->CompileLong( 0 );
    // create a nameless vocabulary entry for does-body opcode
    //newOp = pVocab->AddSymbol( NULL, kOpUserDef, (long) pEngine->GetDP() );
    newOp = pEngine->AddOp( pEngine->GetDP() );
    newOp = COMPILED_OP( kOpUserDef, newOp );
    pEngine->CompileLong( OP_DO_DOES );
    // stuff does-body opcode in dummy word
    pEngine->GetDP()[-2] = newOp;
    // compile local vars allocation op (if needed)
    pEngine->EndOpDefinition();
}

extern char *newestSymbol;
FORTHOP( newestSymbolOp )
{
    g->Push( (long) g->GetEngine()->GetDefinitionVocabulary()->NewestSymbol() );
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
    g->GetEngine()->GetDefinitionVocabulary()->UnSmudgeNewestSymbol();
    
    // fetch opcode at pIP, compile it into dummy word remembered by builds
    *gpSavedDP = *g->GetIP();
    // we are done defining, bail out
    g->SetIP( (long *) (g->RPop()) );
}

// doDoes
// - does not have precedence
// - is executed while executing the user word
// - it gets the parameter address of the user word
// 
FORTHOP( doDoesOp )
{
    g->Push( g->RPop() );
}

// exit has precedence
FORTHOP( exitOp )
{
    ForthEngine *pEngine = g->GetEngine();
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
    ForthEngine *pEngine = g->GetEngine();

    exitOp( g );
    // switch back from compile mode to execute mode
    pEngine->SetCompileState( 0 );
    pEngine->SetCompileFlags( 0 );
    // finish current symbol definition
    // compile local vars allocation op (if needed)
    pEngine->EndOpDefinition( true );
}

FORTHOP( colonOp )
{
    ForthEngine *pEngine = g->GetEngine();
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition( true );
    // switch to compile mode
    pEngine->SetCompileState( 1 );
    pEngine->SetCompileFlags( 0 );
}

FORTHOP( createOp )
{
    ForthEngine *pEngine = g->GetEngine();
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition( false );
    pEngine->CompileLong( OP_DO_VAR );
}

FORTHOP( forgetOp )
{
    ForthEngine *pEngine = g->GetEngine();
    pEngine->ForgetSymbol( pEngine->GetNextSimpleToken() );
    // reset search & definitions vocabs in case we deleted a vocab we were using
    pEngine->SetDefinitionVocabulary( pEngine->GetForthVocabulary() );
    pEngine->SetSearchVocabulary( pEngine->GetForthVocabulary() );
}

FORTHOP( definitionsOp )
{
    g->GetEngine()->SetDefinitionVocabulary( (ForthVocabulary *) (g->Pop()) );
}

FORTHOP( usesOp )
{
    g->GetEngine()->SetSearchVocabulary( (ForthVocabulary *) (g->Pop()) );
}

FORTHOP( forthOp )
{
    g->Push( (long) (g->GetEngine()->GetForthVocabulary()) );
}

FORTHOP( searchVocabOp )
{
    if ( g->GetVarOperation() == kVarFetch )
    {
        g->Push( (long) (g->GetEngine()->GetSearchVocabulary()) );
    }
    else if ( g->GetVarOperation() == kVarStore )
    {
        g->GetEngine()->SetSearchVocabulary( (ForthVocabulary *) (g->Pop()) );
    }
}

FORTHOP( definitionsVocabOp )
{
    if ( g->GetVarOperation() == kVarFetch )
    {
        g->Push( (long) (g->GetEngine()->GetDefinitionVocabulary()) );
    }
    else if ( g->GetVarOperation() == kVarStore )
    {
        g->GetEngine()->SetDefinitionVocabulary( (ForthVocabulary *) (g->Pop()) );
    }
}

FORTHOP( vocabularyOp )
{
    ForthEngine *pEngine = g->GetEngine();
    ForthVocabulary *pDefinitionsVocab = pEngine->GetDefinitionVocabulary();
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition();
    pEngine->CompileLong( OP_DO_VOCAB );
    ForthVocabulary* pVocab = new ForthVocabulary( pEngine,
                                                   pDefinitionsVocab->GetEntryName( pDefinitionsVocab->GetNewestEntry() ),
                                                   1,
                                                   512,
                                                   pEngine->GetDP(),
                                                   ForthVocabulary::GetEntryValue( pDefinitionsVocab->GetNewestEntry() ) );
    pVocab->SetNextSearchVocabulary( pEngine->GetSearchVocabulary() );
    pEngine->CompileLong( (long) pVocab );
}

FORTHOP( variableOp )
{
    ForthEngine *pEngine = g->GetEngine();
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->StartOpDefinition();
    pEngine->CompileLong( OP_DO_VAR );
    pEngine->CompileLong( 0 );
}

FORTHOP( varsOp )
{
    ForthEngine *pEngine = g->GetEngine();
    pEngine->StartVarsDefinition();
    pEngine->SetCompileState( 0 );
}

FORTHOP( endvarsOp )
{
    ForthEngine *pEngine = g->GetEngine();
    pEngine->EndVarsDefinition();
    pEngine->SetCompileState( 1 );
}

FORTHOP( doVariableOp )
{
    // IP points to data field
    g->Push( (long) g->GetIP() );
    g->SetIP( (long *) (g->RPop()) );
}

FORTHOP( constantOp )
{
    ForthEngine *pEngine = g->GetEngine();
    pEngine->StartOpDefinition();
    pEngine->CompileLong( OP_DO_CONSTANT );
    pEngine->CompileLong( g->Pop() );
}

FORTHOP( doConstantOp )
{
    // IP points to data field
    g->Push( *g->GetIP() );
    g->SetIP( (long *) (g->RPop()) );
}

FORTHOP( dconstantOp )
{
    ForthEngine *pEngine = g->GetEngine();
    pEngine->StartOpDefinition();
    pEngine->CompileLong( OP_DO_DCONSTANT );
    pEngine->CompileDouble( g->DPop() );
}

FORTHOP( doDConstantOp )
{
    // IP points to data field
    g->DPush( *((double *) (g->GetIP())) );
    g->SetIP( (long *) (g->RPop()) );
}

FORTHOP( intOp )
{
    ForthEngine *pEngine = g->GetEngine();
    char *pToken = pEngine->GetNextSimpleToken();
    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->InVarsDefinition() ) {
        // define local variable
        pEngine->AddLocalVar( pToken, kOpLocalInt, sizeof(long) );
    } else {
        // define global variable
        pEngine->AddUserOp( pToken );
        pEngine->CompileLong( OP_DO_INT );
        pEngine->CompileLong( 0 );
    }
}

FORTHOP( doIntFetch ) 
{
    // IP points to data field
    g->Push( *g->GetIP() );
}

FORTHOP( doIntRef )
{
    // IP points to data field
    g->Push( (long) g->GetIP() );
}

FORTHOP( doIntStore ) 
{
    // IP points to data field
    long *pA = (long *) (g->GetIP());
    *pA = g->Pop();
}

FORTHOP( doIntPlusStore ) 
{
    // IP points to data field
    long *pA = (long *) (g->GetIP());
    *pA += g->Pop();
}

FORTHOP( doIntMinusStore ) 
{
    // IP points to data field
    long *pA = (long *) (g->GetIP());
    *pA -= g->Pop();
}

ForthOp intOps[] = {
    doIntFetch,
    doIntRef,
    doIntStore,
    doIntPlusStore,
    doIntMinusStore
};

FORTHOP( doIntOp )
{
    
    // IP points to data field
    if ( g->GetVarOperation() == kVarFetch ) {

        g->Push( *g->GetIP() );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        intOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
    g->SetIP( (long *) (g->RPop()) );
}

FORTHOP( doLocalIntFetch ) 
{
    // IP points to data field
    g->Push( * (long *) g->Pop() );
}

FORTHOP( doLocalIntRef )
{
    // address is already on stack
}

FORTHOP( doLocalIntStore ) 
{
    // IP points to data field
    long *pA = (long *) g->Pop();
    *pA = g->Pop();
}

FORTHOP( doLocalIntPlusStore ) 
{
    // IP points to data field
    long *pA = (long *) g->Pop();
    *pA += g->Pop();
}

FORTHOP( doLocalIntMinusStore ) 
{
    // IP points to data field
    long *pA = (long *) g->Pop();
    *pA -= g->Pop();
}

ForthOp localIntOps[] = {
    doLocalIntFetch,
    doLocalIntRef,
    doLocalIntStore,
    doLocalIntPlusStore,
    doLocalIntMinusStore
};

void
ForthEngine::LocalIntAction( ForthThread   *g,
                             ulong         offset )
{
    if ( g->GetVarOperation() == kVarFetch ) {

        g->Push( *(g->GetFP() - offset) );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long)(g->GetFP() - offset) );
        // TOS points to data field
        localIntOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


void
ForthEngine::FieldIntAction( ForthThread   *g,
                             ulong         offset )
{
    long *pVar = ((long *)*(g->GetSP())) + offset;
    if ( g->GetVarOperation() == kVarFetch ) {

        *(g->GetSP()) = *pVar;

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        *(g->GetSP()) = (long) pVar;
        // TOS points to data field
        localIntOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


void
ForthEngine::MemberIntAction( ForthThread   *g,
                              ulong         offset )
{
    long *pInt = ((long *) (g->GetTP()[1])) + offset;
    if ( g->GetVarOperation() == kVarFetch ) {

        g->Push( *pInt );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long)pInt );
        // TOS points to data field
        localIntOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


FORTHOP( floatOp )
{
    float t = 0.0;
    ForthEngine *pEngine = g->GetEngine();
    char *pToken = pEngine->GetNextSimpleToken();
    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->InVarsDefinition() ) {
        // define local variable
        pEngine->AddLocalVar( pToken, kOpLocalFloat, sizeof(float) );
    } else {
        // define global variable
        pEngine->AddUserOp( pToken );
        pEngine->CompileLong( OP_DO_FLOAT );
        pEngine->CompileLong( *(long *) &t );
    }
}

FORTHOP( doFloatPlusStore ) 
{
    // IP points to data field
    float *pA = (float *) (g->GetIP());
    *pA += g->FPop();
}

FORTHOP( doFloatMinusStore ) 
{
    // IP points to data field
    float *pA = (float *) (g->GetIP());
    *pA -= g->FPop();
}

ForthOp floatOps[] = {
    doIntFetch,
    doIntRef,
    doIntStore,
    doFloatPlusStore,
    doFloatMinusStore
};

FORTHOP( doFloatOp )
{    
    // IP points to data field
    if ( g->GetVarOperation() == kVarFetch ) {

        g->FPush( *(float *) g->GetIP() );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        floatOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
    g->SetIP( (long *) (g->RPop()) );
}

FORTHOP( doLocalFloatPlusStore ) 
{
    // IP points to data field
    float *pA = (float *) g->Pop();
    *pA += g->FPop();
}

FORTHOP( doLocalFloatMinusStore ) 
{
    // IP points to data field
    float *pA = (float *) g->Pop();
    *pA -= g->FPop();
}

ForthOp localFloatOps[] = {
    doLocalIntFetch,
    doLocalIntRef,
    doLocalIntStore,
    doLocalFloatPlusStore,
    doLocalFloatMinusStore
};

void
ForthEngine::LocalFloatAction( ForthThread   *g,
                               ulong         offset )
{
    if ( g->GetVarOperation() == kVarFetch ) {

        g->FPush( *(float *)(g->GetFP() - offset) );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long)(g->GetFP() - offset) );
        localFloatOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


void
ForthEngine::FieldFloatAction( ForthThread   *g,
                               ulong         offset )
{
    float *pVar = (float *) ( ((long *)*(g->GetSP())) + offset );

    if ( g->GetVarOperation() == kVarFetch ) {

        *(g->GetSP()) = *((long *) pVar);

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        *(g->GetSP()) = (long) pVar;
        // TOS points to data field
        localFloatOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


void
ForthEngine::MemberFloatAction( ForthThread   *g,
                                ulong         offset )
{
    float *pFloat = ((float *) (g->GetTP()[1])) + offset;
    if ( g->GetVarOperation() == kVarFetch ) {

        g->FPush( *pFloat );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long) pFloat );
        // TOS points to data field
        localFloatOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


FORTHOP( doubleOp )
{
    double *pDT;
    ForthEngine *pEngine = g->GetEngine();
    char *pToken = pEngine->GetNextSimpleToken();
    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->InVarsDefinition() ) {
        // define local variable
        pEngine->AddLocalVar( pToken, kOpLocalDouble, sizeof(double) );
    } else {
        // define global variable
        pEngine->AddUserOp( pToken );
        pEngine->CompileLong( OP_DO_DOUBLE );
        pDT = (double *) pEngine->GetDP();
        *pDT++ = 0.0;
        pEngine->SetDP( (long *) pDT );
    }
}

FORTHOP( doDoubleFetch ) 
{
    // IP points to data field
    double *pA = (double *) (g->GetIP());
    g->DPush( *pA );
}

FORTHOP( doDoubleStore ) 
{
    // IP points to data field
    double *pA = (double *) (g->GetIP());
    *pA = g->DPop();
}

FORTHOP( doDoublePlusStore ) 
{
    // IP points to data field
    double *pA = (double *) (g->GetIP());
    *pA += g->DPop();
}

FORTHOP( doDoubleMinusStore ) 
{
    // IP points to data field
    double *pA = (double *) (g->GetIP());
    *pA -= g->DPop();
}

ForthOp doubleOps[] = {
    doDoubleFetch,
    doIntRef,
    doDoubleStore,
    doDoublePlusStore,
    doDoubleMinusStore
};

FORTHOP( doDoubleOp )
{
    // IP points to data field
    if ( g->GetVarOperation() == kVarFetch ) {

        g->DPush( *(double *) g->GetIP() );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        doubleOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
    g->SetIP( (long *) (g->RPop()) );
}

FORTHOP( doLocalDoubleFetch ) 
{
    // IP points to data field
    double *pA = (double *) g->Pop();
    g->DPush( *pA );
}

FORTHOP( doLocalDoubleStore ) 
{
    // IP points to data field
    double *pA = (double *) g->Pop();
    *pA = g->DPop();
}

FORTHOP( doLocalDoublePlusStore ) 
{
    // IP points to data field
    double *pA = (double *) g->Pop();
    *pA += g->DPop();
}

FORTHOP( doLocalDoubleMinusStore ) 
{
    // IP points to data field
    double *pA = (double *) g->Pop();
    *pA -= g->DPop();
}

ForthOp localDoubleOps[] = {
    doLocalDoubleFetch,
    doLocalIntRef,
    doLocalDoubleStore,
    doLocalDoublePlusStore,
    doLocalDoubleMinusStore
};

void
ForthEngine::LocalDoubleAction( ForthThread   *g,
                                ulong         offset )
{
    if ( g->GetVarOperation() == kVarFetch ) {

        g->DPush( *(double *)(g->GetFP() - offset) );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long)(g->GetFP() - offset) );
        localDoubleOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


void
ForthEngine::FieldDoubleAction( ForthThread   *g,
                                ulong         offset )
{
    double *pVar = (double *) ( (long *)(g->Pop()) + offset );

    if ( g->GetVarOperation() == kVarFetch ) {

        g->DPush( *pVar );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long) pVar );
        localDoubleOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


void
ForthEngine::MemberDoubleAction( ForthThread   *g,
                                 ulong         offset )
{
    double *pDouble = (double *) (((long *) (g->GetTP()[1])) + offset);
    if ( g->GetVarOperation() == kVarFetch ) {

        g->DPush( *pDouble );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long) pDouble );
        // TOS points to data field
        localDoubleOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


FORTHOP( stringOp )
{
    ForthEngine *pEngine = g->GetEngine();
    char *pToken = pEngine->GetNextSimpleToken();
    long len = g->Pop();

    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->InVarsDefinition() ) {
        // define local variable
        pEngine->AddLocalVar( pToken, kOpLocalString, len );
    } else {
        // define global variable
        pEngine->AddUserOp( pToken );
        pEngine->CompileLong( OP_DO_STRING );
        pEngine->AllotLongs( ((len  + 3) & ~3) >> 2 );
    }
}

FORTHOP( doStringStore ) 
{
    // IP points to data field
    char *pA = (char *) (g->GetIP());
    strcpy( pA, (char *) g->Pop() );
}

FORTHOP( doStringAppend ) 
{
    // IP points to data field
    char *pA = (char *) (g->GetIP());
    strcat( pA, (char *) g->Pop() );
}

ForthOp stringOps[] = {
    doIntRef,
    doIntRef,
    doStringStore,
    doStringAppend
};

FORTHOP( doStringOp )
{
    
    // IP points to data field
    if ( g->GetVarOperation() == kVarFetch ) {

        g->Push( (long) g->GetIP() );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarPlusStore) ) {

        stringOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
    g->SetIP( (long *) (g->RPop()) );
}

FORTHOP( doLocalStringStore ) 
{
    // IP points to data field
    char *pA = (char *) g->Pop();
    strcpy( pA, (char *) g->Pop() );
}

FORTHOP( doLocalStringAppend ) 
{
    // IP points to data field
    char *pA = (char *) g->Pop();
    strcat( pA, (char *) g->Pop() );
}

ForthOp localStringOps[] = {
    doLocalIntRef,
    doLocalIntRef,
    doLocalStringStore,
    doLocalStringAppend
};

void
ForthEngine::LocalStringAction( ForthThread   *g,
                                ulong         offset )
{
    if ( g->GetVarOperation() == kVarFetch ) {

        g->Push( (long)(g->GetFP() - offset) );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarPlusStore) ) {

        g->Push( (long)(g->GetFP() - offset) );
        // TOS points to data field
        localStringOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


void
ForthEngine::FieldStringAction( ForthThread   *g,
                                ulong         offset )
{
    long *pVar = ((long *)*(g->GetSP())) + offset;
    if ( g->GetVarOperation() == kVarFetch ) {

        *(g->GetSP()) = (long) pVar;

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarPlusStore) ) {

        *(g->GetSP()) = (long) pVar;
        // TOS points to data field
        localStringOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}


void
ForthEngine::MemberStringAction( ForthThread   *g,
                                 ulong         offset )
{
    char *pString = (char *) (((long *) (g->GetTP()[1])) + offset);
    if ( g->GetVarOperation() == kVarFetch ) {

        g->Push( (long) pString );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long) pString );
        // TOS points to data field
        localStringOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}

FORTHOP( doVocabOp )
{
    // IP points to data field
    g->Push( *g->GetIP() );
    g->SetIP( (long *) (g->RPop()) );
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
    g->GetEngine()->GetDefinitionVocabulary()->UnSmudgeNewestSymbol();
}

FORTHOP( precedenceOp )
{
    ForthEngine *pEngine = g->GetEngine();
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
    ForthEngine *pEngine = g->GetEngine();
    char *pFileName = pEngine->GetNextSimpleToken();

    if ( pFileName != NULL ) {
        pInFile = fopen( pFileName, "r" );
        if ( pInFile != NULL ) {
            pEngine->PushInputStream( pInFile );
        } else {
            printf( "!!!! Failure opening source file %s !!!!\n", pFileName );
            TRACE( "!!!! Failure opening source file %s !!!!\n", pFileName );
        }

    }
}

FORTHOP( loadDoneOp )
{
    g->GetEngine()->PopInputStream();
}

FORTHOP( stateInterpretOp )
{
    g->GetEngine()->SetCompileState( 0 );
}

FORTHOP( stateCompileOp )
{
    g->GetEngine()->SetCompileState( 1 );
}

FORTHOP( stateOp )
{
    g->Push( (long) g->GetEngine()->GetCompileStatePtr() );
}

FORTHOP( tickOp )
{
    ForthEngine *pEngine = g->GetEngine();
    char *pToken = pEngine->GetNextSimpleToken();
    long *pSymbol = (long *) (pEngine->FindSymbol( pToken ));
    if ( pSymbol != NULL ) {
        g->Push( *pSymbol );
    } else {
        g->SetError( kForthErrorUnknownSymbol );
    }
}

FORTHOP( executeOp )
{
    long mProg[2];
    long *oldIP = g->GetIP();

    // execute the opcode
    mProg[0] = g->Pop();
    mProg[1] = BUILTIN_OP( OP_DONE );
    
    g->SetIP( mProg );
    g->GetEngine()->InnerInterpreter( g );
    g->SetIP( oldIP );
}

// has precedence!
FORTHOP( compileOp )
{
    ForthEngine *pEngine = g->GetEngine();
    char *pToken = pEngine->GetNextSimpleToken();
    long *pSymbol = (long *) (pEngine->FindSymbol( pToken ));
    if ( pSymbol != NULL ) {
        pEngine->CompileLong( *pSymbol );
    } else {
        g->SetError( kForthErrorUnknownSymbol );
    }
}

// has precedence!
FORTHOP( bracketTickOp )
{
    // TBD: what should this do if state is interpret? an error? or act the same as tick?
    ForthEngine *pEngine = g->GetEngine();
    char *pToken = pEngine->GetNextSimpleToken();
    long *pSymbol = (long *) (pEngine->FindSymbol( pToken ));
    if ( pSymbol != NULL ) {
        pEngine->CompileLong( OP_INT_VAL );
        pEngine->CompileLong( *pSymbol );
    } else {
        g->SetError( kForthErrorUnknownSymbol );
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
    char *pStr = (char *) g->Pop();
    char *pOutStr = g->GetConOutString();
    if ( pOutStr != NULL ) {
        strcat( pOutStr, pStr );
    }
}

static void
stringOut( ForthThread  *g,
           const char   *buff )
{    
    FILE *pOutFile = g->GetConOutFile();
    if ( pOutFile != NULL ) {
        fprintf(pOutFile, "%s", buff );
    } else {
        g->Push( (long) buff );
        ConOutOpInvoke( g );
    }
}

static void
printNumInCurrentBase( ForthThread *    g,
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
        base = *(g->GetBaseRef());

        signMode = g->GetPrintSignedNumMode();
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

    stringOut( g, pNext );
}


FORTHOP( printNumOp )
{
    printNumInCurrentBase( g, g->Pop() );
}

FORTHOP( printNumDecimalOp )
{
    NEEDS(1);
    char buff[36];

    long val = g->Pop();
    sprintf( buff, "%d", val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    stringOut( g, buff );
}

FORTHOP( printNumHexOp )
{
    NEEDS(1);
    char buff[12];

    long val = g->Pop();
    sprintf( buff, "%x", val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    stringOut( g, buff );
}

FORTHOP( printFloatOp )
{
    NEEDS(1);
    char buff[36];

    float fval = g->FPop();
    sprintf( buff, "%f", fval );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    stringOut( g, buff );
}

FORTHOP( printDoubleOp )
{
    NEEDS(2);
    char buff[36];
    double dval = g->DPop();

    sprintf( buff, "%f", dval );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    stringOut( g, buff );
}

FORTHOP( printFormattedOp )
{
    NEEDS(2);
    char buff[80];

    char* pFmt = (char *) (g->Pop());
    long val = g->Pop();
    sprintf( buff, pFmt, val );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    stringOut( g, buff );
}

FORTHOP( printStrOp )
{
    NEEDS(1);
    char *buff = (char *) g->Pop();
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    stringOut( g, buff );
}

FORTHOP( printCharOp )
{
    NEEDS(1);
    char buff[4];
    char c = (char) g->Pop();
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %c\n", c );
#endif

    FILE *pOutFile = g->GetConOutFile();
    if ( pOutFile != NULL ) {
        fputc( c, pOutFile );
    } else {
        buff[0] = c;
        buff[1] = 0;
        g->Push( (long) buff );
        ConOutOpInvoke( g );
    }
}

FORTHOP( printSpaceOp )
{
    g->Push( (long) ' ' );
    printCharOp( g );
}

FORTHOP( printNewlineOp )
{
    g->Push( (long) '\n' );
    printCharOp( g );
}

FORTHOP( baseOp )
{
    g->Push( (long) g->GetBaseRef() );
}

FORTHOP( decimalOp )
{
    *g->GetBaseRef() = 10;
}

FORTHOP( hexOp )
{
    *g->GetBaseRef() = 16;
}

FORTHOP( printDecimalSignedOp )
{
    g->SetPrintSignedNumMode( kPrintSignedDecimal );
}

FORTHOP( printAllSignedOp )
{
    g->SetPrintSignedNumMode( kPrintAllSigned );
}

FORTHOP( printAllUnsignedOp )
{
    g->SetPrintSignedNumMode( kPrintAllUnsigned );
}

FORTHOP( outToScreenOp )
{
    g->SetConOutFile( stdout );
}

FORTHOP( outToFileOp )
{
    NEEDS( 1 );
    g->SetConOutFile( (FILE *) g->Pop() );
}

FORTHOP( outToStringOp )
{
    NEEDS( 1 );
    g->SetConOutString( (char *) g->Pop() );
    g->SetConOutFile( NULL );
}

FORTHOP( getConOutFileOp )
{
    g->Push( (long) g->GetConOutFile() );
}

//##############################
//
//  File ops
//
FORTHOP( fopenOp )
{
    NEEDS(2);
    char *pAccess = (char *) g->Pop();
    char *pName = (char *) g->Pop();
    
    FILE *pFP = fopen( pName, pAccess );
    g->Push( (long) pFP );
}

FORTHOP( fcloseOp )
{
    NEEDS(1);
    
    int result = fclose( (FILE *) g->Pop() );
    g->Push( result );
}

FORTHOP( fseekOp )
{
    int ctrl = g->Pop();
    int offset = g->Pop();
    FILE *pFP = (FILE *) g->Pop();
    
    int result = fseek( pFP, offset, ctrl );
    g->Push( result );
}

FORTHOP( freadOp )
{
    NEEDS(4);
    
    FILE *pFP = (FILE *) g->Pop();
    int itemSize = g->Pop();
    int numItems = g->Pop();
    void *pDst = (void *) g->Pop();
    
    int result = fread( pDst, numItems, itemSize, pFP );
    g->Push( result );
}

FORTHOP( fwriteOp )
{
    
    NEEDS(4);
    FILE *pFP = (FILE *) g->Pop();
    int itemSize = g->Pop();
    int numItems = g->Pop();
    void *pSrc = (void *) g->Pop();
    
    int result = fwrite( pSrc, numItems, itemSize, pFP );
    g->Push( result );
}

FORTHOP( fgetcOp )
{    
    NEEDS(1);
    FILE *pFP = (FILE *) g->Pop();
    
    int result = fgetc( pFP );
    g->Push( result );
}

FORTHOP( fputcOp )
{    
    NEEDS(2);
    FILE *pFP = (FILE *) g->Pop();
    int outChar = g->Pop();
    
    int result = fputc( outChar, pFP );
    g->Push( result );
}

FORTHOP( feofOp )
{
    NEEDS(1);
    int result = feof( (FILE *) g->Pop() );
    g->Push( result );
}

FORTHOP( ftellOp )
{
    NEEDS(1);
    int result = ftell( (FILE *) g->Pop() );
    g->Push( result );
}


FORTHOP( systemOp )
{
    NEEDS(1);
    int result = system( (char *) g->Pop() );
    g->Push( result );
}



//##############################
//
// Default input/output file ops
//

FORTHOP( stdinOp )
{
    g->Push( (long) stdin );
}

FORTHOP( stdoutOp )
{
    g->Push( (long) stdout );
}

FORTHOP( stderrOp )
{
    g->Push( (long) stderr );
}

FORTHOP( dstackOp )
{
    long *pSP = g->GetSP();
    int nItems = g->GetSDepth();
    int i;

    stringOut( g, "stack:" );
    for ( i = 0; i < nItems; i++ ) {
        stringOut( g, " " );
        printNumInCurrentBase( g, *pSP++ );
    }
    stringOut( g, "\n" );
}


FORTHOP( drstackOp )
{
    long *pRP = g->GetRP();
    int nItems = g->GetRDepth();
    int i;

    stringOut( g, "rstack:" );
    for ( i = 0; i < nItems; i++ ) {
        stringOut( g, " " );
        printNumInCurrentBase( g, *pRP++ );
    }
    stringOut( g, "\n" );
}


// return true IFF user quit out
static bool
ShowVocab( ForthThread      *g,
           ForthVocabulary  *pVocab )
{
#define BUFF_SIZE 256
    char buff[BUFF_SIZE];
    int i, len;
    bool retVal = false;
    ForthShell *pShell = g->GetEngine()->GetShell();
    int nEntries = pVocab->GetNumEntries();
    void *pEntry = pVocab->GetFirstEntry();

    for ( i = 0; i < nEntries; i++ ) {
        sprintf( buff, "%02x %06x ", ForthVocabulary::GetEntryType( pEntry ), ForthVocabulary::GetEntryValue( pEntry ) );
        stringOut( g, buff );
        len = pVocab->GetEntryNameLength( pEntry );
        if ( len > (BUFF_SIZE - 1)) {
            len = BUFF_SIZE - 1;
        }
        memcpy( buff, pVocab->GetEntryName( pEntry ), len );
        buff[len] = '\0';
        stringOut( g, buff );
        stringOut( g, "\n" );
        pEntry = pVocab->NextEntry( pEntry );
        if ( ((i % 22) == 21) || (i == (nEntries-1)) ) {
            if ( (pShell != NULL) && pShell->GetInput()->InputStream()->IsInteractive() ) {
                stringOut( g, "Hit ENTER to continue, 'q' & ENTER to quit\n" );
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
    stringOut( g, "Definitions Vocabulary:\n" );
    if ( ShowVocab( g, g->GetEngine()->GetDefinitionVocabulary() ) == false ) {
        stringOut( g, "Precedence Vocabulary:\n" );
        ShowVocab( g, g->GetEngine()->GetPrecedenceVocabulary() );
    }
}


FORTHOP( loadLibraryOp )
{
    NEEDS( 1 );
    char* pDLLName = (char *) g->Pop();
    HINSTANCE hDLL = LoadLibrary( pDLLName );
    g->Push( (long) hDLL );
}

FORTHOP( freeLibraryOp )
{
    NEEDS( 1 );
    HINSTANCE hDLL = (HINSTANCE) g->Pop();
    FreeLibrary( hDLL );
}

FORTHOP( getProcAddressOp )
{
    NEEDS( 2 );
    char* pProcName = (char *) g->Pop();
    HINSTANCE hDLL = (HINSTANCE) g->Pop();
    long pFunc = (long) GetProcAddress( hDLL, pProcName );

    g->Push( pFunc );
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
    proc0Args pFunc = (proc0Args) g->Pop();
    g->Push( pFunc() );
}

FORTHOP( callProc1Op )
{
    NEEDS( 1 );
    proc1Args pFunc = (proc1Args) g->Pop();
    long a1 = g->Pop();
    g->Push( pFunc( a1 ) );
}

FORTHOP( callProc2Op )
{
    NEEDS( 2 );
    proc2Args pFunc = (proc2Args) g->Pop();
    long a2 = g->Pop();
    long a1 = g->Pop();
    g->Push( pFunc( a1, a2 ) );
}

FORTHOP( callProc3Op )
{
    NEEDS( 3 );
    proc3Args pFunc = (proc3Args) g->Pop();
    long a3 = g->Pop();
    long a2 = g->Pop();
    long a1 = g->Pop();
    g->Push( pFunc( a1, a2, a3 ) );
}

FORTHOP( callProc4Op )
{
    NEEDS( 4 );
    proc4Args pFunc = (proc4Args) g->Pop();
    long a4 = g->Pop();
    long a3 = g->Pop();
    long a2 = g->Pop();
    long a1 = g->Pop();
    g->Push( pFunc( a1, a2, a3, a4 ) );
}

FORTHOP( callProc5Op )
{
    NEEDS( 5 );
    proc5Args pFunc = (proc5Args) g->Pop();
    long a5 = g->Pop();
    long a4 = g->Pop();
    long a3 = g->Pop();
    long a2 = g->Pop();
    long a1 = g->Pop();
    g->Push( pFunc( a1, a2, a3, a4, a5 ) );
}

FORTHOP( callProc6Op )
{
    NEEDS( 6 );
    proc6Args pFunc = (proc6Args) g->Pop();
    long a6 = g->Pop();
    long a5 = g->Pop();
    long a4 = g->Pop();
    long a3 = g->Pop();
    long a2 = g->Pop();
    long a1 = g->Pop();
    g->Push( pFunc( a1, a2, a3, a4, a5, a6 ) );
}

FORTHOP( callProc7Op )
{
    NEEDS( 7 );
    proc7Args pFunc = (proc7Args) g->Pop();
    long a7 = g->Pop();
    long a6 = g->Pop();
    long a5 = g->Pop();
    long a4 = g->Pop();
    long a3 = g->Pop();
    long a2 = g->Pop();
    long a1 = g->Pop();
    g->Push( pFunc( a1, a2, a3, a4, a5, a6, a7 ) );
}

FORTHOP( callProc8Op )
{
    NEEDS( 8 );
    proc8Args pFunc = (proc8Args) g->Pop();
    long a8 = g->Pop();
    long a7 = g->Pop();
    long a6 = g->Pop();
    long a5 = g->Pop();
    long a4 = g->Pop();
    long a3 = g->Pop();
    long a2 = g->Pop();
    long a1 = g->Pop();
    g->Push( pFunc( a1, a2, a3, a4, a5, a6, a7, a8 ) );
}

FORTHOP( blwordOp )
{
    NEEDS( 0 );
    ForthShell *pShell = g->GetEngine()->GetShell();
    g->Push( (long) (pShell->GetNextSimpleToken()) );
}

FORTHOP( wordOp )
{
    NEEDS( 1 );
    ForthShell *pShell = g->GetEngine()->GetShell();
    char delim = (char) (g->Pop());
    g->Push( (long) (pShell->GetToken( delim )) );
}

FORTHOP( getInBufferBaseOp )
{
    ForthInputStack* pInput = g->GetEngine()->GetShell()->GetInput();
    g->Push( (long) (pInput->GetBufferBasePointer()) );
}

FORTHOP( getInBufferPointerOp )
{
    ForthInputStack* pInput = g->GetEngine()->GetShell()->GetInput();
    g->Push( (long) (pInput->GetBufferPointer()) );
}

FORTHOP( setInBufferPointerOp )
{
    ForthInputStack* pInput = g->GetEngine()->GetShell()->GetInput();
    pInput->SetBufferPointer( (char *) (g->Pop()) );
}

FORTHOP( getInBufferLengthOp )
{
    ForthInputStack* pInput = g->GetEngine()->GetShell()->GetInput();
    g->Push( (long) (pInput->GetBufferLength()) );
}

FORTHOP( fillInBufferOp )
{
    ForthInputStack* pInput = g->GetEngine()->GetShell()->GetInput();
    g->Push( (long) (pInput->GetLine( (char *) (g->Pop()) )) );
}

#define OP( func, funcName )  { funcName, kOpBuiltIn, (ulong) func, 0 }

// ops which have precedence (execute at compile time)
#define PRECOP( func, funcName )  { funcName, ((ulong) kOpBuiltIn) | BASE_DICT_PRECEDENCE_FLAG, (ulong) func, 1 }

// NOTE: the order of the first few entries in this table must agree
// with the list near the top of the file!  (look for COMPILED_OP)

baseDictEntry baseDict[] = {
    ///////////////////////////////////////////
    //  STUFF WHICH IS COMPILED BY OTHER WORDS
    //   DO NOT REARRANGE UNDER PAIN OF DEATH
    ///////////////////////////////////////////
    OP(     abortOp,                "abort" ),
    OP(     dropOp,                 "drop" ),
    OP(     doDoesOp,               "_doDoes"),
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
    OP(     doLoopNOp,               "_+loop" ),
    OP(     doExitOp,               "_exit" ),      // exit normal op with no vars
    OP(     doExitLOp,              "_exitL" ),     // exit normal op with local vars
    OP(     doExitMOp,              "_exitM" ),     // exit method op with no vars
    OP(     doExitMLOp,             "_exitML" ),    // exit method op with local vars
    OP(     doVocabOp,              "_doVocab" ),

    // stuff below this line can be rearranged
    
    ///////////////////////////////////////////
    //  integer math
    ///////////////////////////////////////////
    OP(     plusOp,                 "+" ),
    OP(     plus1Op,                "1+" ),
    OP(     plus2Op,                "2+" ),
    OP(     plus4Op,                "4+" ),
    OP(     minusOp,                "-" ),
    OP(     minus1Op,               "1-" ),
    OP(     minus2Op,               "2-" ),
    OP(     minus4Op,               "4-" ),
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
    OP(     wstoreOp,               "w!" ),
    OP(     wfetchOp,               "w@" ),
    OP(     dstoreOp,               "d!" ),
    OP(     dfetchOp,               "d@" ),
    OP(     addToOp,                "->+" ),
    OP(     subtractFromOp,         "->-" ),
    OP(     addressOfOp,            "addressOf" ),

    ///////////////////////////////////////////
    //  string manipulation
    ///////////////////////////////////////////
    OP(     strcpyOp,               "strcpy" ),
    OP(     strlenOp,               "strlen" ),
    OP(     strcatOp,               "strcat" ),
    OP(     strchrOp,               "strchr" ),
    OP(     strcmpOp,               "strcmp" ),
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
    OP(     definitionsOp,          "definitions" ),
    OP(     usesOp,                 "uses" ),
    OP(     forthOp,                "forth" ),
    OP(     searchVocabOp,          "searchVocab" ),
    OP(     definitionsVocabOp,     "definitionsVocab" ),
    OP(     vocabularyOp,           "vocabulary" ),
    OP(     variableOp,             "variable" ),
    OP(     constantOp,             "constant" ),
    OP(     dconstantOp,            "dconstant" ),
    PRECOP( varsOp,                 "vars" ),
    PRECOP( endvarsOp,              "endvars" ),
    OP(     intOp,                  "int" ),
    OP(     floatOp,                "float" ),
    OP(     doubleOp,               "double" ),
    OP(     stringOp,               "string" ),

    PRECOP( recursiveOp,            "recursive" ),
    OP(     precedenceOp,           "precedence" ),
    OP(     loadOp,                 "load" ),
    OP(     loadDoneOp,             "loaddone" ),
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
    OP(     blwordOp,               "blword"    ),
    OP(     wordOp,                 "word"      ),
    OP(     getInBufferBaseOp,      "getInBufferBase" ),
    OP(     getInBufferPointerOp,   "getInBufferPointer" ),
    OP(     setInBufferPointerOp,   "setInBufferPointer" ),
    OP(     getInBufferLengthOp,    "getInBufferLength" ),
    OP(     fillInBufferOp,         "fillInBuffer" ),

    // following must be last in table
    OP(     NULL,                   "" )
};

//############################################################################
//
//  The end of all things...
//
//############################################################################


