// ForthThread.cpp: implementation of the ForthThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthThread.h"
#include "ForthEngine.h"

// this is the number of extra longs to allocate at top and
//    bottom of stacks
#define GAURD_AREA 4

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ForthThread::ForthThread( ForthEngine *pEngine, int paramStackLongs, int returnStackLongs )
: mpEngine( pEngine )
, mSLen( paramStackLongs )
, mRLen( returnStackLongs )
, mpPrivate( NULL )
, mpErrorString( NULL )
{
    // leave a few extra words above top of stacks, so that underflows don't
    //   tromp on the memory allocator info
    mSB = new long[mSLen + (GAURD_AREA * 2)];
    mSB += GAURD_AREA;
    mST = mSB + mSLen;

    mRB = new long[mRLen + (GAURD_AREA * 2)];
    mRB += GAURD_AREA;
    mRT = mRB + mRLen;

    Reset();
}

ForthThread::~ForthThread()
{
    delete [] (mSB - GAURD_AREA);
    delete [] (mRB - GAURD_AREA);
}

void
ForthThread::Reset( void )
{
    EmptySStack();
    EmptyRStack();
    mFP = NULL;
    mTP = NULL;

    mError = kForthErrorNone;
    mState = kResultDone;
    mVarMode = kVarFetch;
    mBase = 10;
    mSignedPrintMode = kPrintSignedDecimal;
    mpConOutFile = stdout;
    mpConOutStr = NULL;
}


static char *pErrorStrings[] =
{
    "No Error",
    "Bad Opcode",
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
ForthThread::SetError( eForthError e, const char *pString )
{
    mState = kResultError;
    mError = e;
    mpErrorString = pString;
}

void
ForthThread::SetFatalError( eForthError e, const char *pString )
{
    mState = kResultFatalError;
    mError = e;
    mpErrorString = pString;
}

void
ForthThread::GetErrorString( char *pString )
{
    int errorNum = (int) mError;
    if ( errorNum < (sizeof(pErrorStrings) / sizeof(char *)) ) {
        if ( mpErrorString != NULL )
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


//
// ExecuteOneOp is used by the Outer Interpreter (ForthEngine::ProcessToken) to
// execute forth ops, and is also how systems external to forth execute ops
//
eForthResult
ForthThread::ExecuteOneOp( long opCode )
{
    long opScratch[2];
    long *savedIP;
    eForthResult exitStatus = kResultOk;

    opScratch[0] = opCode;
    opScratch[1] = BUILTIN_OP( OP_DONE );

    savedIP = mIP;
    mIP = opScratch;
    exitStatus = mpEngine->InnerInterpreter( this );
    mIP = savedIP;

    return exitStatus;
}


eForthResult
ForthThread::CheckStacks( void )
{
    long depth;
    eForthResult result = kResultOk;

    // check parameter stack for over/underflow
    depth = GetSDepth();
    if ( depth < 0 ) {
        SetError( kForthErrorParamStackUnderflow );
        result = kResultError;
    } else if ( depth >= GetSSize() ) {
        SetError( kForthErrorParamStackOverflow );
        result = kResultError;
    }
    
    // check return stack for over/underflow
    depth = GetRDepth();
    if ( depth < 0 ) {
        SetError( kForthErrorReturnStackUnderflow );
        result = kResultError;
    } else if ( depth >= GetRSize() ) {
        SetError( kForthErrorReturnStackOverflow );
        result = kResultError;
    }

    return result;
}


