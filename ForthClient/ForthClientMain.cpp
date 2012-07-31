// ForthClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <winsock2.h>

#include "Forth.h"
#include "ForthClient.h"
#include "ForthMessages.h"

#pragma comment(lib, "wininet.lib")
int _tmain(int argc, _TCHAR* argv[])
{
	unsigned long ipAddr = inet_addr( (argc > 1) ? argv[1] : "127.0.0.1" );
	unsigned short portNum = FORTH_SERVER_PORT;

	return ForthClientMainLoop( ipAddr, portNum );
}

