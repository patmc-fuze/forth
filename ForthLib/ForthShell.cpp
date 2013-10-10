//////////////////////////////////////////////////////////////////////
//
// ForthShell.cpp: implementation of the ForthShell class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#ifdef LINUX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <dirent.h>
#else
#include <io.h>
#include "dirent.h"
#endif
#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include "ForthVocabulary.h"
#include "ForthExtension.h"

#define CATCH_EXCEPTIONS

#define STORAGE_LONGS 65536

#define PSTACK_LONGS 8192
#define RSTACK_LONGS 8192

namespace
{
    const char * TagStrings[] =
    {
        "NOTHING",
        "do",
        "begin",
        "while",
        "case/of",
        "if",
        "else",
        "paren",
        "string",
        "colon",
        "poundDirective",
		"of",
		"ofif"
    };

    const char * GetTagString( long tag )
    {
        static char msg[28];

        if ( tag < kNumShellTags )
        {
            return TagStrings[ tag ];
        }
        sprintf( msg, "UNKNOWN TAG 0x%x", tag );
        return msg;
    }

    int fileExists( const char* pFilename )
    {
		FILE* pFile = NULL;
		pFile = fopen( pFilename, "r" );
        int result = (pFile != 0) ? ~0 : 0;
        if ( pFile != NULL )
        {
            fclose( pFile );
        }
        return result;
    }

    int fileGetLength( FILE* pFile )
    {
        int oldPos = ftell( pFile );
        fseek( pFile, 0, SEEK_END );
        int result = ftell( pFile );
        fseek( pFile, oldPos, SEEK_SET );
        return result;
    }

	FILE* getStdIn()
	{
		return stdin;
	}

	FILE* getStdOut()
	{
		return stdout;
	}

	FILE* getStdErr()
	{
		return stderr;
	}

	int makeDir( const char* pPath, int mode )
	{
#ifdef WIN32
		return mkdir( pPath );
#else
		return mkdir( pPath, mode );
#endif
	}
}

// return is a DIR*
void* openDir( const char* pPath )
{
	return opendir( pPath );
}

// return is a struct dirent*
void* readDir( void* pDir )
{
	return readdir( (DIR*) pDir );
}

int closeDir( void* pDir )
{
	return closedir( (DIR*) pDir );
}

void rewindDir( void* pDir )
{
	rewinddir( (DIR*) pDir );
}

#if defined(WIN32)
DWORD WINAPI ConsoleInputThreadRoutine( void* pThreadData );
#elif defined(LINUX)
unsigned long ConsoleInputThreadRoutine( void* pThreadData );
#endif

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthShell
// 

ForthShell::ForthShell( ForthEngine *pEngine, ForthExtension *pExtension, ForthThread *pThread, int shellStackLongs )
: mpEngine(pEngine)
, mpThread(pThread)
, mFlags(0)
, mNumArgs(0)
, mpArgs(NULL)
, mNumEnvVars(0)
, mpEnvVarNames(NULL)
, mpEnvVarValues(NULL)
, mPoundIfDepth(0)
, mpInternalFiles(NULL)
, mInternalFileCount(0)
{
    if ( mpEngine == NULL )
    {
        mpEngine = new ForthEngine();
        mFlags = SHELL_FLAG_CREATED_ENGINE;
    }
    mpEngine->Initialize( this, STORAGE_LONGS, true, pExtension );

#if 0
    if ( mpThread == NULL )
    {
        mpThread = mpEngine->CreateThread( 0, PSTACK_LONGS, RSTACK_LONGS );
    }
    mpEngine->SetCurrentThread( mpThread );
#endif

    mpInput = new ForthInputStack;
	mpStack = new ForthShellStack( shellStackLongs );

    mFileInterface.fileOpen = fopen;
    mFileInterface.fileClose = fclose;
    mFileInterface.fileRead = fread;
    mFileInterface.fileWrite = fwrite;
    mFileInterface.fileGetChar = fgetc;
    mFileInterface.filePutChar = fputc;
    mFileInterface.fileAtEnd = feof;
    mFileInterface.fileExists = fileExists;
    mFileInterface.fileSeek = fseek;
    mFileInterface.fileTell = ftell;
    mFileInterface.fileGetLength = fileGetLength;
    mFileInterface.fileGetString = fgets;
    mFileInterface.filePutString = fputs;
	mFileInterface.fileRemove = remove;
#if defined( WIN32 )
	mFileInterface.fileDup = _dup;
	mFileInterface.fileDup2 = _dup2;
	mFileInterface.fileNo = _fileno;
#else
	mFileInterface.fileDup = dup;
	mFileInterface.fileDup2 = dup2;
	mFileInterface.fileNo = fileno;
#endif
	mFileInterface.fileFlush = fflush;
	mFileInterface.getTmpnam = tmpnam;
	mFileInterface.renameFile = rename;
	mFileInterface.runSystem = system;
	mFileInterface.changeDir = chdir;
	mFileInterface.makeDir = makeDir;
	mFileInterface.removeDir = rmdir;
	mFileInterface.getStdIn = getStdIn;
	mFileInterface.getStdOut = getStdOut;
	mFileInterface.getStdErr = getStdErr;
	mFileInterface.openDir = openDir;
	mFileInterface.readDir = readDir;
	mFileInterface.closeDir = closeDir;
	mFileInterface.rewindDir = rewindDir;


#if defined( WIN32 )
	DWORD result = GetCurrentDirectory( MAX_PATH, mWorkingDirPath );
	if ( result == 0 )
	{
		mWorkingDirPath[0] = '\0';
	}
#else
#endif

#if 0
    mMainThreadId = GetThreadId( GetMainThread() );
    mConsoleInputThreadId = 0;
    mConsoleInputThreadHandle = _beginthreadex( NULL,		// thread security attribs
                                                0,			// stack size (default)
                                                ConsoleInputThreadLoop,
                                                this,
                                                CREATE_SUSPENDED,
                                                &mConsoleInputThreadId );

    mConsoleInputEvent = CreateEvent( NULL,               // default security attributes
                                      FALSE,              // auto-reset event
                                      FALSE,              // initial state is nonsignaled
                                      TEXT("ConsoleInputEvent") ); // object name

   mpReadyThreads = new ForthThreadQueue;
#endif

}

