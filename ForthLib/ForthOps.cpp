
#include "stdafx.h"

// Forth operator TBDs:
// - add "align", have colon & builds call it...
// - add dpi, fpi (pi constant)
// - add string lib operators
// - builds...does words do not need to be distinct from other user
//   definitions - doDoes could be made to move the top of the return
//   stack to the param stack

// + add if/else/endif/repeat/until operators - need compile time stack
// + add built-in symbols to user vocabulary at startup

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

static long unravelIP;


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

FORTHOP(minusOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a - b );
}

FORTHOP(timesOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a * b );
}

FORTHOP(divideOp)
{
    NEEDS(2);
    long b = g->Pop();
    long a = g->Pop();
    g->Push( a / b );
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

FORTHOP(exitOp)
{
    if ( g->GetRDepth() < 1 ) {
        g->SetError( kForthErrorReturnStackUnderflow );
    } else {
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

FORTHOP(doOp)
{
    NEEDS(2);
    g->RPush( (long) g->GetIP() );
    g->RPush( g->Pop() );
    g->RPush( g->Pop() );
    // top of rstack is end index, next is current index,
    //  next is looptop IP
}

FORTHOP(loopOp)
{
    RNEEDS(3);
    if ( g->GetRP()[1] >= *(g->GetRP()) ) {
        // loop has ended, drop end, current indices, loopIP
        g->SetRP( g->GetRP() + 3 );
    } else {
        // loop again
        g->GetRP()[1] += 1;
        g->SetIP( (long *) (g->GetRP()[2]) );
    }
}

FORTHOP(loopnOp)
{
    RNEEDS(3);
    if ( g->GetRP()[1] >= *(g->GetRP()) ) {
        // loop has ended, drop end, current indices, loopIP
        g->SetRP( g->GetRP() + 3 );
    } else {
        // loop again
        g->GetRP()[1] += g->Pop();
        g->SetIP( (long *) (g->GetRP()[2]) );
    }
}

FORTHOP(iOp)
{
    g->Push( g->GetRP()[1] );
}

FORTHOP(jOp)
{
    g->Push( g->GetRP()[4] );
}

// this op unravels a stack frame upon exit from a user op which
//   has local variables
FORTHOP( unravelFrameOp )
{
    long oldRP, oldFP;
    
    oldRP = g->RPop();
    oldFP = g->RPop();
    g->SetFP( (long *) oldFP );
    g->SetRP( (long *) oldRP );
    g->SetIP( (long *) g->RPop() );
}


// if - has precedence
FORTHOP( ifOp )
{
    ForthEngine *pEngine = g->GetEngine();
    // save address for else/endif
    g->Push( (long)pEngine->GetDP() );
    // this will be fixed by else/endif
    pEngine->CompileLong( OP_ABORT );
}


// else - has precedence
FORTHOP( elseOp )
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
    long *pIfOp = (long *) g->Pop();
    // save address for endif
    g->Push( (long) pEngine->GetDP() );
    // this will be fixed by endif
    pEngine->CompileLong( OP_ABORT );
    // fill in the branch taken when "if" arg is false
    *pIfOp = COMPILED_OP( kOpBranchZ, (pEngine->GetDP() - pIfOp) - 1 );
}


// endif - has precedence
FORTHOP( endifOp )
{
    NEEDS(1);
    long *pElseOp = (long *) g->Pop();
    // fill in the branch at end of path taken when "if" arg is true
    *pElseOp = COMPILED_OP( kOpBranch, (g->GetEngine()->GetDP() - pElseOp) - 1 );
}



// begin - has precedence
FORTHOP( beginOp )
{
    g->Push( (long) g->GetEngine()->GetDP() );
}


// until - has precedence
FORTHOP( untilOp )
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
    long *pBeginOp =  (long *) g->Pop();
    pEngine->CompileLong( COMPILED_OP( kOpBranchZ, (pBeginOp - pEngine->GetDP()) - 1 ) );
}



// while - has precedence
FORTHOP( whileOp )
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
    long tmp = g->Pop();
    g->Push( (long) pEngine->GetDP() );
    g->Push( tmp );
    // repeat will fill this in
    pEngine->CompileLong( OP_ABORT );
}


// repeat - has precedence
FORTHOP( repeatOp )
{
    NEEDS(2);
    ForthEngine *pEngine = g->GetEngine();
    // get address of "begin"
    long *pOp =  (long *) g->Pop();
    pEngine->CompileLong( COMPILED_OP( kOpBranch, (pOp - pEngine->GetDP()) - 1 ) );
    // get address of "while"
    pOp =  (long *) g->Pop();
    *pOp = COMPILED_OP( kOpBranchZ, (pEngine->GetDP() - pOp) - 1 );
}

