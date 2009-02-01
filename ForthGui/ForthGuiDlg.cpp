// ForthGuiDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ForthGui.h"
#include "ForthGuiDlg.h"

#include "../ForthLib/ForthShell.h"
#include "../ForthLib/ForthInput.h"
#include "../ForthLib/ForthEngine.h"
#include ".\forthguidlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
///
//  stuff to connect Forth output to output pane edit control
//
struct sStreamInScratch
{
	sStreamInScratch( const char* buffer )
		:	m_buffer( buffer )
		,	m_length( strlen( buffer ) )
	{}
	const char* m_buffer;
	int			m_length;
};

DWORD CALLBACK StreamInCallback( DWORD dwCookie, LPBYTE outBuffer, LONG outBufferLen, LONG* bytesWritten )
{
	sStreamInScratch* const scratch = reinterpret_cast<sStreamInScratch*>( dwCookie );
	*bytesWritten = min( outBufferLen, scratch->m_length );		

	memcpy( outBuffer, scratch->m_buffer, *bytesWritten );
	
	scratch->m_length -= *bytesWritten;
	scratch->m_buffer += *bytesWritten;

	return 0;
}

void StreamToOutputPane( CRichEditCtrl* pOutEdit, const char* pMessage )
{
	// Grab the old selection and cursor placement.
	CHARRANGE oldSelection;
	pOutEdit->GetSel( oldSelection );
	const long ixStart = pOutEdit->GetWindowTextLength();
	const int oldLine = pOutEdit->LineFromChar( oldSelection.cpMax );
	const int numLines = pOutEdit->GetLineCount();

	// Move to the end of the text.
	pOutEdit->SetSel( ixStart, -1 );

	// Stream in the message text.
	EDITSTREAM es;
	sStreamInScratch scratch( pMessage );
	es.dwCookie = reinterpret_cast<DWORD>( &scratch );
	es.pfnCallback = StreamInCallback;
	es.dwError = 0;
	pOutEdit->StreamIn( SF_TEXT | SFF_SELECTION, es );

	// Scroll the window if the cursor was on the last line and the selection was empty. Otherwise, reset the selection.
	if ( oldSelection.cpMin != oldSelection.cpMax || pOutEdit->LineFromChar( oldSelection.cpMin ) != (numLines - 1) )
	{
		pOutEdit->SetSel( oldSelection );
	}
	else
	{
		pOutEdit->SetSel( -1, -1 );
		pOutEdit->LineScroll( pOutEdit->GetLineCount() - oldLine );
	}
}

CRichEditCtrl* gpOutEdit = NULL;
void ForthOutRoutine( ForthCoreState *pCore, const char *pBuff )
{
	StreamToOutputPane( gpOutEdit, pBuff );
}

/////////////////////////////////////////////////////////////////////////////
// CForthGuiDlg dialog

CForthGuiDlg::CForthGuiDlg(CWnd* pParent /*=NULL*/)
: CDialog(CForthGuiDlg::IDD, pParent)
, mpShell( NULL )
, mpInStream( NULL )
{
	//{{AFX_DATA_INIT(CForthGuiDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CForthGuiDlg::~CForthGuiDlg()
{
	DestroyForth();
}

void CForthGuiDlg::CreateForth()
{
	mpShell = new ForthShell;
	ForthEngine* pEngine = mpShell->GetEngine();
	pEngine->SetConsoleOut( ForthOutRoutine, GetDlgItem( IDC_RICHEDIT_OUTPUT ) );
	pEngine->ResetConsoleOut( pEngine->GetCoreState()->pThread );
    mpInStream = new ForthBufferInputStream( mInBuffer, INPUT_BUFFER_SIZE );
    mpShell->GetInput()->PushInputStream( mpInStream );
}

void CForthGuiDlg::DestroyForth()
{
	if ( mpShell )
	{
		delete mpShell;
		// the shell destructor deletes all the streams on the input stack, including mpInStream
		mpShell = NULL;
		mpInStream = NULL;
	}
}

void CForthGuiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CForthGuiDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_RICHEDIT_OUTPUT, mOutputEdit);
}

BEGIN_MESSAGE_MAP(CForthGuiDlg, CDialog)
	//{{AFX_MSG_MAP(CForthGuiDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CForthGuiDlg message handlers

BOOL CForthGuiDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	gpOutEdit = static_cast<CRichEditCtrl*>(GetDlgItem( IDC_RICHEDIT_OUTPUT ));
	CreateForth();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CForthGuiDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CForthGuiDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CForthGuiDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CForthGuiDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    //OnOK();
    CEdit *pEdit = (CEdit* )GetDlgItem( IDC_EDIT_INPUT );
	CRichEditCtrl *pOutEdit = (CRichEditCtrl*) GetDlgItem( IDC_RICHEDIT_OUTPUT );
	ForthInputStack* pInput = mpShell->GetInput();
    eForthResult result = kResultOk;

	pEdit->GetWindowText( mInBuffer, INPUT_BUFFER_SIZE - 4 );
	StreamToOutputPane( pOutEdit, mInBuffer );
	StreamToOutputPane( pOutEdit, "\r\n" );
	pEdit->GetWindowText( mInBuffer, INPUT_BUFFER_SIZE - 4 );
    result = mpShell->InterpretLine( mInBuffer );
	StreamToOutputPane( pOutEdit, "\r\n" );
	// clear input buffer
	pEdit->SetSel( 0, -1 );
	pEdit->ReplaceSel( "" );

	if ( result == kResultOk )
	{
		// to implement the "load" op, we must do 
		while ( pInput->InputStream() != mpInStream )
		{
			// get a line
			// interpret the line
			bool bQuit = false;
			const char*pBuffer = pInput->GetLine( "" );
			if ( pBuffer == NULL )
			{
				bQuit = pInput->PopInputStream();
			}

			if ( !bQuit )
			{
			    result = mpShell->InterpretLine();
			}
			if ( bQuit || (result != kResultOk) )
			{
				break;
			}
		}
	}
	switch ( result )
	{
	case kResultExitShell:
		// exit program
		OnCancel();
		break;

	case kResultFatalError:
		DestroyForth();
		CreateForth();
		break;

	default:
		break;
	}
}

