// ForthClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include "ForthPipe.h"
#include "ForthMessages.h"

#pragma comment(lib, "wininet.lib")

void ErrorExit( const char* message )
{
    //TRACE( "%s\n", message );
    WSACleanup();
}

int _tmain(int argc, _TCHAR* argv[])
{
    //----------------------
    // Initialize Winsock
    WSADATA wsaData;
    char errorMessage[128];
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
    }

    //----------------------
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket;
    //ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ConnectSocket == INVALID_SOCKET)
    {
        sprintf( errorMessage, "Error at socket(): %ld", WSAGetLastError() );
        ErrorExit( errorMessage );
        return -1;
    }

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    sockaddr_in clientService; 
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr( (argc > 1) ? argv[1] : "127.0.0.1" );
    clientService.sin_port = htons( FORTH_SERVER_PORT );

    //----------------------
    // Connect to server.
    if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
        sprintf( errorMessage, "Failed to connect." );
        ErrorExit( errorMessage );
        return -1;
    }

    ForthPipe* pMsgPipe = new ForthPipe( ConnectSocket, kClientMsgDisplayText, kClientMsgLimit );
    bool done = false;
    char buffer[ 1024 ];
    int readBufferSize = 16384;
    char* pReadBuffer = new char[ readBufferSize ];
    FILE*   inputStack[ 16 ];
    int inputStackDepth = 0;
    inputStack[ 0 ] = stdin;
    while ( !done )
    {
        int msgLen, msgType;
        if ( pMsgPipe->GetMessage( msgType, msgLen ) )
        {
            switch( msgType )
            {
            case kClientMsgSendLine:
                {
                    // SendLine params:
                    //   string     prompt
                    const char* pPrompt;
                    int promptLen;
                    if ( pMsgPipe->ReadCountedData( pPrompt, promptLen ) )
                    {
                        if ( promptLen != 0 )
                        {
                            printf( "\n%s ", pPrompt );
                        }
                        // read a line of text from top stream on inputStack and send to server
                        char *pBuffer;
                        buffer[0] = '\0';
                        pBuffer = fgets( buffer, sizeof(buffer), inputStack[ inputStackDepth ] );
                        if ( pBuffer != NULL )
                        {
                            char* pNewline = strchr( buffer, '\r' );
                            if ( pNewline != NULL )
                            {
                                *pNewline = '\0';
                            }
                            pMsgPipe->StartMessage( kServerMsgProcessLine );
                            pMsgPipe->WriteString( buffer );
                            pMsgPipe->SendMessage();
                        }
                        else
                        {
                            // this input stream is empty, pop the input stack and let server know
                            pMsgPipe->StartMessage( kServerMsgPopStream );
                            pMsgPipe->SendMessage();
                            fclose( inputStack[ inputStackDepth ] );
                            inputStackDepth--;
                        }
                    }
                    else
                    {
                        printf( "Failed reading prompt string while processing SendLine\n" );
                        done = true;
                    }
                }
                break;

            case kClientMsgStartLoad:
                {
                    const char* pFilename;
                    pMsgPipe->ReadString( pFilename );
                    FILE* newInputFile = fopen( pFilename, "r" );
                    int itWorked = 0;
                    if ( newInputFile != NULL )
                    {
                        inputStackDepth++;
                        inputStack[ inputStackDepth ] = newInputFile;
                        itWorked = 1;
                    }
                    else
                    {
                        printf( "Client: failed to open '%s' upon server request!\n", pFilename );
                    }
                    pMsgPipe->StartMessage( kServerMsgStartLoadResult );
                    pMsgPipe->WriteInt( itWorked );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgPopStream:
                {
                    // a "loaddone" was executed on server, so pop input stream
                    if ( inputStackDepth > 0 )
                    {
                        fclose( inputStack[ inputStackDepth ] );
                        inputStackDepth--;
                    }
                }
                break;

            case kClientMsgDisplayText:
                {
                    const char* pText;
                    int textLen;
                    if ( pMsgPipe->ReadCountedData( pText, textLen ) )
                    {
                        if ( textLen != 0 )
                        {
                            printf( "%s", pText );
                        }
                    }
                    else
                    {
                        printf( "Failed reading text string while processing DisplayText\n" );
                        done = true;
                    }
                }
                break;

            case kClientMsgGetChar:
                {
                    int c = getchar();
                    pMsgPipe->StartMessage( kServerMsgProcessChar );
                    pMsgPipe->WriteInt( c );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGoAway:
                done = true;
                break;

            case kClientMsgFileOpen:
                {
                    const char* pFilename;
                    const char* accessMode;
                    pMsgPipe->ReadString( pFilename );
                    pMsgPipe->ReadString( accessMode );
                    FILE* pFile = fopen( pFilename, accessMode );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( (int) pFile );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileClose:
                {
                    int file;
                    pMsgPipe->ReadInt( file );
                    int result = fclose( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileSetPosition:
                {
                    int file, offset, control;
                    pMsgPipe->ReadInt( file );
                    pMsgPipe->ReadInt( offset );
                    pMsgPipe->ReadInt( control );
                    int result = fseek( (FILE *) file, offset, control );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileRead:
                {
                    int file, numItems, itemSize;
                    pMsgPipe->ReadInt( file );
                    pMsgPipe->ReadInt( itemSize );
                    pMsgPipe->ReadInt( numItems );
                    int numBytes = numItems * itemSize;
                    if ( numBytes > readBufferSize )
                    {
                        delete [] pReadBuffer;
                        pReadBuffer = new char[ numBytes ];
                        readBufferSize = numBytes;
                    }
                    int result = fread( pReadBuffer, itemSize, numItems, (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileReadResult );
                    pMsgPipe->WriteInt( result );
                    if ( result > 0 )
                    {
                        numBytes = result * itemSize;
                        pMsgPipe->WriteCountedData( pReadBuffer, numBytes );
                    }
                    else
                    {
                        printf( "fread returned %d\n", result );
                    }
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileWrite:
                {
                    int file, numItems, itemSize, numBytes;
                    const char* pData;
                    pMsgPipe->ReadInt( file );
                    pMsgPipe->ReadInt( itemSize );
                    pMsgPipe->ReadInt( numItems );
                    pMsgPipe->ReadCountedData( pData, numBytes );
                    int result = fwrite( pData, itemSize, numItems, (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileGetChar:
                {
                    int file;
                    pMsgPipe->ReadInt( file );
                    int result = fgetc( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileCheckEOF:
                {
                    int file;
                    pMsgPipe->ReadInt( file );
                    int result = feof( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileGetLength:
                {
                    int file;
                    pMsgPipe->ReadInt( file );
                    FILE* pFile = (FILE *) file;
                    int oldPos = ftell( pFile );
                    fseek( pFile, 0, SEEK_END );
                    int result = ftell( pFile );
                    fseek( pFile, oldPos, SEEK_SET );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileCheckExists:
                {
                    const char* pFilename;
                    pMsgPipe->ReadString( pFilename );
                    FILE* pFile = fopen( pFilename, "r" );
                    int result = (pFile != NULL) ? ~0 : 0;
                    if ( pFile != NULL )
                    {
                        fclose( pFile );
                    }

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileGetPosition:
                {
                    int file;
                    pMsgPipe->ReadInt( file );
                    int result = ftell( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileGetString:
                {
                    int file, maxChars;
                    pMsgPipe->ReadInt( file );
                    pMsgPipe->ReadInt( maxChars );
                    if ( maxChars > readBufferSize )
                    {
                        delete [] pReadBuffer;
                        pReadBuffer = new char[ maxChars ];
                        readBufferSize = maxChars;
                    }
                    char* result = fgets( pReadBuffer, maxChars, (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileGetStringResult );
                    pMsgPipe->WriteString( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFilePutString:
                {
                    int file;
                    const char* pString;
                    pMsgPipe->ReadInt( file );
                    pMsgPipe->ReadString( pString );
                    int result = fputs( pString, (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileFlush:
                {
                    int file;
                    pMsgPipe->ReadInt( file );
                    int result = fflush( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgRemoveFile:
                {
                    const char* pString;
                    pMsgPipe->ReadString( pString );
                    int result = remove( pString );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgDupHandle:
                {
                    int fileHandle;
                    pMsgPipe->ReadInt( fileHandle );
                    int result = _dup( fileHandle );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

			case kClientMsgDupHandle2:
                {
                    int srcFileHandle;
                    int dstFileHandle;
                    pMsgPipe->ReadInt( srcFileHandle );
                    pMsgPipe->ReadInt( dstFileHandle );
                    int result = _dup2( srcFileHandle, dstFileHandle );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileToHandle:
                {
                    int file;
                    pMsgPipe->ReadInt( file );
                    int result = _fileno( (FILE *) file );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGetTempFilename:
                {
					char* pBuffer = (char *) malloc( L_tmpnam );
					char* pResult = tmpnam( pBuffer );
					int numChars = strlen( pResult ) + 1;

                    pMsgPipe->StartMessage( kServerMsgGetTmpnamResult );
                    pMsgPipe->WriteString( pResult );
                    pMsgPipe->SendMessage();
					free( pBuffer );
                }
                break;

            case kClientMsgRenameFile:
                {
                    const char* pOldName;
                    const char* pNewName;
                    pMsgPipe->ReadString( pOldName );
                    pMsgPipe->ReadString( pNewName );
                    int result = rename( pOldName, pNewName );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgRunSystem:
                {
                    const char* pString;
                    pMsgPipe->ReadString( pString );
                    int result = system( pString );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgChangeDir:
                {
                    const char* pString;
                    pMsgPipe->ReadString( pString );
                    int result = chdir( pString );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgMakeDir:
                {
                    const char* pString;
					int mode;
                    pMsgPipe->ReadString( pString );
                    pMsgPipe->ReadInt( mode );
                    int result = mkdir( pString );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgRemoveDir:
                {
                    const char* pString;
					int mode;
                    pMsgPipe->ReadString( pString );
                    int result = rmdir( pString );

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGetStdIn:
				{
                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( (int) stdin );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGetStdOut:
				{
                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( (int) stdout );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgGetStdErr:
				{
                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( (int) stderr );
                    pMsgPipe->SendMessage();
                }
                break;

            default:
                {
                    printf( "Got unexpected message, type %d len %d \n", msgType, msgLen );
                }
                break;
            }
        }
        else
        {
            done = true;
        }

    }   // end     while ( !done )

    WSACleanup();
    delete [] pReadBuffer;
    delete pMsgPipe;
    return 0;
}