// again - has precedence
FORTHOP( againOp )
{
    NEEDS(1);
    ForthEngine *pEngine = g->GetEngine();
    long *pBeginOp =  (long *) g->Pop();
    pEngine->CompileLong( COMPILED_OP( kOpBranch, (pBeginOp - pEngine->GetDP()) - 1 ) );
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
    g->Push( a << b );
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
    g->Push( *pA );
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
    ForthVocabulary *pVocab = pEngine->GetCurrentVocabulary();

    // get next symbol, add it to vocabulary with type "builds/does"
    pEngine->AlignDP();
    pVocab->AddSymbol( pEngine->GetNextSimpleToken(), kOpUserDef, (long) pEngine->GetDP() );
    pVocab->SmudgeNewestSymbol();
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
    ForthVocabulary *pVocab = pEngine->GetCurrentVocabulary();
    
    // finish current symbol definition (of defining op)
    pVocab->UnSmudgeNewestSymbol();
    // compile dodoes opcode & dummy word
    pEngine->CompileLong( OP_END_BUILDS );
    pEngine->CompileLong( 0 );
    // create a nameless vocabulary entry for does-body opcode
    newOp = pVocab->AddSymbol( NULL, kOpUserDef, (long) pEngine->GetDP() );
    pEngine->CompileLong( OP_DO_DOES );
    // stuff does-body opcode in dummy word
    pEngine->GetDP()[-2] = newOp;
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
    g->GetEngine()->GetCurrentVocabulary()->UnSmudgeNewestSymbol();
    
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

// semi has precedence
FORTHOP( semiOp )
{
    ForthEngine *pEngine = g->GetEngine();
    // compile exitOp
    pEngine->CompileLong( OP_EXIT );
    // finish current symbol definition
    pEngine->GetCurrentVocabulary()->UnSmudgeNewestSymbol();
    // switch back from compile mode to execute mode
    pEngine->SetCompileState( 0 );
    // compile local vars allocation op (if needed)
    pEngine->EndOpDefinition();
}

FORTHOP( colonOp )
{
    ForthEngine *pEngine = g->GetEngine();
    pEngine->StartOpDefinition();
    // switch to compile mode
    pEngine->SetCompileState( 1 );
    // get next symbol, add it to vocabulary with type "user op"
    pEngine->GetCurrentVocabulary()->SmudgeNewestSymbol();
}

FORTHOP( createOp )
{
    ForthEngine *pEngine = g->GetEngine();
    pEngine->StartOpDefinition();
    pEngine->CompileLong( OP_DO_VAR );
}

FORTHOP( variableOp )
{
    ForthEngine *pEngine = g->GetEngine();
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

FORTHOP( intOp )
{
    ForthEngine *pEngine = g->GetEngine();
    char *pToken = pEngine->GetNextSimpleToken();
    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->InVarsDefinition() ) {
        // TBD: define local variable
        pEngine->AddLocalVar( pToken, kOpLocalInt, sizeof(long) );
    } else {
        pEngine->AlignDP();
        pEngine->GetCurrentVocabulary()->AddSymbol( pToken, kOpUserDef, (long) pEngine->GetDP() );
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

        g->Push( *(g->GetFP() + offset) );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long)(g->GetFP() + offset) );
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
        // TBD: define local variable
        pEngine->AddLocalVar( pToken, kOpLocalFloat, sizeof(float) );
    } else {
        pEngine->AlignDP();
        pEngine->GetCurrentVocabulary()->AddSymbol( pToken, kOpUserDef, (long) pEngine->GetDP() );
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

        g->FPush( *(float *)(g->GetFP() + offset) );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long)(g->GetFP() + offset) );
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
        // TBD: define local variable
        pEngine->AddLocalVar( pToken, kOpLocalDouble, sizeof(double) );
    } else {
        pEngine->AlignDP();
        pEngine->GetCurrentVocabulary()->AddSymbol( pToken, kOpUserDef, (long) pEngine->GetDP() );
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

        g->DPush( *(double *)(g->GetFP() + offset) );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarMinusStore) ) {

        g->Push( (long)(g->GetFP() + offset) );
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
    long len = (g->Pop() + 3) & ~3;

    // get next symbol, add it to vocabulary with type "user op"
    if ( pEngine->InVarsDefinition() ) {
        // TBD: define local variable
        pEngine->AddLocalVar( pToken, kOpLocalString, len );
    } else {
        pEngine->AlignDP();
        pEngine->GetCurrentVocabulary()->AddSymbol( pToken, kOpUserDef, (long) pEngine->GetDP() );
        pEngine->CompileLong( OP_DO_STRING );
        pEngine->AllotLongs( len >> 2 );
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

        g->Push( (long)(g->GetFP() + offset) );

    } else if ( (g->GetVarOperation() > kVarFetch) && (g->GetVarOperation() <= kVarPlusStore) ) {

        g->Push( (long)(g->GetFP() + offset) );
        // TOS points to data field
        localStringOps[ g->GetVarOperation() ] ( g );
        g->ClearVarOperation();

    //} else {
        // TBD: report g->GetVarOperation() out of range
    }
}



