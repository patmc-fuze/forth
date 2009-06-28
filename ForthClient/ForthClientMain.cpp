// ForthClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include "ForthPipe.h"
#include "ForthMessages.h"

#pragma comment(lib, "wininet.lib")


int _tmain(int argc, _TCHAR* argv[])
{
    //----------------------
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != NO_ERROR)
        printf("Error at WSAStartup()\n");

    //----------------------
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket;
    //ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ConnectSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    sockaddr_in clientService; 
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr( (argc > 1) ? argv[1] : "127.0.0.1" );
    clientService.sin_port = htons( 27015 );

    //----------------------
    // Connect to server.
    if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
        printf( "Failed to connect.\n" );
        WSACleanup();
        return -1;
    }

    ForthPipe* pMsgPipe = new ForthPipe( ConnectSocket, kClientMsgDisplayText, kClientMsgLimit );
    bool done = false;
    char buffer[ 1024 ];
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
                    char* pPrompt;
                    int promptLen;
                    if ( pMsgPipe->ReadCountedData( pPrompt, promptLen ) )
                    {
                        if ( promptLen != 0 )
                        {
                            printf( "%s", pPrompt );
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
                    char* pFilename;
                    pMsgPipe->ReadString( pFilename );
                    FILE* newInputFile = fopen( pFilename, "r" );
                    if ( newInputFile != NULL )
                    {
                        inputStackDepth++;
                        inputStack[ inputStackDepth ] = newInputFile;
                    }
                    else
                    {
                        pMsgPipe->StartMessage( kServerMsgPopStream );
                        pMsgPipe->SendMessage();
                        printf( "Client: failed to open '%s' upon server request!\n", buffer );
                    }
                }
                break;

            case kClientMsgDisplayText:
                {
                    char* pText;
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
    delete pMsgPipe;
    return 0;
}
