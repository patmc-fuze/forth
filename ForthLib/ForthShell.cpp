// ForthShell.cpp: implementation of the ForthShell class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthShell.h"

#define STORAGE_LONGS 65536
#define VOCAB_LONGS 16384
#define MAX_USER_OPS 1024

#define PSTACK_LONGS 8192
#define RSTACK_LONGS 8192

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ForthShell::ForthShell( ForthEngine *pEngine, ForthThread *pThread )
{
    if ( pEngine == NULL ) {
        pEngine = new ForthEngine();
        pEngine->Initialize( STORAGE_LONGS, VOCAB_LONGS, MAX_USER_OPS, true );
        mbCreatedEngine = true;
    } else {
        mbCreatedEngine = false;
    }
    mpEngine = pEngine;
    mpEngine->SetShell( this );

    if ( pThread == NULL ) {
        pThread = new ForthThread( mpEngine, PSTACK_LONGS, RSTACK_LONGS );
        mbCreatedThread = true;
    } else {
        mbCreatedThread = false;
    }
    mpThread = pThread;

    mpInStream = NULL;
}

ForthShell::~ForthShell()
{
    if ( mbCreatedEngine ) {
        delete mpEngine;
    }
    if ( mbCreatedThread ) {
        delete mpThread;
    }

    // TBD: should we be closing file here?
    while ( mpInStream != NULL ) {
        ForthInputStream *pNextStream = mpInStream->pNext;
        delete [] mpInStream->pBuffer;
        if ( mpInStream->pInFile != NULL ) {
            fclose( mpInStream->pInFile );
        }
        delete mpInStream;
        mpInStream = pNextStream;
    }
}

void
ForthShell::PushInputStream( FILE *pInFile )
{
    ForthInputStream *pOldStream;
    
    assert( pInFile != NULL );

    pOldStream = mpInStream;
    mpInStream = new ForthInputStream;
    mpInStream->pInFile = pInFile;
    mpInStream->pNext = pOldStream;
    mpInStream->pBuffer = new char [ DEFAULT_INPUT_BUFFER_LEN ];
    mpInStream->bufferLen = DEFAULT_INPUT_BUFFER_LEN;
    mpInStream->bufferOffset = 0;
}


bool
ForthShell::PopInputStream( void )
{
    ForthInputStream *pNext;

    if ( mpInStream->pInFile != NULL ) {
        fclose( mpInStream->pInFile );
        mpInStream->pInFile = NULL;
    }
    if ( mpInStream->pNext == NULL ) {
        // all done!
        return true;
    } else {
        // this stream is done, pop out to previous input stream
        pNext = mpInStream->pNext;
        delete [] mpInStream->pBuffer;
        delete mpInStream;
        mpInStream = pNext;
    }
    return false;
}