ForthShell::~ForthShell()
{
    // engine will destroy thread for us if we created it
    if ( mFlags & SHELL_FLAG_CREATED_ENGINE )
    {
        delete mpEngine;
    }

    DeleteEnvironmentVars();
    DeleteCommandLine();
    delete mpInput;
	delete mpStack;
#if 0
    delete mpReadyThreads;
    CloseHandle( mConsoleInputEvent );
#endif
}



//
// create a new file input stream & push on stack
//
bool
ForthShell::PushInputFile( const char *pFilename )
{
    FILE *pInFile = OpenForthFile( pFilename );
    if ( pInFile != NULL )
    {
        mpInput->PushInputStream( new ForthFileInputStream( pInFile ) );
        return true;
    }
    return false;
}


//
// create a new buffer input stream & push on stack
//
void
ForthShell::PushInputBuffer( char *pDataBuffer, int dataBufferLen )
{
    mpInput->PushInputStream( new ForthBufferInputStream( pDataBuffer, dataBufferLen ) );
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
    const char *pBuffer;
    int retVal = 0;
    bool bQuit = false;
    eForthResult result = kResultOk;
    bool bInteractiveMode = pInStream->IsInteractive();

    mpInput->PushInputStream( pInStream );

    const char* autoloadFilename = "app_autoload.txt";
	FILE* pFile = OpenInternalFile( autoloadFilename );
	if ( pFile == NULL )
	{
		// no internal file found, try opening app_autoload.txt as a standard file
		pFile = fopen( autoloadFilename, "r" );
	}
    if ( pFile != NULL )
    {
        // there is an app autoload file, use that
        fclose( pFile );
    }
    else
    {
        // no app autload, try using the normal autoload file
        autoloadFilename = "forth_autoload.txt";
    }
    mpEngine->PushInputFile( autoloadFilename );

    while ( !bQuit )
    {

        // try to fetch a line from current stream
        pBuffer = mpInput->GetLine( mpEngine->GetFastMode() ? "ok>" : "OK>" );
        if ( pBuffer == NULL )
        {
            bQuit = PopInputStream();
        }

        if ( !bQuit )
        {
            result = ProcessLine();

            switch( result )
            {

            case kResultOk:
                break;

            case kResultShutdown:			// what should shutdown do on non-client/server?
            case kResultExitShell:
                // users has typed "bye", exit the shell
                bQuit = true;
                retVal = 0;
                break;

            case kResultError:
            case kResultException:
                // an error has occured, empty input stream stack
                // TBD
                if ( !bInteractiveMode )
                {
                    bQuit = true;
                }
                else
                {
                    // TBD: dump all but outermost input stream
                }
                retVal = 0;
                break;

            case kResultFatalError:
            default:
                // a fatal error has occured, exit the shell
                bQuit = true;
                retVal = 1;
                break;
            }
        }
    } // while !bQuit
    
    return retVal;
}

