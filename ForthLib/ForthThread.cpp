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
{
    mState.SLen = paramStackLongs;
    mState.RLen = returnStackLongs;
    mState.pPrivate = NULL;
    mState.pErrorString = NULL;
    // leave a few extra words above top of stacks, so that underflows don't
    //   tromp on the memory allocator info
    mState.SB = new long[mState.SLen + (GAURD_AREA * 2)];
    mState.SB += GAURD_AREA;
    mState.ST = mState.SB + mState.SLen;

    mState.RB = new long[mState.RLen + (GAURD_AREA * 2)];
    mState.RB += GAURD_AREA;
    mState.RT = mState.RB + mState.RLen;

    Reset();
}

ForthThread::~ForthThread()
{
    delete [] (mState.SB - GAURD_AREA);
    delete [] (mState.RB - GAURD_AREA);
}

void
ForthThread::Reset( void )
{
    EmptySStack();
    EmptyRStack();
    mState.FP = NULL;
    mState.TP = NULL;

    mState.error = kForthErrorNone;
    mState.state = kResultDone;
    mState.varMode = kVarFetch;
    mState.base = 10;
    mState.signedPrintMode = kPrintSignedDecimal;
    mState.pConOutFile = stdout;
    mState.pConOutStr = NULL;
}


static char *pErrorStrings[] =
{
    "No Error",
    "Bad Opcode",
    "Bad OpcodeType",
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
    mState.state = kResultError;
    mState.error = e;
    mState.pErrorString = pString;
}

void
ForthThread::SetFatalError( eForthError e, const char *pString )
{
    mState.state = kResultFatalError;
    mState.error = e;
    mState.pErrorString = pString;
}

void
ForthThread::GetErrorString( char *pString )
{
    int errorNum = (int) mState.error;
    if ( errorNum < (sizeof(pErrorStrings) / sizeof(char *)) ) {
        if ( mState.pErrorString != NULL )
        {
            sprintf( pString, "%s: %s", pErrorStrings[errorNum], mState.pErrorString );
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


void
ForthThread::Activate( ForthCoreState& core )
{
    core.IP = mState.IP;
    core.SP = mState.SP;
    core.RP = mState.RP;
    core.FP = mState.FP;
    core.ST = mState.ST;
    core.RT = mState.RT;
    core.varMode = mState.varMode;
    core.state = mState.state;
    core.pThread = &mState;
}


void ForthThread::Deactivate( ForthCoreState& core )
{
    mState.IP = core.IP;
    mState.SP = core.SP;
    mState.RP = core.RP;
    mState.FP = core.FP;
    // NOTE: we don't copy RT & ST back into thread - they should not change
    mState.varMode = core.varMode;
    mState.state = core.state;
}


