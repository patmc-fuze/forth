// ForthServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string.h>
#include <winsock2.h>
#include "..\ForthLib\ForthShell.h"
#include "..\ForthLib\ForthEngine.h"
#include "..\ForthLib\ForthInput.h"
#include "..\ForthLib\ForthThread.h"
#include "..\ForthLib\ForthServer.h"
#include "..\ForthLib\ForthPipe.h"
#include "..\ForthLib\ForthMessages.h"

using namespace std;

//int _tmain(int argc, _TCHAR* argv[])
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    WSADATA WsaData;
    SOCKET ServerSocket;
    SOCKET ClientSocket;
    struct sockaddr_in ServerInfo;
    int iRetVal = 0;
    char* pszSendData = "Hello world!";
    ForthServerShell *pShell;
    ForthInputStream *pInStream;

    WSAStartup(0x202, &WsaData);

    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == INVALID_SOCKET)
    {
        printf("error: unable to create the listening socket...\n");
    }
    else
    {
        ServerInfo.sin_family = AF_INET;
        ServerInfo.sin_addr.s_addr = INADDR_ANY;
        ServerInfo.sin_port = htons(27015);
        iRetVal = bind(ServerSocket, (struct sockaddr*) &ServerInfo, sizeof(struct sockaddr));
        if (iRetVal == SOCKET_ERROR)
        {
            printf("error: unable to bind listening socket...\n");
        }
        else
        {
            iRetVal = listen(ServerSocket, 10);
            if (iRetVal == SOCKET_ERROR)
            {
                printf("error: unable to listen on listening socket...\n");
            }
            else
            {
                pShell = new ForthServerShell;
                while (true)
                {
                    printf( "Waiting for a client to connect.\n" );
                    ClientSocket = accept(ServerSocket, NULL, NULL);
                    printf("Incoming connection accepted!\n");

                    ForthPipe* pMsgPipe = new ForthPipe( ClientSocket, kServerMsgProcessLine, kServerMsgLimit );
                    pInStream = new ForthServerInputStream( pMsgPipe );
                    iRetVal = pShell->Run( pInStream );

                    //send(ClientSocket, pszSendData, strlen(pszData), 0);
                    closesocket(ClientSocket);
                }
                delete pShell;
            }
        }
    }
    closesocket(ServerSocket);
    WSACleanup();
    return 0;
}
