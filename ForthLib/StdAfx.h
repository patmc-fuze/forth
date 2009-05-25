#pragma once
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifdef _WINDOWS
// compile for Win2K or newer
#define WINVER 0x0500

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afx.h>
#include <afxwin.h>
#include <afxtempl.h>

#else

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ASSERT(x, ...)
#define TRACE(x, ...)

#endif

// TODO: reference additional headers your program requires here

#include <assert.h>
#include <math.h>

#ifndef ulong
#define ulong unsigned long
#endif

#if 0
#include "Forth.h"
#include "ForthEngine.h"
#include "ForthForgettable.h"
#include "ForthInner.h"
#include "ForthInput.h"
#include "ForthShell.h"
#include "ForthStructs.h"
#include "ForthThread.h"
#include "ForthVocabulary.h"
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