// TBD: recursion support
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
    g->GetEngine()->GetCurrentVocabulary()->UnSmudgeNewestSymbol();
}

FORTHOP( precedenceOp )
{
    ForthEngine *pEngine = g->GetEngine();
    char *pSym = (char *) pEngine->GetCurrentVocabulary()->FindSymbol( pEngine->GetNextSimpleToken() );
    
    if ( pSym ) {
        switch( (forthOpType) *pSym ) {
        case kOpBuiltIn:        case kOpBuiltInPrec:
        case kOpUserDef:        case kOpUserDefPrec:
        //case kOpUserBuilds:     case kOpUserBuildsPrec:
            // set highest bit of symbol
            *pSym |= 0x80;
            break;
        default:
            // TBD: report symbol has wrong type, cannot be given precedence
            break;
        }
        return;
    }
    // TBD: report that symbol was not found
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
    long *pSymbol = (long *) pEngine->GetCurrentVocabulary()->FindSymbol( pToken );
    if ( pSymbol != NULL ) {
        g->Push( *pSymbol );
    } else {
        // TBD: fix this
        g->Push( -1 );
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
           char         *buff )
{    
    FILE *pOutFile = g->GetConOutFile();
    if ( pOutFile != NULL ) {
        fprintf(pOutFile, "%s", buff );
    } else {
        g->Push( (long) buff );
        ConOutOpInvoke( g );
    }
}

FORTHOP( printNumOp )
{
    NEEDS(1);
#define PRINT_NUM_BUFF_CHARS 36
    char buff[ PRINT_NUM_BUFF_CHARS ];
    char *pNext = &buff[ PRINT_NUM_BUFF_CHARS ];
    div_t v;
    long base;
    long val = g->Pop();
    bool bIsNegative, bPrintUnsigned;
    ulong urem;

    sprintf( buff, "%d", val );
    if ( val == 0 ) {
        sprintf( buff, "0" );
        pNext = buff;
    } else {
        *--pNext = 0;
        base = *(g->GetBaseRef());

        switch( g->GetPrintSignedNumMode() ) {
        case kPrintSignedDecimal:
            bPrintUnsigned = (base != 10);
            break;
        case kPrintAllSigned:
            bPrintUnsigned = false;
            break;
        case kPrintAllUnsigned:
            bPrintUnsigned = true;
            break;
        }
        if ( bPrintUnsigned ) {
            // since div is defined as signed divide/mod, make sure
            //   that the number is not negative by generating the bottom digit
            bIsNegative = false;
            urem = ((ulong) val) % ((ulong) base);
            *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'A') );
            val = ((ulong) val) / ((ulong) base);
        } else {
            bIsNegative = ( val < 0 );
            if ( bIsNegative ) {
                val = (-val);
            }
        }
        while ( val != 0 ) {
            v = div( val, base );
            *--pNext = (char) ( (v.rem < 10) ? (v.rem + '0') : ((v.rem - 10) + 'A') );
            val = v.quot;
        }
        if ( bIsNegative ) {
            *--pNext = '-';
        }
    }
    TRACE( "printed %s\n", pNext );

    stringOut( g, buff );
}

FORTHOP( printNumDecimalOp )
{
    NEEDS(1);
    char buff[36];

    long val = g->Pop();
    sprintf( buff, "%d", val );
    TRACE( "printed %s\n", buff );

    stringOut( g, buff );
}

FORTHOP( printNumHexOp )
{
    NEEDS(1);
    char buff[12];

    long val = g->Pop();
    sprintf( buff, "%x", val );
    TRACE( "printed %s\n", buff );

    stringOut( g, buff );
}

FORTHOP( printFloatOp )
{
    NEEDS(1);
    char buff[36];

    float fval = g->FPop();
    sprintf( buff, "%f", fval );
    TRACE( "printed %s\n", buff );

    stringOut( g, buff );
}

FORTHOP( printDoubleOp )
{
    NEEDS(2);
    char buff[36];
    double dval = g->DPop();

    sprintf( buff, "%f", dval );
    TRACE( "printed %s\n", buff );

    stringOut( g, buff );
}

FORTHOP( printStrOp )
{
    NEEDS(1);
    char *buff = (char *) g->Pop();
    TRACE( "printed %s\n", buff );

    stringOut( g, buff );
}

