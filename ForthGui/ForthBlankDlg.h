//////////////////////////////////////////////////////////////////////
//
// ForthBlankDlg.h: interface for Forth dialog support
//
//////////////////////////////////////////////////////////////////////

#if !defined(_FORTH_BLANK_DLG_H_INCLUDED_)
#define _FORTH_BLANK_DLG_H_INCLUDED_

#include "afxcmn.h"
#include "resource.h"		// main symbols

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ForthBlankDlg : public CDialog
{
public:
	ForthBlankDlg();
	~ForthBlankDlg();

    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// Dialog Data
	//{{AFX_DATA(ForthBlankDlg)
	enum { IDD = IDD_BLANK_DIALOG };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ForthBlankDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(ForthBlankDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
