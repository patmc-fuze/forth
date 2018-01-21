//////////////////////////////////////////////////////////////////////
//
// ForthClient.cpp: Forth client support
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#pragma comment(lib, "wininet.lib")

#include <stdio.h>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include "Forth.h"
#include "ForthPipe.h"
#include "ForthClient.h"
#include "ForthMessages.h"

#ifndef SOCKADDR
#define SOCKADDR struct sockaddr
#endif

namespace
{
	void ErrorExit( const char* message )
	{
#ifdef WIN32
        shutdownSockets();
#else
		// TODO
#endif
	}
}

int ForthClientMainLoop( ForthEngine *pEngine, const char* pServerStr, unsigned short portNum )
{
    char errorMessage[128];
	unsigned long ipAddress;
#ifdef WIN32
    startupSockets();
#else
		// TODO
#endif

	struct hostent *host = gethostbyname(pServerStr);
	ipAddress = *((unsigned long *)(host->h_addr_list[0]));
	printf( "Connecting to host %s (%d) on port %d\n", pServerStr, ipAddress, portNum );

    //----------------------
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket;
    //ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
#ifdef WIN32
    if (ConnectSocket == INVALID_SOCKET)
    {
        sprintf( errorMessage, "Error at socket(): %ld", WSAGetLastError() );
        ErrorExit( errorMessage );
        return -1;
    }
#else
    if (ConnectSocket == -1)
    {
        sprintf( errorMessage, "Error at socket(): %d", errno );
        ErrorExit( errorMessage );
        return -1;
    }
#endif

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    sockaddr_in clientService; 
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = ipAddress;
    clientService.sin_port = htons( portNum );

    //----------------------
    // Connect to server.
#ifdef WIN32
    if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
        sprintf( errorMessage, "Failed to connect." );
        ErrorExit( errorMessage );
        return -1;
    }
#else
    if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) != 0 ) {
        sprintf( errorMessage, "Failed to connect." );
        ErrorExit( errorMessage );
        return -1;
    }
#endif

    ForthPipe* pMsgPipe = new ForthPipe( ConnectSocket, kClientMsgDisplayText, kClientMsgLimit );
    bool done = false;
    char buffer[ 1024 ];
    int readBufferSize = 16384;
    char* pReadBuffer = (char *)__MALLOC( readBufferSize );
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
						pReadBuffer = (char *)__REALLOC(pReadBuffer, numBytes);
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
						pReadBuffer = (char *)__REALLOC(pReadBuffer, maxChars);
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

			case kClientMsgFilePutChar:
			{
				int file;
				int ch;
				pMsgPipe->ReadInt(file);
				pMsgPipe->ReadInt(ch);
				int result = fputc(ch, (FILE *)file);

				pMsgPipe->StartMessage(kServerMsgFileOpResult);
				pMsgPipe->WriteInt(result);
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
#ifdef WIN32
                    int result = _dup( fileHandle );
#else
                    int result = dup( fileHandle );
#endif

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
#ifdef WIN32
                    int result = _dup2( srcFileHandle, dstFileHandle );
#else
                    int result = dup2( srcFileHandle, dstFileHandle );
#endif

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgFileToHandle:
                {
                    int file;
                    pMsgPipe->ReadInt( file );
#ifdef WIN32
                    int result = _fileno( (FILE *) file );
#else
                    int result = fileno( (FILE *) file );
#endif

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
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
#ifdef WIN32
                    int result = mkdir( pString );
#else
                    int result = mkdir( pString, mode );
#endif

                    pMsgPipe->StartMessage( kServerMsgFileOpResult );
                    pMsgPipe->WriteInt( result );
                    pMsgPipe->SendMessage();
                }
                break;

            case kClientMsgRemoveDir:
                {
                    const char* pString;
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

#ifdef WIN32
    shutdownSockets();
#else
    // TODO
#endif
    __FREE( pReadBuffer );
    delete pMsgPipe;
    return 0;
}
