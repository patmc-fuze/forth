// ForthGuiDlg.h : header file
//

#if !defined(AFX_FORTHGUIDLG_H__FC416F88_AC2E_46DC_A696_CF8F972408E7__INCLUDED_)
#define AFX_FORTHGUIDLG_H__FC416F88_AC2E_46DC_A696_CF8F972408E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class ForthShell;
class ForthBufferInputStream;

#define INPUT_BUFFER_SIZE 1024

/////////////////////////////////////////////////////////////////////////////
// CForthGuiDlg dialog

class CForthGuiDlg : public CDialog
{
// Construction
public:
	CForthGuiDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CForthGuiDlg)
	enum { IDD = IDD_FORTHGUI_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CForthGuiDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

    ForthShell*                 mpShell;
    ForthBufferInputStream*     mpInStream;
    char                        mInBuffer[INPUT_BUFFER_SIZE];
	// Generated message map functions
	//{{AFX_MSG(CForthGuiDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnEnChangeEditInput();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FORTHGUIDLG_H__FC416F88_AC2E_46DC_A696_CF8F972408E7__INCLUDED_)
