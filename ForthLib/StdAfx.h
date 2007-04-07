// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__0A0B1769_8766_11D4_A3C4_84B80AB96A51__INCLUDED_)
#define AFX_STDAFX_H__0A0B1769_8766_11D4_A3C4_84B80AB96A51__INCLUDED_

// compile for Win2K or newer
#define WINVER 0x0500

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afx.h>
#include <afxwin.h>

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

#endif // !defined(AFX_STDAFX_H__0A0B1769_8766_11D4_A3C4_84B80AB96A51__INCLUDED_)
