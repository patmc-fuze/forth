// ForthMain.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ForthMain.h"
//#include "..\ForthLib\Forth.h"
#include "..\ForthLib\ForthEngine.h"
#include "..\ForthLib\ForthThread.h"
#include "..\ForthLib\ForthShell.h"
#include "..\ForthLib\ForthVocabulary.h"

#if 0
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;
    ForthShell *pShell;

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
        pShell = new ForthShell( NULL, NULL );
        nRetCode = pShell->Run( (argc > 1) ? argv[1] : NULL );
        delete pShell;
	}

	return nRetCode;
}