FORTHOP( printCharOp )
{
    NEEDS(1);
    char buff[4];
    char c = (char) g->Pop();
    TRACE( "printed %c\n", c );

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
    g->SetConOutFile( (FILE *) g->Pop() );
}

FORTHOP( outToStringOp )
{
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

#define OP( func, funcName )  { funcName, kOpBuiltIn, (ulong) func }

// ops which have precedence (execute at compile time)
#define PRECOP( func, funcName )  { funcName, kOpBuiltInPrec, (ulong) func }

// NOTE: the order of the first few entries in this table must agree
// with the list near the top of the file!  (look for COMPILED_OP)

baseDictEntry baseDict[] = {
    ///////////////////////////////////////////
    //  STUFF WHICH IS COMPILED BY OTHER WORDS
    //   DO NOT REARRANGE UNDER PAIN OF DEATH
    ///////////////////////////////////////////
    OP(     abortOp,                "abort" ),
    OP(     unravelFrameOp,         "_unravelFrame" ),
    OP(     doDoesOp,               "_doDoes"),
    OP(     exitOp,                 "exit"),
    OP(     litOp,                  "lit" ),
    OP(     litOp,                  "flit" ),
    OP(     dlitOp,                 "dlit" ),
    OP(     doVariableOp,           "_doVariable" ),
    OP(     doConstantOp,           "_doConstant" ),
    OP(     endBuildsOp,            "_endBuilds" ),
    OP(     doneOp,                 "_done" ),
    OP(     doIntOp,                "_doInt" ),
    OP(     doFloatOp,              "_doFloat" ),
    OP(     doDoubleOp,             "_doDouble" ),
    OP(     doStringOp,             "_doString" ),
    OP(     intoOp,                 "->" ),
    
    // stuff below this line can be rearranged
    
    ///////////////////////////////////////////
    //  integer math
    ///////////////////////////////////////////
    OP(     plusOp,                 "+" ),
    OP(     minusOp,                "-" ),
    OP(     timesOp,                "*" ),
    OP(     divideOp,               "/" ),
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
    OP(     doOp,                   "do" ),
    OP(     loopOp,                 "loop" ),
    OP(     loopnOp,                "loop+" ),
    OP(     iOp,                    "i" ),
    OP(     jOp,                    "j" ),
    PRECOP( ifOp,                   "if" ),
    PRECOP( elseOp,                 "else" ),
    PRECOP( endifOp,                "endif" ),
    PRECOP( beginOp,                "begin" ),
    PRECOP( untilOp,                "until" ),
    PRECOP( whileOp,                "while" ),
    PRECOP( repeatOp,               "repeat" ),
    PRECOP( againOp,                "again" ),
    OP(     doneOp,                 "done" ),

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
    OP(     greaterThanOp,          ">" ),
    OP(     greaterEqualsOp,        ">=" ),
    OP(     lessThanOp,             "<" ),
    OP(     lessEqualsOp,           "<=" ),
    OP(     equalsZeroOp,           "==0" ),
    OP(     greaterThanZeroOp,      ">0" ),
    OP(     greaterEqualsZeroOp,    ">=0" ),
    OP(     lessThanZeroOp,         "<0" ),
    OP(     lessEqualsZeroOp,       "<=0" ),
    
    ///////////////////////////////////////////
    //  stack manipulation
    ///////////////////////////////////////////
    OP(     rpushOp,                "r<" ),
    OP(     rpopOp,                 "r>" ),
    OP(     rdropOp,                "rdrop" ),
    OP(     dupOp,                  "dup" ),
    OP(     swapOp,                 "swap" ),
    OP(     dropOp,                 "drop" ),
    OP(     rotOp,                  "rot" ),
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
    OP(     stateOp,                "state" ),
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

    ///////////////////////////////////////////
    //  defining words
    ///////////////////////////////////////////
    OP(     buildsOp,               "builds" ),
    PRECOP( doesOp,                 "does" ),
    PRECOP( semiOp,                 ";" ),
    OP(     colonOp,                ":" ),
    OP(     createOp,               "create" ),
    OP(     variableOp,             "variable" ),
    OP(     constantOp,             "constant" ),
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
    OP(     stateInterpretOp,       "[" ),
    OP(     stateCompileOp,         "]" ),
    OP(     stateOp,                "state" ),
    OP(     tickOp,                 "\'" ),
    OP(     executeOp,              "execute" ),

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
    OP(     feofOp,                 "feof" ),
    OP(     ftellOp,                "ftell" ),
    OP(     stdinOp,                "stdin" ),
    OP(     stdoutOp,               "stdout" ),
    OP(     stderrOp,               "stderr" ),
    
    OP(     byeOp,                  "bye" ),



    // following must be last in table
    OP(     NULL,                   "" )
};

//############################################################################
//
//  The end of all things...
//
//############################################################################


