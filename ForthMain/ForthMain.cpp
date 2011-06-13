// ForthMain.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ForthMain.h"
#include "..\ForthLib\ForthShell.h"
#include "..\ForthLib\ForthInput.h"

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
    ForthShell *pShell = NULL;
    ForthInputStream *pInStream = NULL;

    // initialize MFC and print an error on failure
    if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
    {
        // TODO: change error code to suit your needs
        cerr << _T("Fatal Error: MFC initialization failed") << endl;
        nRetCode = 1;
    }
    else
    {
        pShell = new ForthShell;
        pShell->SetCommandLine( argc, (const char **) (argv));
        pShell->SetEnvironmentVars( (const char **) envp );
#if 0
        if ( argc > 1 )
        {

            //
            // interpret the forth file named on the command line
            //
            FILE *pInFile = fopen( argv[1], "r" );
            if ( pInFile != NULL )
            {
                pInStream = new ForthFileInputStream(pInFile);
                nRetCode = pShell->Run( pInStream );
                fclose( pInFile );

            }
        }
        else
#endif
        {

            //
            // run forth in interactive mode
            //
            pInStream = new ForthConsoleInputStream;
            nRetCode = pShell->Run( pInStream );

        }
        delete pShell;
    }

    return nRetCode;
}


