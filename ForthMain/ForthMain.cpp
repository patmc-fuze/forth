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


static HANDLE hLoggingPipe = INVALID_HANDLE_VALUE;

void OutputToLogger(const char* pBuffer)
{
    //OutputDebugString(buffer);

    DWORD dwWritten;
    int bufferLen = 1 + strlen(pBuffer);

    if (hLoggingPipe == INVALID_HANDLE_VALUE)
    {
        hLoggingPipe = CreateFile(TEXT("\\\\.\\pipe\\ForthLogPipe"),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
    }
    if (hLoggingPipe != INVALID_HANDLE_VALUE)
    {
        WriteFile(hLoggingPipe,
            pBuffer,
            bufferLen,   // = length of string + terminating '\0' !!!
            &dwWritten,
            NULL);

        //CloseHandle(hPipe);
    }

    return;
}

static bool InitSystem()
{
#if AFX_BUILD

    // initialize MFC and print an error on failure
    if ( !AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0) )
    {
        // TODO: change error code to suit your needs
        cerr << _T("Fatal Error: MFC initialization failed") << endl;
		return false;
	}
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE)
	{
		DWORD dwMode = 0;
		GetConsoleMode(hOut, &dwMode);
		dwMode |= 4;// ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hOut, dwMode);
	}

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	if (hIn != INVALID_HANDLE_VALUE)
	{
		DWORD dwMode = 0;
		GetConsoleMode(hIn, &dwMode);
		dwMode |= 0x200;// ENABLE_VIRTUAL_TERMINAL_INPUT;
		SetConsoleMode(hIn, dwMode);
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
		nRetCode = 1;
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

	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);    // or _CRTDBG_FILE_STDERR?
	if (_CrtDumpMemoryLeaks())        // returns TRUE if a memory leak was found
	{
		printf("\nHit RETURN to exit\n");
		getchar();
	}
	return nRetCode;
}