// ProcessLine is the layer between Run and InterpretLine that implements pound directives
eForthResult ForthShell::ProcessLine( const char *pSrcLine )
{
    eForthResult result = kResultOk;

    char* pLineBuff = mpInput->GetBufferBasePointer();
    if ( pSrcLine != NULL )
	{
		strcpy( pLineBuff, pSrcLine );
		mpInput->SetBufferPointer( pLineBuff );
	}

    if ( (mFlags & SHELL_FLAG_SKIP_SECTION) != 0 )
    {
        // we are currently skipping input lines, check if we should stop skipping
        char* pToken = GetNextSimpleToken();
        if ( pToken != NULL )
        {
            if ( !strcmp( "#if", pToken ) )
            {
                mPoundIfDepth++;
            }
            else if ( !strcmp( "#ifdef", pToken ) )
            {
                mPoundIfDepth++;
            }
            else if ( !strcmp( "#ifndef", pToken ) )
            {
                mPoundIfDepth++;
            }
            else if ( !strcmp( "#else", pToken ) )
            {
                if ( mPoundIfDepth == 0 )
                {
                    mpStack->Push( kShellTagPoundIf );
                    mFlags &= ~SHELL_FLAG_SKIP_SECTION;
                }
            }
            else if ( !strcmp( "#endif", pToken ) )
            {
                if ( mPoundIfDepth == 0 )
                {
                    mFlags &= ~SHELL_FLAG_SKIP_SECTION;
                }
                else
                {
                    mPoundIfDepth--;
                }

            }
        }
    }
    else
    {
        // we are currently not skipping input lines
        result = InterpretLine();

        if ( result == kResultOk )
        {
            // process pound directive if needed
            if ( (mFlags & SHELL_FLAG_START_IF) != 0 )
            {
                // this line started with #if
                ForthCoreState* pCore = mpEngine->GetCoreState();
                if ( GET_SDEPTH > 0 )
                {
                    long expressionResult = SPOP;
                    if ( expressionResult != 0 )
                    {
                        // compile "if" part
                        mpStack->Push( kShellTagPoundIf );
                    }
                    else
                    {
                        // skip to #else or #endif
                        mFlags |= SHELL_FLAG_SKIP_SECTION;
                        mPoundIfDepth = 0;
                    }
                    if ( (mFlags & SHELL_FLAG_START_IF_C) != 0 )
                    {
                        // state was compile before #if, put it back
                        mpEngine->SetCompileState( 1 );
                    }
                }
                else
                {
                    mpEngine->SetError( kForthErrorBadPreprocessorDirective, "#if expression left empty stack" );
                }
                mFlags &= ~SHELL_FLAG_START_IF;
            }
        }
    }
    return result;
}

