// ForthClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "wininet.lib")

typedef enum
{
    kClientCmdDisplayText,
    kClientCmdSendLine,
    kClientCmdStartLoad,
    kClientCmdGetChar,
};

typedef enum
{
    kServerCmdProcessLine,
    kServerCmdProcessChar,
    kServerCmdPopStream         // sent when file is empty
};


void SendCommandString( SOCKET mSocket, int command, const char* str )
{
    send( mSocket, (const char*) &command, sizeof(command), 0 );
    int len = (str == NULL ) ? 0 : strlen( str );
    send( mSocket, (const char*) &len, sizeof(len), 0 );
    if ( len != 0 )
    {
        send( mSocket, str, len, 0 );
    }
}

int readSocketData( SOCKET s, char *buf, int len )
{
    int nBytes = 0;
    while ( nBytes != len )
    {
        nBytes = recv( s, buf, len, MSG_PEEK );
        if ( nBytes == 0 )
        {
            return 0;
        }
    }
    nBytes = recv( s, buf, len, 0 );
    return nBytes;
}

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

    bool done = false;
    char buffer[ 1024 ];
    FILE*   inputStack[ 16 ];
    int inputStackDepth = 0;
    inputStack[ 0 ] = stdin;
    while ( !done )
    {
        int command;
        int nBytes = readSocketData( ConnectSocket, (char *) &command, sizeof(command) );
        if ( nBytes != sizeof( command ) )
        {
            done = true;
        }
        else
        {
            int len;
            nBytes = readSocketData( ConnectSocket, (char *) &len, sizeof(len) );
            if ( nBytes != sizeof( len ) )
            {
                done = true;
            }
            else
            {
                if ( len != 0 )
                {
                    nBytes = readSocketData( ConnectSocket, buffer, len );
                    if ( nBytes != len )
                    {
                        done = true;
                    }
                }
                buffer[len] = '\0';
                switch ( command )
                {
                    case kClientCmdSendLine:
                        if ( len != 0 )
                        {
                            printf( buffer );
                        }
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
                            SendCommandString( ConnectSocket, kServerCmdProcessLine, pBuffer );
                        }
                        else
                        {
                            SendCommandString( ConnectSocket, kServerCmdPopStream, NULL );
                            fclose( inputStack[ inputStackDepth ] );
                            inputStackDepth--;
                        }
                        break;
                    case kClientCmdStartLoad:
                        {
                            FILE* newInputFile = fopen( buffer, "r" );
                            if ( newInputFile != NULL )
                            {
                                inputStackDepth++;
                                inputStack[ inputStackDepth ] = newInputFile;
                            }
                            else
                            {
                                SendCommandString( ConnectSocket, kServerCmdPopStream, NULL );
                                printf( "Client: failed to open '%s' upon server request!\n", buffer );
                            }
                        }
                        break;

                    case kClientCmdDisplayText:
                        if ( len != 0 )
                        {
                            printf( "%s", buffer );
                        }
                        break;

                    case kClientCmdGetChar:
                        buffer[0] = getchar();
                        buffer[1] = '\0';
                        SendCommandString( ConnectSocket, kServerCmdProcessChar, buffer );
                        break;
                }
            }
        }

    }
    WSACleanup();
    return 0;
}
