//////////////////////////////////////////////////////////////////////
//
// ForthShell.cpp: implementation of the ForthShell class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include "ForthVocabulary.h"

#define STORAGE_LONGS 65536

#define PSTACK_LONGS 8192
#define RSTACK_LONGS 8192

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ForthShell::ForthShell( ForthEngine *pEngine, ForthThread *pThread )
: mpEngine(pEngine)
, mpThread(pThread)
, mbCreatedEngine(false)
{
    if ( mpEngine == NULL ) {
        mpEngine = new ForthEngine();
        mpEngine->Initialize( STORAGE_LONGS, true );
        mbCreatedEngine = true;
    }
    mpEngine->SetShell( this );

    if ( mpThread == NULL ) {
        mpThread = mpEngine->CreateThread( PSTACK_LONGS, RSTACK_LONGS, NULL );
    }

    mpInput = new ForthInputStack;

}

ForthShell::~ForthShell()
{
    // engine will destroy thread for us if we created it
    if ( mbCreatedEngine ) {
        delete mpEngine;
    }

    delete mpInput;
}


//
// create a new file input stream & push on stack
//
void
ForthShell::PushInputStream( FILE *pInFile )
{
    assert( pInFile != NULL );

    mpInput->PushInputStream( new ForthFileInputStream(pInFile) );
}


bool
ForthShell::PopInputStream( void )
{
    return mpInput->PopInputStream();
}


//
// interpret named file, interpret from standard in if
//   pFileName is NULL
// return 0 for normal exit
//
int
ForthShell::Run( ForthInputStream *pInStream )
{
    char *pBuffer;
    int retVal = 0;
    bool bQuit = false;
    eForthResult result = kResultOk;
    bool bInteractiveMode = pInStream->IsInteractive();

    mpInput->PushInputStream( pInStream );

    while ( !bQuit ) {

        // try to fetch a line from current stream
        pBuffer = mpInput->GetLine( "ok>" );
        if ( pBuffer == NULL ) {
            bQuit = PopInputStream();
        }

        if ( !bQuit ) {


            result = InterpretLine();


            switch( result ) {

            case kResultExitShell:
                // users has typed "bye", exit the shell
                bQuit = true;
                retVal = 0;
                break;

            case kResultError:
                // an error has occured, empty input stream stack
                // TBD
                if ( !bInteractiveMode ) {
                    bQuit = true;
                } else {
                    // TBD: dump all but outermost input stream
                }
                retVal = 0;
                break;

            case kResultFatalError:
                // a fatal error has occured, exit the shell
                bQuit = true;
                retVal = 1;
                break;
            }

        }
    }
    
    return retVal;
}

//
// return true IFF the forth shell should exit
//
eForthResult
ForthShell::InterpretLine( void )
{
    eForthResult  result = kResultOk;
    bool bLineEmpty;
    ForthParseInfo parseInfo( mTokenBuffer, sizeof(mTokenBuffer) );

    char *pLineBuff;

    // TBD: set exit code on exit due to error

    pLineBuff = mpInput->GetBufferPointer();

#ifdef TRACE_SHELL
    TRACE( "*** InterpretLine \"%s\"\n", pLineBuff );
#endif
    bLineEmpty = false;
    mpThread->SetError( kForthErrorNone );
    while ( !bLineEmpty && (result == kResultOk) ) {
        bLineEmpty = ParseToken( &parseInfo );

        if ( !bLineEmpty ) {

            result = mpEngine->ProcessToken( mpThread, &parseInfo );
            if ( result == kResultOk ) {
                result = mpThread->CheckStacks();
            }
            if ( result != kResultOk ) {
                if ( result != kResultExitShell ) {
                    ReportError();
                }
                mpEngine->Reset();
                mpThread->Reset();
                mpInput->Reset();
                if ( !mpInput->InputStream()->IsInteractive() ) {
                    // if the initial input stream was a file, any error
                    //   must be treated as a fatal error
                    result = kResultFatalError;
                }
            }
           
        }
    }

    return result;
}

void
ForthShell::ReportError( void )
{
    char errorBuf[256];
    char *pLastInputToken;

    mpThread->GetErrorString( errorBuf );
    pLastInputToken = mpEngine->GetLastInputToken();
    if ( pLastInputToken != NULL ) {
        TRACE( "%s!, last input token: %s\n", errorBuf, pLastInputToken );
        printf( "%s!, last input token: %s\n", errorBuf, pLastInputToken );
    } else {
        TRACE( "%s!\n", errorBuf );
        printf( "%s!\n", errorBuf );
    }
}

static char
backslashChar( char c )
{
    char cResult;

    switch( c ) {

    case 'a':        cResult = '\a';        break;
    case 'b':        cResult = '\b';        break;
    case 'f':        cResult = '\f';        break;
    case 'n':        cResult = '\n';        break;
    case 'r':        cResult = '\r';        break;
    case 't':        cResult = '\t';        break;
    case 'v':        cResult = '\v';        break;
    case '\\':       cResult = '\\';        break;
    case '\?':       cResult = '\?';        break;
    case '\'':       cResult = '\'';        break;
    case '\"':       cResult = '\"';        break;

    default:         cResult = c;           break;

    }
    return cResult;
}