//
// return true IFF the forth shell should exit
//
eForthResult
ForthShell::InterpretLine( const char *pSrcLine )
{
    eForthResult  result = kResultOk;
    bool bLineEmpty;
    ForthParseInfo parseInfo( mTokenBuffer, sizeof(mTokenBuffer) );

    char *pLineBuff;

    // TBD: set exit code on exit due to error

    pLineBuff = mpInput->GetBufferBasePointer();
    if ( pSrcLine != NULL )
	{
        strcpy( pLineBuff, pSrcLine );
		mpInput->SetBufferPointer( pLineBuff );
	}
    SPEW_SHELL( "*** InterpretLine \"%s\"\n", pLineBuff );
    bLineEmpty = false;
    mpEngine->SetError( kForthErrorNone );
    while ( !bLineEmpty && (result == kResultOk) )
	{
        bLineEmpty = ParseToken( &parseInfo );

        if ( !bLineEmpty )
		{

#ifdef WIN32
#ifdef CATCH_EXCEPTIONS
            try
#endif
#endif
            {
                result = mpEngine->ProcessToken( &parseInfo );
                CHECK_STACKS( mpEngine->GetMainThread() );
            }
#ifdef WIN32
#ifdef CATCH_EXCEPTIONS
            catch(...)
            {
                result = kResultException;
                mpEngine->SetError( kForthErrorException );
            }
#endif
#endif
            if ( result == kResultOk )
			{
                result = mpEngine->CheckStacks();
            }
            if ( result != kResultOk )
			{
				bool exitingShell = (result == kResultExitShell) || (result == kResultShutdown);
                if ( !exitingShell )
				{
                    ReportError();
					mpEngine->DumpCrashState();
                }
				ErrorReset();
                if ( !mpInput->InputStream()->IsInteractive() && !exitingShell )
				{
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
ForthShell::ErrorReset( void )
{
	mpEngine->ErrorReset();
	mpInput->Reset();
	mpStack->EmptyStack();
    mPoundIfDepth = 0;
    mFlags &= SHELL_FLAGS_NOT_RESET_ON_ERROR;
}

void
ForthShell::ReportError( void )
{
    char errorBuf1[512];
    char errorBuf2[512];
    char *pLastInputToken;

    mpEngine->GetErrorString( errorBuf1, sizeof(errorBuf1) );
    pLastInputToken = mpEngine->GetLastInputToken();
	ForthCoreState* pCore = mpEngine->GetCoreState();

	if ( pLastInputToken != NULL )
    {
        sprintf( errorBuf2, "%s, last input token: <%s> last IP 0x%x", errorBuf1, pLastInputToken, pCore->IP );
    }
    else
    {
        sprintf( errorBuf2, "%s", errorBuf1 );
    }
    int lineNumber = mpInput->InputStream()->GetLineNumber();
    if ( lineNumber > 0 )
    {
        sprintf( errorBuf1, "%s at line number %d", errorBuf2, lineNumber );
    }
    else
    {
        strcpy( errorBuf1, errorBuf2 );
    }
    TRACE( "%s", errorBuf1 );
	pCore->consoleOut( pCore, errorBuf1 );
    char *pBase = mpInput->GetBufferBasePointer();
    pLastInputToken = mpInput->GetBufferPointer();
    if ( (pBase != NULL) && (pLastInputToken != NULL) )
    {
		char* pBuf = errorBuf1;
		*pBuf++ = '\n';
        while ( pBase < pLastInputToken )
        {
            *pBuf++ = *pBase++;
        }
		int bufferLimit = sizeof(errorBuf1) - (pBuf - &(errorBuf1[0]));
        sprintf( pBuf, "{}%s\n", pLastInputToken );
    }
	TRACE( "%s", errorBuf1 );
	pCore->consoleOut( pCore, errorBuf1 );
}

static char
backslashChar( char c )
{
    char cResult;

    switch( c )
    {

    case 'a':        cResult = '\a';        break;
    case 'b':        cResult = '\b';        break;
    case 'f':        cResult = '\f';        break;
    case 'n':        cResult = '\n';        break;
    case 'r':        cResult = '\r';        break;
    case 't':        cResult = '\t';        break;
    case 'v':        cResult = '\v';        break;
    case '0':        cResult = '\0';        break;
    case '?':        cResult = '\?';        break;
    case '\\':       cResult = '\\';        break;
    case '\'':       cResult = '\'';        break;
    case '\"':       cResult = '\"';        break;

    default:         cResult = c;           break;

    }
    return cResult;
}


static const char *
ForthParseSingleQuote( const char       *pSrc,
                       ForthParseInfo   *pInfo )
{
    char cc[2];

    if ( (pSrc[1] != 0) && (pSrc[2] != 0) )
    {
        if ( pSrc[1] == '\\' )
        {
            // special backslashed character constant
            if ( (pSrc[3] == '\'') &&
                ((pSrc[4] == 0) || (pSrc[4] == ' ') || (pSrc[4] == '\t')) )
            {
                pInfo->SetFlag( PARSE_FLAG_QUOTED_CHARACTER );
                cc[0] = backslashChar( pSrc[2] );
                cc[1] = '\0';
                pSrc += 4;
                pInfo->SetToken( cc );
            }
        }
        else
        {
            // regular character constant
            if ( (pSrc[2] == '\'') &&
                ((pSrc[3] == 0) || (pSrc[3] == ' ') || (pSrc[3] == '\t')) )
            {
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


static const char *
ForthParseDoubleQuote( const char       *pSrc,
                       ForthParseInfo   *pInfo )
{
    char  *pDst = pInfo->GetToken();

    pInfo->SetFlag( PARSE_FLAG_QUOTED_STRING );

    pSrc++;  // skip first double-quote
    while ( *pSrc != '\0' )
    {

        switch ( *pSrc )
        {

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


// return true IFF done parsing line - in this case no string is returned in pInfo
// this is a stripped down version of ParseToken used just for building string tables
// TBD!!! there is nothing to keep us from writing past end of pTokenBuffer
bool
ForthShell::ParseString( ForthParseInfo *pInfo )
{
    const char *pSrc;
    const char *pEndSrc;
    char *pDst;
    bool gotAToken = false;

    pInfo->SetAllFlags( 0 );

    while ( !gotAToken )
    {

        pSrc = mpInput->GetBufferPointer();
        pDst = pInfo->GetToken();
        if ( *pSrc == '\0' )
        {
            // input buffer is empty
            return true;
        }

        *pDst = 0;

        // eat any leading white space
        while ( (*pSrc == ' ') || (*pSrc == '\t') )
        {
            pSrc++;
        }

        // support C++ end-of-line style comments
        if ( (*pSrc == '/') && (pSrc[1] == '/') )
        {
            return true;
        }

        // parse symbol up till next white space
        switch ( *pSrc )
        {
           case '\"':
              // support C-style quoted strings...
              pEndSrc = ForthParseDoubleQuote( pSrc, pInfo );
              gotAToken = true;
              break;

           default:
              break;
        }

        if ( pInfo->GetFlags() == 0 )
        {
            // token is not a special case, just parse till blank, space or EOL
           bool done = false;
            pEndSrc = pSrc;
            while ( !done )
            {
               switch ( *pEndSrc )
               {
                  case ' ':
                  case '\t':
                  case '\0':
                     done = true;
                     *pDst++ = '\0';
                     // set token length byte
                     pInfo->SetToken();
                     gotAToken = true;
                     break;

                  default:
                     *pDst++ = *pEndSrc++;
                     break;

               }
            } // while not done
        }

        mpInput->SetBufferPointer( (char *) pEndSrc );
    }   // while ! gotAToken

    return false;
}


// return true IFF done parsing line - in this case no token is returned in pInfo
// TBD!!! there is nothing to keep us from writing past end of pTokenBuffer
bool
ForthShell::ParseToken( ForthParseInfo *pInfo )
{
    const char *pSrc;
    const char *pEndSrc;
    char *pDst;
    bool gotAToken = false;

    pInfo->SetAllFlags( 0 );
    if ( mFlags & SHELL_FLAG_POP_NEXT_TOKEN )
    {
        // previous symbol ended in ")", so next token to process is on top of shell stack
        mFlags &= ~SHELL_FLAG_POP_NEXT_TOKEN;
        if ( CheckSyntaxError( ")", mpStack->Pop(), kShellTagParen ) )
        {
            long tag = mpStack->Peek();
            if ( mpStack->PopString( pInfo->GetToken() ) )
            {
                 pInfo->SetToken();
				 pSrc = pInfo->GetToken();

				 // set parseInfo flags for popped token
				 while ( *pSrc != '\0' )
				 {
					 switch ( *pSrc )
					 {
					 case '.':
						 pInfo->SetFlag( PARSE_FLAG_HAS_PERIOD );
						 break;

					 case ':':
						pInfo->SetFlag( PARSE_FLAG_HAS_COLON );
						break;

					 default:
						 break;
					 }

					 pSrc++;
				 }
            }
            else
            {
                sprintf( mErrorString,  "top of shell stack is <%s>, was expecting <string>",
                         GetTagString( tag ) );
                mpEngine->SetError( kForthErrorBadSyntax, mErrorString );
            }
        }
        return false;
    }

    while ( !gotAToken )
    {

        pSrc = mpInput->GetBufferPointer();
        pDst = pInfo->GetToken();
        if ( *pSrc == '\0' )
        {
            // input buffer is empty
            return true;
        }

        *pDst = 0;

        // eat any leading white space
        while ( (*pSrc == ' ') || (*pSrc == '\t') )
        {
            pSrc++;
        }

        // support C++ end-of-line style comments
        if ( (*pSrc == '/') && (pSrc[1] == '/') )
        {
            return true;
        }

        // parse symbol up till next white space
        switch ( *pSrc )
        {
           case '\"':
              // support C-style quoted strings...
              pEndSrc = ForthParseDoubleQuote( pSrc, pInfo );
              gotAToken = true;
              break;

           case '\'':
              // support C-style quoted characters like 'a' or '\n'
              pEndSrc = ForthParseSingleQuote( pSrc, pInfo );
              gotAToken = true;
              break;

           default:
              break;
        }

        if ( pInfo->GetFlags() == 0 )
        {
            // token is not a special case, just parse till blank, space or EOL
           bool done = false;
            pEndSrc = pSrc;
            while ( !done )
            {
               switch ( *pEndSrc )
               {
                  case ' ':
                  case '\t':
                  case '\0':
                     done = true;
                     *pDst++ = '\0';
                     // set token length byte
                     pInfo->SetToken();
                     gotAToken = true;
                     break;

                  case '(':
                     if ( mpEngine->CheckFlag( kEngineFlagParenIsComment ) )
                     {
                        *pDst++ = *pEndSrc++;
                     }
                     else
                     {
                        // push accumulated token (if any) onto shell stack
                        pEndSrc++;
                        *pDst++ = '\0';
                        pDst = pInfo->GetToken();
                        mpStack->PushString( pDst );
                        mpStack->Push( kShellTagParen );
                     }
                     break;

                  case ')':
                     if ( mpEngine->CheckFlag( kEngineFlagParenIsComment ) )
                     {
                        *pDst++ = *pEndSrc++;
                     }
                     else
                     {
                        // process accumulated token (if any), pop shell stack, compile/interpret if not empty
                        pEndSrc++;
                        done = true;
                        *pDst++ = '\0';
                        mFlags |= SHELL_FLAG_POP_NEXT_TOKEN;
                        pInfo->SetToken();
                        gotAToken = true;
                     }
                     break;

                  case '.':
                     pInfo->SetFlag( PARSE_FLAG_HAS_PERIOD );
                     *pDst++ = *pEndSrc++;
                     break;

                  case ':':
                     pInfo->SetFlag( PARSE_FLAG_HAS_COLON );
                     *pDst++ = *pEndSrc++;
                     break;

                  default:
                     *pDst++ = *pEndSrc++;
                     break;

               }
            } // while not done
        }

        mpInput->SetBufferPointer( (char *) pEndSrc );
    }   // while ! gotAToken

    return false;
}


char *
ForthShell::GetNextSimpleToken( void )
{
    char *pToken = mpInput->GetBufferPointer();
    char *pEndToken, c;
    bool bDone;

    // eat any leading white space
    while ( (*pToken == ' ') || (*pToken == '\t') )
    {
        pToken++;
    }

    pEndToken = pToken;
    bDone = false;
    while ( !bDone )
    {
        c = *pEndToken;
        switch( c )
        {
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


char *
ForthShell::GetToken( char delim )
{
    char *pToken = mpInput->GetBufferPointer();
    char *pEndToken, c;
    bool bDone;

    // eat any leading white space
    while ( (*pToken == ' ') || (*pToken == '\t') )
    {
        pToken++;
    }

    pEndToken = pToken;
    bDone = false;
    while ( !bDone )
    {
        c = *pEndToken;
        if ( c == '\0' )
        {
			// don't move input ptr past terminating null
            bDone = true;
            *pEndToken = '\0';
        }
        else if ( c == delim )
        {
            bDone = true;
            *pEndToken++ = '\0';
        }
        else
        {
            pEndToken++;
        }
    }
    mpInput->SetBufferPointer( pEndToken );

    return pToken;
}

void
ForthShell::SetCommandLine( int argc, const char ** argv )
{
    int i, len;

    DeleteCommandLine();

    i = 0;
    mpArgs = new char *[ argc ];
    while ( i < argc )
    {
        len = strlen( argv[i] ) + 1;
        mpArgs[i] = new char [ len ];
        strcpy( mpArgs[i], argv[i] );
        i++;
    }
    mNumArgs = argc;

    // see if there are files appended to the executable file
    // the format is:
    //
    // PER FILE:
    // ... file1 databytes...
    // length of file1 as int
    // 0xDEADBEEF
    // ... file1name ...
    // length of file1name as int
    //
    // ... fileZ databytes...
    // length of fileZ as int
    // 0xDEADBEEF
    // ... fileZname ...
    // length of fileZname as int
    //
    // number of appended files as int
    // 0x34323137
    if ( (argc > 0) && (argv[0] != NULL) )
    {
        FILE* pFile = NULL;
		pFile = fopen( argv[0], "rb" );
        if ( pFile != NULL )
        {
            int res = fseek( pFile, -4, SEEK_END );
            int token = 0;
            if ( res == 0 )
            {
                int nItems = fread( &token, sizeof(token), 1, pFile );
                if ( nItems == 1 )
#define FORTH_MAGIC_1 0x37313234
#define FORTH_MAGIC_2 0xDEADBEEF
                {
                    if ( token == FORTH_MAGIC_1 )
                    {
                        int count = 0;
                        res = fseek( pFile, -8, SEEK_CUR );
                        nItems = fread( &count, sizeof(count), 1, pFile );
                        if ( (res == 0) && (nItems == 1) )
                        {
                            mpInternalFiles = new sInternalFile[ count ];
                            mInternalFileCount = count;
                            for ( i = 0; i < count; i++ )
                            {
                                mpInternalFiles[i].pName = NULL;
                                mpInternalFiles[i].length = 0;
                            }
                            res = fseek( pFile, -4, SEEK_CUR );
                            for ( i = 0; i < mInternalFileCount; i++ )
                            {
                                // first get the filename length
                                res = fseek( pFile, -4, SEEK_CUR );
                                nItems = fread( &count, sizeof(count), 1, pFile );
                                if ( (res != 0) || (nItems != 1) )
                                {
                                    break;
                                }
                                res = fseek( pFile, -(count + 8), SEEK_CUR );
                                // filepos is at start of token2, followed by filename
                                int offset = ftell( pFile );
                                // check for magic token2
                                nItems = fread( &token, sizeof(token), 1, pFile );
                                if ( (res != 0) || (nItems != 1) || (token != FORTH_MAGIC_2) )
                                {
                                    break;
                                }
                                // filepos is at filename
                                mpInternalFiles[i].pName = new char[ count + 1 ];
                                mpInternalFiles[i].pName[count] = '\0';
                                nItems = fread( mpInternalFiles[i].pName, count, 1, pFile );
                                if ( (res != 0) || (nItems != 1) )
                                {
                                    delete [] mpInternalFiles[i].pName;
                                    mpInternalFiles[i].pName = NULL;
                                    break;
                                }
                                // seek back to data length just before filename
                                res = fseek( pFile, (offset - 4), SEEK_SET );
                                nItems = fread( &count, sizeof(count), 1, pFile );
                                if ( (res != 0) || (nItems != 1) )
                                {
                                    break;
                                }
                                res = fseek( pFile, -(count + 4), SEEK_CUR );
                                if ( res != 0 )
                                {
                                    break;
                                }
                                // filepos is at start of data section
                                mpInternalFiles[i].offset = ftell( pFile );
                                mpInternalFiles[i].length = count;
                                TRACE( "Found file %s, %d bytes at 0x%x\n", mpInternalFiles[i].pName,
                                        count, mpInternalFiles[i].offset );
                            }
                        }
                    }
                }
            }
        }
    }
}


void
ForthShell::SetCommandLine( const char *pCmdLine )
{
    // TBD
}


void
ForthShell::SetEnvironmentVars( const char ** envp )
{
    int i, len;
    char *pValue;

    DeleteEnvironmentVars();

    // count number of environment variables
    mNumEnvVars = 0;
    while ( envp[mNumEnvVars] != NULL )
    {
        mNumEnvVars++;
    }
    mpEnvVarNames = new char *[ mNumEnvVars ];
    mpEnvVarValues = new char *[ mNumEnvVars ];

    // make copies of vars
    i = 0;
    while ( i < mNumEnvVars )
    {
        len = strlen( envp[i] ) + 1;
        mpEnvVarNames[i] = new char[ len ];
        strcpy( mpEnvVarNames[i], envp[i] );
        pValue = strchr( mpEnvVarNames[i], '=' );
        if ( pValue != NULL )
        {
            *pValue++ = '\0';
            mpEnvVarValues[i] = pValue;
        }
        else
        {
            printf( "Malformed environment variable: %s\n", envp[i] );
        }
        i++;
    }
}

void
ForthShell::DeleteCommandLine( void )
{
    while ( mNumArgs > 0 )
    {
        mNumArgs--;
        delete [] mpArgs[mNumArgs];
    }
    delete [] mpArgs;

    mpArgs = NULL;

    if ( mpInternalFiles != NULL )
    {
        for ( int i = 0; i < mInternalFileCount; i++ )
        {
            if ( mpInternalFiles[i].pName != NULL )
            {
                delete [] mpInternalFiles[i].pName;
            }
        }
        delete [] mpInternalFiles;
        mpInternalFiles = NULL;
    }
    mInternalFileCount = 0;
}


void
ForthShell::DeleteEnvironmentVars( void )
{
    while ( mNumEnvVars > 0 )
    {
        mNumEnvVars--;
        delete [] mpEnvVarNames[mNumEnvVars];
    }
    delete [] mpEnvVarNames;
    delete [] mpEnvVarValues;

    mpEnvVarNames = NULL;
    mpEnvVarValues = NULL;
}


bool
ForthShell::CheckSyntaxError( const char *pString, long tag, long desiredTag )
{
    if ( tag != desiredTag )
    {
        sprintf( mErrorString, "<%s> preceeded by <%s>, was expecting <%s>",
                 pString, GetTagString( tag ), GetTagString( desiredTag ) );
        mpEngine->SetError( kForthErrorBadSyntax, mErrorString );
        return false;
    }
    return true;
}


ForthFileInterface*
ForthShell::GetFileInterface()
{
    return &mFileInterface;
}

char
ForthShell::GetChar()
{
    return getchar();
}

FILE*
ForthShell::FileOpen( const char* filePath, const char* openMode )
{
	FILE* pFile = NULL;
    pFile = fopen( filePath, openMode );
	return pFile;
}

int
ForthShell::FileClose( FILE* pFile )
{
    return fclose( pFile );
}

int
ForthShell::FileSeek( FILE* pFile, int offset, int control )
{
    return fseek( pFile, offset, control );
}

int
ForthShell::FileRead( FILE* pFile, void* pDst, int itemSize, int numItems )
{
    return fread( pDst, itemSize, numItems, pFile );
}

int
ForthShell::FileWrite( FILE* pFile, const void* pSrc, int itemSize, int numItems ) 
{
    return fwrite( pSrc, itemSize, numItems, pFile );
}

int
ForthShell::FileGetChar( FILE* pFile )
{
    return fgetc( pFile );
}

int
ForthShell::FilePutChar( FILE* pFile, int outChar )
{
    return fputc( outChar, pFile );
}

int
ForthShell::FileAtEOF( FILE* pFile )
{
    return feof( pFile );
}

int
ForthShell::FileCheckExists( const char* pFilename )
{
    FILE* pFile = fopen( pFilename, "r" );
    int result = (pFile != NULL) ? ~0 : 0;
    if ( pFile != NULL )
    {
        fclose( pFile );
    }
    return result;
}

int
ForthShell::FileGetLength( FILE* pFile )
{
    int oldPos = ftell( pFile );
    fseek( pFile, 0, SEEK_END );
    int result = ftell( pFile );
    fseek( pFile, oldPos, SEEK_SET );
    return result;
}

int
ForthShell::FileGetPosition( FILE* pFile )
{
    return ftell( pFile );
}

char*
ForthShell::FileGetString( FILE* pFile, char* pBuffer, int maxChars )
{
    return fgets( pBuffer, maxChars, pFile );
}


int
ForthShell::FilePutString( FILE* pFile, const char* pBuffer )
{
    return fputs( pBuffer, pFile );
}

/*int
ForthShell::FileRemove( Fonst char* pFilename )
{
    return remove( pFilename );
}*/

//
// support for conditional compilation ops
//

void ForthShell::PoundIf()
{
    //mPoundIfDepth++;

    // set flags so Run will know to check result at end of line
    //   and either continue compiling or skip section
    if ( mpEngine->IsCompiling() )
    {
        // this means set state back to compile at end of line
        mFlags |= SHELL_FLAG_START_IF_C;
    }
    else
    {
        mFlags |= SHELL_FLAG_START_IF_I;
    }

    // evaluate rest of input line
    mpEngine->SetCompileState( 0 );
}


void ForthShell::PoundIfdef( bool isDefined )
{
    ForthVocabulary* pVocab = mpEngine->GetSearchVocabulary();
    char* pToken = GetNextSimpleToken();
    if ( (pToken != NULL) && (pVocab != NULL)
        && ((pVocab->FindSymbol( pToken ) != NULL) == isDefined) )
    {
        // compile "if" part
        mpStack->Push( kShellTagPoundIf );
    }
    else
    {
        // skip to "else" or "endif"
        mFlags |= SHELL_FLAG_SKIP_SECTION;
        mPoundIfDepth = 0;
    }
}


void ForthShell::PoundElse()
{
    long marker = mpStack->Pop();
    if ( marker == kShellTagPoundIf )
    {
        mFlags |= SHELL_FLAG_SKIP_SECTION;
        // put the marker back for PoundEndif
        mpStack->Push( marker );
    }
    else
    {
        // error - unexpected else
        mpEngine->SetError( kForthErrorBadPreprocessorDirective, "unexpected #else" );
    }
}


void ForthShell::PoundEndif()
{
    long marker = mpStack->Pop();
    if ( marker != kShellTagPoundIf )
    {
        // error - unexpected endif
        mpEngine->SetError( kForthErrorBadPreprocessorDirective, "unexpected #endif" );
    }
}


FILE* ForthShell::OpenInternalFile( const char* pFilename )
{
    // see if file is an internal file, and if so use it
	FILE* pFile = NULL;
    for ( int i = 0; i < mInternalFileCount; i++ )
    {
        if ( strcmp( mpInternalFiles[i].pName, pFilename ) == 0 )
        {
            // there is an internal file, open this .exe and seek to internal file
            pFile = fopen( mpArgs[0], "r" );
            if ( fseek( pFile, mpInternalFiles[i].offset, 0 ) != 0 )
            {
                fclose( pFile );
                pFile = NULL;
            }
            break;
        }
    }
	return pFile;
}


FILE* ForthShell::OpenForthFile( const char* pPath )
{
    // see if file is an internal file, and if so use it
    FILE *pFile = OpenInternalFile( pPath );
    if ( pFile == NULL )
    {
		pFile = fopen( pPath, "r" );
    }
	bool pathIsRelative = true;
#if defined( WIN32 )
	if ( strchr( pPath, ':' ) != NULL )
	{
		pathIsRelative = false;
	}
#elif defined( LINUX )
	if ( *pPath == '/' )
	{
		pathIsRelative = false;
	}
#endif
    if ( (pFile == NULL) && pathIsRelative )
    {
		char* pSysPath = new char[ strlen(mWorkingDirPath) + strlen(pPath) + 16 ];
		strcpy( pSysPath, mWorkingDirPath );
#if defined( WIN32 )
		strcat( pSysPath, "\\system\\" );
#elif defined( LINUX )
		strcat( pSysPath, "/system/" );
#endif
		strcat( pSysPath, pPath );
		pFile = fopen( pSysPath, "r" );
		delete pSysPath;
    }
	return pFile;
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthParseInfo
// 

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


// copy string to mpToken buffer, set length, and pad with nulls to a longword boundary
// if pSrc is null, just set length and do padding
void
ForthParseInfo::SetToken( const char *pSrc )
{
    int symLen, padChars;
    char *pDst;

    if ( pSrc != NULL )
    {

        symLen = strlen( pSrc );
        pDst = (char *) mpToken;

        if ( symLen > mMaxChars )
        {
            symLen = mMaxChars;
        }
        // set length byte
        *pDst++ = symLen;

        // make copy of symbol
        memcpy( pDst, pSrc, symLen );
        pDst += symLen;
        *pDst++ = '\0';

    }
    else
    {

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
    if ( padChars > 1 )
    {
        padChars = 4 - padChars;
        while ( padChars > 0 )
        {
            *pDst++ = '\0';
            padChars--;
        }
    }
}


//////////////////////////////////////////////////////////////////////
////
///
//                     ForthShellStack
// 

// this is the number of extra longs to allocate at top and
//    bottom of stacks
#define GAURD_AREA 4

ForthShellStack::ForthShellStack( int numLongs )
: mSSLen( numLongs )
{
   mSSB = new long[mSSLen + (GAURD_AREA * 2)];
   mSSB += GAURD_AREA;
   mSST = mSSB + mSSLen;
   EmptyStack();
}

ForthShellStack::~ForthShellStack()
{
   delete [] (mSSB - GAURD_AREA);
}


void
ForthShellStack::Push( long tag )
{
    *--mSSP = tag;
    SPEW_SHELL( "Pushed Tag %s\n", GetTagString( tag ) );
}

long
ForthShellStack::Pop( void )
{
    if ( mSSP == mSST )
    {
        return kShellTagNothing;
    }
    SPEW_SHELL( "Popped Tag %s\n", GetTagString( *mSSP ) );
    return *mSSP++;
}

long
ForthShellStack::Peek( void )
{
    if ( mSSP == mSST )
    {
        return kShellTagNothing;
    }
    return *mSSP;
}

void
ForthShellStack::PushString( const char *pString )
{
    int len = strlen( pString );
    mSSP -= (len >> 2) + 1;
    strcpy( (char *) mSSP, pString );
    SPEW_SHELL( "Pushed String \"%s\"\n", pString );
    Push( kShellTagString );
}

bool
ForthShellStack::PopString( char *pString )
{
    if ( *mSSP != kShellTagString )
    {
        *pString = '\0';
        SPEW_SHELL( "Failed to pop string\n" );
        return false;
    }
    mSSP++;
    int len = strlen( (char *) mSSP );
    strcpy( pString, (char *) mSSP );
    mSSP += (len >> 2) + 1;
    SPEW_SHELL( "Popped Tag string\n" );
    SPEW_SHELL( "Popped String \"%s\"\n", pString );
    return true;
}

//////////////////////////////////////////////////////////////////////
////
///
//                     Windows Thread Procedures
// 

#if defined(WIN32)
DWORD WINAPI ConsoleInputThreadRoutine( void* pThreadData )
#elif defined(LINUX)
unsigned long ConsoleInputThreadRoutine( void* pThreadData )
#endif
{
    ForthShell* pShell = (ForthShell *) pThreadData;

#if 0
    return pShell->ConsoleInputLoop();
#else
    return NULL;
#endif
}

