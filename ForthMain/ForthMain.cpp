// ForthMain.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ForthMain.h"
#include "..\ForthParser\ForthParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;


void
traceAction( char   *pToken,
             int    parseFlags )
{
    TRACE( "token [%s]\n", pToken );
}


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.
        char lineBuff[256];
        FILE *pInFile = fopen( argv[1], "r" );
        if ( pInFile != NULL ) {
            while ( fgets( lineBuff, sizeof(lineBuff), pInFile ) != NULL ) {
                ForthParseLine( lineBuff, traceAction );
                TRACE( "End parse line\n" );
            }
        }
	}

	return nRetCode;
}


