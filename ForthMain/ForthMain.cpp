// ForthMain.cpp : Defines the entry point for the console application.
//

#include "StdAfx.h"

#if defined(WIN32) && !defined(MINGW)
#define AFX_BUILD 1
#endif

#if AFX_BUILD
#include "ForthMain.h"
#endif
#include "ForthShell.h"
#include "ForthInput.h"


#if AFX_BUILD

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;
#endif

using namespace std;

static boolean InitSystem()
{
#if AFX_BUILD

    // initialize MFC and print an error on failure
    if ( !AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0) )
    {
        // TODO: change error code to suit your needs
        cerr << _T("Fatal Error: MFC initialization failed") << endl;
		return false;
	}
#endif
	return true;
}

#if AFX_BUILD
int _tmain( int argc, TCHAR* argv[], TCHAR* envp[] )
#else
int main( int argc, char* argv[], char* envp[] )
#endif
{
    int nRetCode = 0;
    ForthShell *pShell = NULL;
    ForthInputStream *pInStream = NULL;

	if ( !InitSystem() )
	{
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


