#include "stdafx.h"
#include <string.h>

#include <nds.h>
#include <dswifi9.h>
#include <netinet/in.h>

#include "..\ForthLib\ForthShell.h"
#include "..\ForthLib\ForthEngine.h"
#include "..\ForthLib\ForthInput.h"
#include "..\ForthLib\ForthThread.h"
#include "..\ForthLib\ForthServer.h"
#include "..\ForthLib\ForthPipe.h"
#include "..\ForthLib\ForthMessages.h"




void DSWifi_Setup()
{
	struct in_addr ip, gateway, mask, dns1, dns2;

	iprintf("\n\n\tSimple Wifi Connection Demo\n\n");
	iprintf("Connecting via WFC data ...\n");

	if(!Wifi_InitDefault(WFC_CONNECT))
	{
		iprintf("Failed to connect!");
	}
	else
	{

		iprintf("Connected\n\n");

		ip = Wifi_GetIPInfo(&gateway, &mask, &dns1, &dns2);
		
		iprintf("ip     : %s\n", inet_ntoa(ip) );
		iprintf("gateway: %s\n", inet_ntoa(gateway) );
		iprintf("mask   : %s\n", inet_ntoa(mask) );
		iprintf("dns1   : %s\n", inet_ntoa(dns1) );
		iprintf("dns2   : %s\n", inet_ntoa(dns2) );
	}
}	

extern "C"
{
	
void ShowIP( long *IP )
{
	iprintf( "%08x %08x\n", IP, *IP );
}

void ShowString( const char* str )
{
	iprintf( "%s", str );
}

};

using namespace std;

int main(int argc, char* argv[], char* envp[])
{
    int iRetVal = 0;
    ForthServerShell *pShell = new ForthServerShell( false );

    ForthInputStream *pInStream;
    
	consoleDemoInit();  //setup the sub screen for printing

#ifdef DEBUG_WITH_NDS_EMULATOR
	const char* woohoo = "55 66 + 33 * %d";
    pInStream = new ForthBufferInputStream( woohoo, strlen(woohoo) );
    iprintf( "Forth: interpret %s\n", woohoo );
    iRetVal = pShell->Run( pInStream );
#else
    SOCKET ServerSocket;
    SOCKET ClientSocket;
    struct sockaddr_in ServerInfo;
    
	DSWifi_Setup();

    ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocket == ~0)		// not sure if this is right value
    {
        printf("error: unable to create the listening socket...\n");
    }
    else
    {
        ServerInfo.sin_family = AF_INET;
        ServerInfo.sin_addr.s_addr = 0;			// was INADDR_ANY;
        ServerInfo.sin_port = htons(27015);
        iRetVal = bind(ServerSocket, (struct sockaddr*) &ServerInfo, sizeof(struct sockaddr));
        if (iRetVal == SOCKET_ERROR)
        {
            printf("error: unable to bind listening socket...\n");
        }
        else
        {
	        int i = 1;
	        ioctl( ServerSocket, FIONBIO, &i );
            iRetVal = listen(ServerSocket, 10);
            if (iRetVal == SOCKET_ERROR)
            {
                printf("error: unable to listen on listening socket...\n");
            }
            else
            {
	            bool waitingForConnection = true;
                while (true)
                {
	                if ( waitingForConnection )
	                {           
		            	printf( "Waiting for a client to connect.\n" );
		            	waitingForConnection = false;
	            	}
                    ClientSocket = accept(ServerSocket, (struct sockaddr *)&ServerInfo, &i);
                    if ( ClientSocket != -1 )
                    {
	                    printf("Incoming connection accepted on %d!\n", ClientSocket );
	                    ForthPipe* pMsgPipe = new ForthPipe( ClientSocket, kServerMsgProcessLine, kServerMsgLimit );
	                    pInStream = new ForthServerInputStream( pMsgPipe );
	                    iRetVal = pShell->Run( pInStream );
	                    delete pMsgPipe;
	                    
	                    printf( "Connection closed on %d!\n", ClientSocket );
	                    closesocket(ClientSocket);
	                    waitingForConnection = true;
                    }
                }
            }
        }
    }
    closesocket(ServerSocket);
    delete pShell;
    return 0;
}

#endif