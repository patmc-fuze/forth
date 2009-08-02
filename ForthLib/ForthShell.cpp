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

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthShell
// 

ForthShell::ForthShell( ForthEngine *pEngine, ForthThread *pThread, int shellStackLongs )
: mpEngine(pEngine)
, mpThread(pThread)
, mFlags(0)
, mNumArgs(0)
, mpArgs(NULL)
, mNumEnvVars(0)
, mpEnvVarNames(NULL)
, mpEnvVarValues(NULL)
{
    if ( mpEngine == NULL ) {
        mpEngine = new ForthEngine();
        mpEngine->Initialize( STORAGE_LONGS, true );
        mFlags = SHELL_FLAG_CREATED_ENGINE;
    }
    mpEngine->SetShell( this );

    if ( mpThread == NULL ) {
        mpThread = mpEngine->CreateThread( PSTACK_LONGS, RSTACK_LONGS, NULL );
    }
    mpEngine->SetCurrentThread( mpThread );

    mpInput = new ForthInputStack;
	mpStack = new ForthShellStack( shellStackLongs );

}

ForthShell::~ForthShell()
{
    // engine will destroy thread for us if we created it
    if ( mFlags & SHELL_FLAG_CREATED_ENGINE ) {
        delete mpEngine;
    }

    DeleteEnvironmentVars();
    DeleteCommandLine();
    delete mpInput;
	delete mpStack;
}



//
// create a new file input stream & push on stack
//
bool
ForthShell::PushInputFile( const char *pFileName )
{
    FILE *pInFile = fopen( pFileName, "r" );
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

    mpEngine->PushInputFile( "forth_autoload.txt" );

    while ( !bQuit ) {

        // try to fetch a line from current stream
        pBuffer = mpInput->GetLine( mpEngine->GetFastMode() ? "turbo>" : "ok>" );
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

#ifdef _WINDOWS
            try
#endif
            {
                result = mpEngine->ProcessToken( &parseInfo );
                CHECK_STACKS( mpEngine->GetCurrentThread() );
            }
#ifdef _WINDOWS
            catch(...)
            {
                result = kResultException;
                mpEngine->SetError( kForthErrorException );
            }
#endif
            if ( result == kResultOk )
			{
                result = mpEngine->CheckStacks();
            }
            if ( result != kResultOk )
			{
                if ( result != kResultExitShell )
				{
                    ReportError();
                }
				ErrorReset();
                if ( !mpInput->InputStream()->IsInteractive() && ( result != kResultExitShell) )
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
}

void
ForthShell::ReportError( void )
{
    char errorBuf1[512];
    char errorBuf2[512];
    char *pLastInputToken;

    mpEngine->GetErrorString( errorBuf1 );
    pLastInputToken = mpEngine->GetLastInputToken();
	ForthCoreState* pCore = mpEngine->GetCoreState();
	ForthThreadState* pThread = pCore->pThread;

	if ( pLastInputToken != NULL ) {
        sprintf( errorBuf2, "%s, last input token: <%s> last IP 0x%x", errorBuf1, pLastInputToken, pCore->IP );
    } else {
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
	pThread->consoleOut( pCore, errorBuf1 );
    char *pBase = mpInput->GetBufferBasePointer();
    pLastInputToken = mpInput->GetBufferPointer();
    if ( (pBase != NULL) && (pLastInputToken != NULL) ) {
		char* pBuf = errorBuf1;
		*pBuf++ = '\n';
        while ( pBase < pLastInputToken )
        {
            *pBuf++ = *pBase++;
        }
        sprintf( pBuf, "{}%s\n", pLastInputToken );
    }
	TRACE( "%s", errorBuf1 );
	pThread->consoleOut( pCore, errorBuf1 );
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


static const char *
ForthParseSingleQuote( const char       *pSrc,
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


static const char *
ForthParseDoubleQuote( const char       *pSrc,
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

        if ( pInfo->GetFlags() == 0 ) {
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
            }
            else
            {
                sprintf( mErrorString, "top of shell stack is <%s>, was expecting <string>",
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

        if ( pInfo->GetFlags() == 0 ) {
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


char *
ForthShell::GetToken( char delim )
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
    while ( i < argc ) {
        len = strlen( argv[i] );
        mpArgs[i] = new char [ len + 1 ];
        strcpy( mpArgs[i], argv[i] );
        i++;
    }
    mNumArgs = argc;
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
        len = strlen( envp[i] );
        mpEnvVarNames[i] = new char[ len + 1 ];
        strcpy( mpEnvVarNames[i], envp[i] );
        pValue = strchr( mpEnvVarNames[i], '=' );
        if ( pValue != NULL )
        {
            *pValue++ = '\0';
            mpEnvVarValues[i] = pValue;
        } else {
            printf( "Malformed environment variable: %s\n", envp[i] );
        }
        i++;
    }
}

void
ForthShell::DeleteCommandLine( void )
{
    while ( mNumArgs > 0 ) {
        mNumArgs--;
        delete [] mpArgs[mNumArgs];
    }
    delete [] mpArgs;

    mpArgs = NULL;
}


void
ForthShell::DeleteEnvironmentVars( void )
{
    while ( mNumEnvVars > 0 ) {
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


char
ForthShell::GetChar()
{
    return getchar();
}

FILE*
ForthShell::FileOpen( const char* filePath, const char* openMode )
{
    return fopen( filePath, openMode );
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
