// ForthDLL.h : main header file for the ForthDLL DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#include "ForthEngine.h"

__declspec(dllexport) ForthEngine* CreateForthEngine();
__declspec(dllexport) ForthShell* CreateForthShell(int argc, const char ** argv, const char ** envp,
    ForthEngine *pEngine = NULL, ForthExtension *pExtension = NULL, int shellStackLongs = 1024 );

__declspec(dllexport) ForthBufferInputStream* CreateForthBufferInputStream(
    const char *pDataBuffer, int dataBufferLen, int bufferLen = DEFAULT_INPUT_BUFFER_LEN, bool deleteWhenEmpty=true);
__declspec(dllexport) ForthConsoleInputStream* CreateForthConsoleInputStream(
    int bufferLen = DEFAULT_INPUT_BUFFER_LEN, bool deleteWhenEmpty=true);
__declspec(dllexport) ForthFileInputStream* CreateForthFileInputStream(
    FILE *pInFile, const char *pFilename, int bufferLen, bool deleteWhenEmpty=true);

__declspec(dllexport) void DeleteForthEngine( ForthEngine* pEngine );
__declspec(dllexport) void DeleteForthShell( ForthShell* pShell );
__declspec(dllexport) void DeleteForthInputStream( ForthInputStream* pStream );

// CForthDLLApp
// See ForthDLL.cpp for the implementation of this class
//

class CForthDLLApp : public CWinApp
{
public:
	CForthDLLApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
