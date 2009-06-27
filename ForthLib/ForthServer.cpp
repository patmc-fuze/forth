//////////////////////////////////////////////////////////////////////
//
// ForthServer.cpp: Forth server support
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ForthServer.h"

#ifdef DEBUG_WITH_NDS_EMULATOR
#include <nds.h>
#endif

int readSocketData( SOCKET s, char *buf, int len )
{
    int nBytes = 0;
    while ( nBytes != len )
    {
        nBytes = recv( s, buf, len, MSG_PEEK );
    }
    nBytes = recv( s, buf, len, 0 );
    return nBytes;
}

void sendCommandToClient( SOCKET s, int command, const char* data, int dataLen )
{
    send( s, (const char*) &command, sizeof(command), 0 );
    send( s, (const char*) &dataLen, sizeof(dataLen), 0 );
    if ( dataLen != 0 )
    {
        send( s, data, dataLen, 0 );
    }
}


void sendStringCommandToClient( SOCKET s, int command, const char* str )
{
    int len = (str == NULL ) ? 0 : strlen( str );
    sendCommandToClient( s, command, str, len );
}

bool readSocketResponse( SOCKET s, int* command, char* buffer, int bufferLen )
{
    int nBytes = readSocketData( s, (char *) command, sizeof(*command) );
    if ( nBytes != sizeof( *command ) )
    {
        return false;
    }
    int len;
    nBytes = readSocketData( s, (char *) &len, sizeof(len) );
    if ( nBytes != sizeof( len ) )
    {
        return false;
    }
    // TBD: check len for buffer overflow
    if ( len != 0 )
    {
        nBytes = readSocketData( s, buffer, len );
        if ( nBytes != len )
        {
            return false;
        }
    }
    buffer[len] = '\0';

    return true;
}

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

ForthServerInputStream::ForthServerInputStream( SOCKET inSocket, bool isFile, int bufferLen )
:   ForthInputStream( bufferLen )
,   mSocket( inSocket )
,   mIsFile( isFile )
{
}

ForthServerInputStream::~ForthServerInputStream()
{
}

char* ForthServerInputStream::GetLine( const char *pPrompt )
{
    int cmd;
    SendCommandString( kClientMsgSendLine, mIsFile ? NULL : pPrompt );
    mpBuffer = mpBufferBase;
    if ( GetResponse( &cmd ) ) 
    {
        return (cmd == kServerMsgPopStream) ? NULL : mpBuffer;
    }
    else
    {
        // error
    }
    return NULL;
}

bool ForthServerInputStream::GetResponse( int* command )
{
    return readSocketResponse( mSocket, command, mpBufferBase, mBufferLen );
}

void ForthServerInputStream::SendCommandString( int command, const char* str )
{
    sendStringCommandToClient( mSocket, command, str );
}

bool ForthServerInputStream::IsInteractive( void )
{
    return !mIsFile;
}

SOCKET ForthServerInputStream::GetSocket()
{
    return mSocket;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ForthServerShell::ForthServerShell( bool doAutoload, ForthEngine *pEngine, ForthThread *pThread, int shellStackLongs )
:   ForthShell( pEngine, pThread, shellStackLongs )
,   mDoAutoload( doAutoload )
{
}

ForthServerShell::~ForthServerShell()
{
}

int ForthServerShell::Run( ForthInputStream *pInputStream )
{
    ForthServerInputStream* pStream = (ForthServerInputStream *) pInputStream;
    mSocket = pStream->GetSocket();

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
    sendStringCommandToClient( mSocket, kClientMsgStartLoad, pFileName );
    mpInput->PushInputStream( new ForthServerInputStream( mSocket, true ) );
    return true;
}

void ForthServerShell::SendTextToClient( const char *pMessage )
{
    sendStringCommandToClient( mSocket, kClientMsgDisplayText, pMessage );
}

char ForthServerShell::GetChar()
{
    char buffer[16];
    int cmd;

    sendStringCommandToClient( mSocket, kClientMsgGetChar, NULL );
    readSocketResponse( mSocket, &cmd, buffer, sizeof(buffer) );
    return buffer[0];
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