static char *
ForthParseSingleQuote( char             *pSrc,
                       ForthParseInfo   *pInfo )
{
    char cc[2];

    if ( (pSrc[1] != 0) && (pSrc[2] != 0) ) {
        if ( pSrc[1] == '\\' ) {
            // special backslashed character constant
            if ( (pSrc[3] == '\'') &&
                ((pSrc[4] == 0) || (pSrc[4] == ' ') || (pSrc[4] == '\t')) ) {
                pInfo->SetFlag( PARSE_FLAG_QUOTED_CHARACTER );
                cc[0] = backslashChar( pSrc[2] );
                cc[1] = '\0';
                pSrc += 4;
                pInfo->SetToken( cc );
            }
        } else {
            // regular character constant
            if ( (pSrc[2] == '\'') &&
                ((pSrc[3] == 0) || (pSrc[3] == ' ') || (pSrc[3] == '\t')) ) {
                pInfo->SetFlag( PARSE_FLAG_QUOTED_CHARACTER );
                cc[0] = pSrc[1];
                cc[1] = '\0';
                pSrc += 3;
                pInfo->SetToken( cc );
            }
        }
    }
    return pSrc;
}


static char *
ForthParseDoubleQuote( char             *pSrc,
                       ForthParseInfo   *pInfo )
{
    char  *pDst = pInfo->GetToken();

    pInfo->SetFlag( PARSE_FLAG_QUOTED_STRING );

    pSrc++;  // skip first double-quote
    while ( *pSrc != '\0' ) {

        switch ( *pSrc ) {

        case '"':
            *pDst = 0;
            // set token length byte
            pInfo->SetToken();
            return pSrc + 1;

        case '\\':
            pSrc++;
            *pDst++ = backslashChar( *pSrc );
            break;

        default:
            *pDst++ = *pSrc;
            break;

        }

        pSrc++;
    }
    *pDst = 0;
    pInfo->SetToken();
    return pSrc;
}


// return true IFF done parsing line
// TBD!!! there is nothing to keep us from writing past end of pTokenBuffer
bool
ForthShell::ParseToken( ForthParseInfo *pInfo )
{
    char *pSrc, *pEndSrc;
    char *pDst = pInfo->GetToken();

    pInfo->SetAllFlags( 0 );

    pSrc = mpInput->GetBufferPointer();
    if ( *pSrc == '\0' ) {
        // input buffer is empty
        return true;
    }

    *pDst = 0;

    // eat any leading white space
    while ( (*pSrc == ' ') || (*pSrc == '\t') ) {
        pSrc++;
    }

    // support C++ end-of-line style comments
    if ( (*pSrc == '/') && (pSrc[1] == '/') ) {
        return true;
    }

    // parse symbol up till next white space
    if ( *pSrc == '\"' ) {

        // support C-style quoted strings...
        pEndSrc = ForthParseDoubleQuote( pSrc, pInfo );

    } else if ( *pSrc == '\'' ) {

        // support C-style quoted characters like 'a' or '\n'
        pEndSrc = ForthParseSingleQuote( pSrc, pInfo );

    }

    if ( pInfo->GetFlags() == 0 ) {
        // token is not a special case, just parse till blank, space or EOL
        pEndSrc = pSrc;
        while ( (*pEndSrc != ' ') && (*pEndSrc != '\t')
            && (*pEndSrc != '\0') ) {
            *pDst++ = *pEndSrc++;
        }
        *pDst++ = '\0';
        // set token length byte
        pInfo->SetToken();
    }
    mpInput->SetBufferPointer( pEndSrc );

    return false;
}


char *
ForthShell::GetNextSimpleToken( void )
{
    char *pToken = mpInput->GetBufferPointer();
    char *pEndToken, c;
    bool bDone;

    // eat any leading white space
    while ( (*pToken == ' ') || (*pToken == '\t') ) {
        pToken++;
    }

    pEndToken = pToken;
    bDone = false;
    while ( !bDone ) {
        c = *pEndToken;
        switch( c ) {
        case ' ':
        case '\t':
            bDone = true;
            *pEndToken++ = '\0';
            break;
        case '\0':
            // symbol is last on line, don't go past terminator
            bDone = true;
            break;
        default:
            pEndToken++;
        }
    }
    mpInput->SetBufferPointer( pEndToken );

    return pToken;
}


ForthParseInfo::ForthParseInfo( long *pBuffer, int numLongs )
: mpToken( pBuffer )
, mMaxChars( (numLongs << 2) - 2 )
, mFlags(0)
, mNumLongs(0)
{
    ASSERT( numLongs > 0 );

    // zero length byte, and first char
    *mpToken = 0;
}


ForthParseInfo::~ForthParseInfo()
{
    // NOTE: don't delete mpToken, it doesn't belong to us
}


void
ForthParseInfo::SetToken( char *pSrc )
{
    int symLen, padChars;
    char *pDst;

    if ( pSrc != NULL ) {

        symLen = strlen( pSrc );
        pDst = (char *) mpToken;

        if ( symLen > mMaxChars ) {
            symLen = mMaxChars;
        }
        // set length byte
        *pDst++ = symLen;

        // make copy of symbol
        memcpy( pDst, pSrc, symLen );
        pDst += symLen;
        *pDst++ = '\0';

    } else {

        // token has already been copied to mpToken, just set length byte
        symLen = strlen( ((char *) mpToken) + 1 );
        *((char *) mpToken) = symLen;
        pDst = ((char *) mpToken) + symLen + 2;
    }

    // in diagram, first char is length byte, 'a' are symbol chars, '0' is terminator, '#' is padding
    //
    //            symLen     padding nLongs
    // 1a0#|        1           1       1
    // 2aa0|        2           0       1
    // 3aaa|0       3           0       1
    // 4aaa|a0##    4           2       2
    // 5aaa|aa0#    5           1       2
    //
    mNumLongs = (symLen + 4) >> 2;


    padChars = (symLen + 2) & 3;
    if ( padChars > 1 ) {
        padChars = 4 - padChars;
        while ( padChars > 0 ) {
            *pDst++ = '\0';
            padChars--;
        }
    }
}


