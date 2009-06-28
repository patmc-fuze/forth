//////////////////////////////////////////////////////////////////////
//
// ForthServer.cpp: Forth server support
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ForthServer.h"
#include "ForthPipe.h"
#include "ForthMessages.h"

#ifdef DEBUG_WITH_NDS_EMULATOR
#include <nds.h>
#endif

static void consoleOutToClient( ForthCoreState   *pCore,
                                const char       *pMessage )
{
#ifdef DEBUG_WITH_NDS_EMULATOR
	iprintf( "%s", pMessage );
#else
    ForthServerShell* pShell = (ForthServerShell *) (((ForthEngine *)(pCore->pEngine))->GetShell());
    pShell->SendTextToClient( pMessage );
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ForthServerInputStream::ForthServerInputStream( ForthPipe* pPipe, bool isFile, int bufferLen )
:   ForthInputStream( bufferLen )
,   mpMsgPipe( pPipe )
,   mIsFile( isFile )
{
}

ForthServerInputStream::~ForthServerInputStream()
{
}

char* ForthServerInputStream::GetLine( const char *pPrompt )
{
    int msgType, msgLen;
    char* result = NULL;

    mpBuffer = mpBufferBase;

    mpMsgPipe->StartMessage( kClientMsgSendLine );
    mpMsgPipe->WriteString( mIsFile ? NULL : pPrompt );
    mpMsgPipe->SendMessage();

    mpMsgPipe->GetMessage( msgType, msgLen );
    switch ( msgType )
    {
    case kServerMsgProcessLine:
        {
            const char* pSrcLine;
            int srcLen = 0;
            if ( mpMsgPipe->ReadCountedData( pSrcLine, srcLen ) )
            {
                if ( srcLen != 0 )
                {
                    memcpy( mpBuffer, pSrcLine, srcLen );
                    result = mpBuffer;
                }
            }
            else
            {
                // TBD: report error
            }
        }
        break;

    case kServerMsgPopStream:
        break;

    default:
        // TBD: report unexpected message type error
    printf( "ForthServerShell::GetLine unexpected message type %d\n", msgType );
        break;
    }

    return result;
}

bool ForthServerInputStream::IsInteractive( void )
{
    return !mIsFile;
}

ForthPipe* ForthServerInputStream::GetPipe()
{
    return mpMsgPipe;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ForthServerShell::ForthServerShell( bool doAutoload, ForthEngine *pEngine, ForthThread *pThread, int shellStackLongs )
:   ForthShell( pEngine, pThread, shellStackLongs )
,   mDoAutoload( doAutoload )
,   mpMsgPipe( NULL )
{
}

ForthServerShell::~ForthServerShell()
{
}

int ForthServerShell::Run( ForthInputStream *pInputStream )
{
    ForthServerInputStream* pStream = (ForthServerInputStream *) pInputStream;
    mpMsgPipe = pStream->GetPipe();

    const char *pBuffer;
    int retVal = 0;
    bool bQuit = false;
    eForthResult result = kResultOk;
    bool bInteractiveMode = pStream->IsInteractive();

    mpEngine->SetConsoleOut( consoleOutToClient, NULL );
	mpEngine->ResetConsoleOut( mpEngine->GetCoreState()->pThread );
    mpInput->PushInputStream( pStream );

    if ( mDoAutoload )
    {
        mpEngine->PushInputFile( "forth_autoload.txt" );
    }

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
                mpMsgPipe->StartMessage( kClientMsgGoAway );
                mpMsgPipe->SendMessage();
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

            default:
                break;
            }

        }
    }

    return retVal;
}

bool ForthServerShell::PushInputFile( const char *pFileName )
{
    mpMsgPipe->StartMessage( kClientMsgStartLoad );
    mpMsgPipe->WriteString( pFileName );
    mpMsgPipe->SendMessage();
    mpInput->PushInputStream( new ForthServerInputStream( mpMsgPipe, true ) );
    return true;
}

void ForthServerShell::SendTextToClient( const char *pMessage )
{
    mpMsgPipe->StartMessage( kClientMsgDisplayText );
    mpMsgPipe->WriteString( pMessage );
    mpMsgPipe->SendMessage();
}

char ForthServerShell::GetChar()
{
    int c;
    int msgType, msgLen;

    mpMsgPipe->StartMessage( kClientMsgGetChar );
    mpMsgPipe->SendMessage();
    mpMsgPipe->GetMessage( msgType, msgLen );
    if ( msgType == kServerMsgProcessChar )
    {
        mpMsgPipe->ReadInt( c );
    }
    else
    {
        // TBD: report error
        printf( "ForthServerShell::GetChar unexpected message type %d\n", msgType );
    }
    return (int) c;
}

FILE*
ForthServerShell::FileOpen( const char* filePath, const char* openMode )
{
#if 0
    FILE* resultFile = NULL;
    int command = kClientMsgFileOpen;

    send( mSocket, (const char*) &command, sizeof(command), 0 );
    int pathLen = strlen( filePath );
    int modeLen = strlen( openMode );
    int dataLen = pathLen + modeLen + 8;
    send( mSocket, (const char*) &dataLen, sizeof(dataLen), 0 );
    send( mSocket, (const char*) &pathLen, sizeof(pathLen), 0 );
    if ( pathLen != 0 )
    {
        send( mSocket, filePath, pathLen, 0 );
    }
    send( mSocket, (const char*) &modeLen, sizeof(modeLen), 0 );
    if ( modeLen != 0 )
    {
        send( mSocket, openMode, modeLen, 0 );
    }
    readSocketResponse( mSocket, &command, resultFile, sizeof(resultFile) );
    if ( command != expectedResult )
        .........
    return (FILE*) &(buffer[0]);
#endif
    return fopen( filePath, openMode );
}

int
ForthServerShell::FileClose( FILE* pFile )
{
    return fclose( pFile );
}

int
ForthServerShell::FileSeek( FILE* pFile, int offset, int control )
{
    return fseek( pFile, offset, control );
}

int
ForthServerShell::FileRead( FILE* pFile, void* pDst, int numItems, int itemSize )
{
    return fread( pDst, numItems, itemSize, pFile );
}

int
ForthServerShell::FileWrite( FILE* pFile, void* pDst, int numItems, int itemSize ) 
{
    return fwrite( pDst, numItems, itemSize, pFile );
}

int
ForthServerShell::FileGetChar( FILE* pFile )
{
    return fgetc( pFile );
}

int
ForthServerShell::FilePutChar( FILE* pFile, int outChar )
{
    return fputc( outChar, pFile );
}

int
ForthServerShell::FileAtEOF( FILE* pFile )
{
    return feof( pFile );
}

int
ForthServerShell::FileGetLength( FILE* pFile )
{
    int oldPos = ftell( pFile );
    fseek( pFile, 0, SEEK_END );
    int result = ftell( pFile );
    fseek( pFile, oldPos, SEEK_SET );
    return result;
}

int
ForthServerShell::FileGetPosition( FILE* pFile )
{
    return ftell( pFile );
}

char*
ForthServerShell::FileGetString( FILE* pFile, char* pBuffer, int maxChars )
{
    return fgets( pBuffer, maxChars, pFile );
}


int
ForthServerShell::FilePutString( FILE* pFile, const char* pBuffer )
{
    return fputs( pBuffer, pFile );
}

