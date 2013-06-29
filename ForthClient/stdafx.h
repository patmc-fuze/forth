// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#include <iostream>
#include <tchar.h>

#ifdef WIN32
#include <conio.h>
#include <direct.h>
#include <io.h>
#elif defined(LINUX)
#include <unistd.h>
#endif

// compile for Win2K or newer
#define WINVER 0x0500

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

// TODO: reference additional headers your program requires here