//
// interpret named file, interpret from standard in if
//   pFileName is NULL
// return 0 for normal exit
//
int
ForthShell::Run( char *pFileName )
{
    char *pBuffer;
    int retVal = 0;
    bool bQuit = false;
    eForthResult result = kResultOk;
    bool bInteractiveMode = (pFileName == NULL);

    mpInStream = new ForthInputStream;

    mpInStream->pNext = NULL;
    mpInStream->pBuffer = new char [ DEFAULT_INPUT_BUFFER_LEN ];
    mpInStream->bufferLen = DEFAULT_INPUT_BUFFER_LEN;
    mpInStream->bufferOffset = 0;

    if ( bInteractiveMode ) {

        // interactive mode
        mpInStream->pInFile = NULL;

    } else {

        // get input from named file
        mpInStream->pInFile = fopen( pFileName, "r" );
        if ( mpInStream->pInFile == NULL ) {
            // failure!
            delete [] mpInStream->pBuffer;
            delete mpInStream;
            return 1;
        }

    }

    while ( !bQuit ) {

        // try to fetch a line from current stream
        if ( mpInStream->pInFile != NULL ) {
            pBuffer = fgets( mpInStream->pBuffer, mpInStream->bufferLen, mpInStream->pInFile );
        } else {
            printf( "\nok> " );
            pBuffer = gets( mpInStream->pBuffer );
        }

        if ( pBuffer == NULL ) {
            bQuit = PopInputStream();
        } else {
            mpInStream->bufferOffset = 0;
        }

        if ( !bQuit ) {


            result = InterpretLine( &retVal );


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
    
    delete [] mpInStream->pBuffer;
    delete mpInStream;
    mpInStream = NULL;

    return retVal;
}

//
// return true IFF the forth shell should exit
//
eForthResult
ForthShell::InterpretLine( int *pExitCode )
{
    eForthResult  result = kResultOk;
    bool bLineEmpty;
    int parseFlags;
    char tokenBuf[256], *pEndLine;
    long depth;

    // TBD: set exit code on exit due to error

    // get rid of the linefeed (if any)
    pEndLine = strchr( mpInStream->pBuffer, '\n' );
    if ( pEndLine ) {
        *pEndLine = '\0';
    }

    TRACE( "**** InterpretLine: %s\n", mpInStream->pBuffer );
    bLineEmpty = false;
    mpThread->SetError( kForthErrorNone );
    while ( !bLineEmpty && (result == kResultOk) ) {
        bLineEmpty = ParseToken( tokenBuf, &parseFlags );

        if ( !bLineEmpty ) {

            result = mpEngine->ProcessToken( mpThread, tokenBuf, parseFlags );
            if ( result == kResultOk ) {
            
                // check parameter stack for over/underflow
                depth = mpThread->GetSDepth();
                if ( depth < 0 ) {
                    mpThread->SetError( kForthErrorParamStackUnderflow );
                    result = kResultError;
                } else if ( depth >= mpThread->GetSSize() ) {
                    mpThread->SetError( kForthErrorParamStackOverflow );
                    result = kResultError;
                }
                
                // check return stack for over/underflow
                depth = mpThread->GetRDepth();
                if ( depth < 0 ) {
                    mpThread->SetError( kForthErrorReturnStackUnderflow );
                    result = kResultError;
                } else if ( depth >= mpThread->GetRSize() ) {
                    mpThread->SetError( kForthErrorReturnStackOverflow );
                    result = kResultError;
                }
            }
            if ( result != kResultOk ) {
                ReportError();
                mpThread->Reset();
                // dump all nested input stream
                while ( mpInStream->pNext != NULL ) {
                    PopInputStream();
                }
                // if the initial input stream was a file, any error
                //   must be treated as a fatal error
                if ( mpInStream->pInFile != NULL ) {
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

    switch( mpThread->GetError() ) {
    case kForthErrorNone:
        sprintf( errorBuf, "No Error" );
        break;
    case kForthErrorBadOpcode:
        sprintf( errorBuf, "Bad Opcode" );
        break;
    case kForthErrorParamStackUnderflow:
        sprintf( errorBuf, "Parameter Stack Underflow" );
        break;
    case kForthErrorParamStackOverflow:
        sprintf( errorBuf, "Parameter Stack Overflow" );
        break;
    case kForthErrorReturnStackUnderflow:
        sprintf( errorBuf, "Return Stack Underflow" );
        break;
    case kForthErrorReturnStackOverflow:
        sprintf( errorBuf, "Return Stack Overflow" );
        break;
    case kForthErrorUnknownSymbol:
        sprintf( errorBuf, "Unknown Symbol" );
        break;
    case kForthErrorFileOpen:
        sprintf( errorBuf, "File Open Failed" );
        break;
    case kForthErrorAbort:
        sprintf( errorBuf, "Aborted" );
        break;
    default:
        sprintf( errorBuf, "Unknown Error %d", (int) mpThread->GetError() );
        break;
    }
    TRACE( "%s!, last input token: %s\n", errorBuf, mpEngine->GetLastInputToken() );
    printf( "%s!, last input token: %s\n", errorBuf, mpEngine->GetLastInputToken() );
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
ForthParseDoubleQuote( char  *pBuffer,
                       char  *pDst )
{
    char *pToken;

    pToken = pBuffer + 1;
    while ( *pToken != '\0' ) {
        switch ( *pToken ) {

        case '"':
            *pDst = 0;
            return pToken + 1;

        case '\\':
            pToken++;
            *pDst++ = backslashChar( *pToken );
            break;

        default:
            *pDst++ = *pToken;
            break;

        }
        pToken++;
    }
    return pToken;
}


// return true IFF done parsing line
bool
ForthShell::ParseToken( char               *pTokenBuffer,
                        int                *pParseFlags )
{
    char *pToken, *pEndToken;
    *pParseFlags = 0;

    pToken = mpInStream->pBuffer + mpInStream->bufferOffset;
    if ( *pToken == '\0' ) {
        // input buffer is empty
        return true;
    }

    *pTokenBuffer = 0;

    // eat any leading white space
    while ( (*pToken == ' ') || (*pToken == '\t') ) {
        pToken++;
    }

    // support C++ end-of-line style comments
    if ( (*pToken == '/') && (pToken[1] == '/') ) {
        return true;
    }

    // parse symbol up till next white space
    pEndToken = pToken;
    switch ( *pToken ) {

    case '\"':
        // support C-style quoted strings...
        *pParseFlags |= PARSE_FLAG_QUOTED_STRING;
        pEndToken = ForthParseDoubleQuote( pToken, pTokenBuffer );
        break;

    case '\'':
        // support C-style quoted characters like 'a' or '\n'
        if ( (pToken[1] != 0) && (pToken[2] != 0) ) {
            if ( pToken[1] == '\\' ) {
                // special backslashed character constant
                if ( (pToken[3] == '\'') &&
                    ((pToken[4] == 0) || (pToken[4] == ' ') || (pToken[4] == '\t')) ) {
                    *pParseFlags |= PARSE_FLAG_QUOTED_CHARACTER;
                    *pTokenBuffer++ = backslashChar( pToken[2] );
                    *pTokenBuffer++ = 0;
                    pEndToken = pToken + 4;
                }
            } else {
                // regular character constant
                if ( (pToken[2] == '\'') &&
                    ((pToken[3] == 0) || (pToken[3] == ' ') || (pToken[3] == '\t')) ) {
                    *pParseFlags |= PARSE_FLAG_QUOTED_CHARACTER;
                    *pTokenBuffer++ = pToken[1];
                    *pTokenBuffer++ = 0;
                    pEndToken = pToken + 3;
                }
            }
        } 
        break;

    }
    if ( *pParseFlags == 0 ) {
        // token is not a special case, just parse till blank, space or EOL
        while ( (*pEndToken != ' ') && (*pEndToken != '\t')
            && (*pEndToken != '\0') ) {
            *pTokenBuffer++ = *pEndToken++;
        }
        *pTokenBuffer++ = 0;
    }
    mpInStream->bufferOffset = pEndToken - mpInStream->pBuffer;

    return false;
}


char *
ForthShell::GetNextSimpleToken( void )
{
    char *pToken = mpInStream->pBuffer + mpInStream->bufferOffset;
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
    mpInStream->bufferOffset = pEndToken - mpInStream->pBuffer;

    return pToken;
}


