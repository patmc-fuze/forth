// ForthServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string.h>
#include <winsock2.h>
#include "..\ForthLib\ForthShell.h"
#include "..\ForthLib\ForthEngine.h"
#include "..\ForthLib\ForthInput.h"
#include "..\ForthLib\ForthThread.h"
#include "..\ForthLib\ForthPipe.h"
#include "..\ForthLib\ForthServer.h"
#include "..\ForthLib\ForthMessages.h"


//int _tmain(int argc, _TCHAR* argv[])
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	return ForthServerMainLoop( FORTH_SERVER_PORT );
}
