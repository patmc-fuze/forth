//////////////////////////////////////////////////////////////////////
//
// ForthServer.cpp: Forth server support
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ForthServer.h"

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

void writeSocketString( SOCKET s, int command, const char* str )
{
    send( s, (const char*) &command, sizeof(command), 0 );
    int len = (str == NULL ) ? 0 : strlen( str );
    send( s, (const char*) &len, sizeof(len), 0 );
    if ( len != 0 )
    {
        send( s, str, len, 0 );
    }
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
    ForthServerShell* pShell = (ForthServerShell *) (((ForthEngine *)(pCore->pEngine))->GetShell());
    pShell->SendTextToClient( pMessage );
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
    SendCommandString( kClientCmdSendLine, mIsFile ? NULL : pPrompt );
    mpBuffer = mpBufferBase;
    if ( GetResponse( &cmd ) ) 
    {
        return (cmd == kServerCmdPopStream) ? NULL : mpBuffer;
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
    writeSocketString( mSocket, command, str );
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
    writeSocketString( mSocket, kClientCmdStartLoad, pFileName );
    mpInput->PushInputStream( new ForthServerInputStream( mSocket, true ) );
    return true;
}

void ForthServerShell::SendTextToClient( const char *pMessage )
{
    writeSocketString( mSocket, kClientCmdDisplayText, pMessage );
}

char ForthServerShell::GetChar()
{
    char buffer[16];
    int cmd;

    writeSocketString( mSocket, kClientCmdGetChar, NULL );
    readSocketResponse( mSocket, &cmd, buffer, sizeof(buffer) );
    return buffer[0];
}



