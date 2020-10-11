// ForthDLL.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "ForthDLL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CForthDLLApp

BEGIN_MESSAGE_MAP(CForthDLLApp, CWinApp)
END_MESSAGE_MAP()


// TODO: OutputToLogger is copied from ForthMain.cpp, stick it in its own file
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

// CForthDLLApp construction

CForthDLLApp::CForthDLLApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CForthDLLApp object

CForthDLLApp theApp;


// CForthDLLApp initialization

BOOL CForthDLLApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}

ForthEngine* CreateForthEngine()
{
	return new ForthEngine;
}

ForthShell* CreateForthShell(int argc, const char ** argv, const char ** envp, ForthEngine *pEngine, ForthExtension *pExtension, int shellStackLongs )
{
	return new ForthShell(argc, argv, envp, pEngine, pExtension, shellStackLongs );
}

ForthBufferInputStream* CreateForthBufferInputStream( const char *pDataBuffer, int dataBufferLen, int bufferLen, bool deleteWhenEmpty)
{
    ForthBufferInputStream* inStream = new ForthBufferInputStream( pDataBuffer, dataBufferLen, bufferLen );
    inStream->SetDeleteWhenEmpty(deleteWhenEmpty);
    return inStream;
}

ForthConsoleInputStream* CreateForthConsoleInputStream( int bufferLen, bool deleteWhenEmpty )
{
    ForthConsoleInputStream* inStream = new ForthConsoleInputStream( bufferLen );
    inStream->SetDeleteWhenEmpty(deleteWhenEmpty);
    return inStream;
}

ForthFileInputStream* CreateForthFileInputStream( FILE *pInFile, const char *pFilename, int bufferLen, bool deleteWhenEmpty)
{
    ForthFileInputStream* inStream = new ForthFileInputStream( pInFile, pFilename, bufferLen );
    inStream->SetDeleteWhenEmpty(deleteWhenEmpty);
    return inStream;
}

void DeleteForthEngine( ForthEngine* pEngine )
{
	delete pEngine;
}

void DeleteForthShell( ForthShell* pShell )
{
	delete pShell;
}

void DeleteForthFiber( ForthFiber* pFiber )
{
	delete pFiber;
}

void DeleteForthInputStream( ForthInputStream* pStream )
{
	delete pStream;
}

