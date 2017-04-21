
// newLCheckLogDlg.cpp : implementation file
//

#include "stdafx.h"
#include "newLCheckLog.h"
#include "newLCheckLogDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CnewLCheckLogDlg dialog




CnewLCheckLogDlg::CnewLCheckLogDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CnewLCheckLogDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CnewLCheckLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CnewLCheckLogDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()


// CnewLCheckLogDlg message handlers

BOOL CnewLCheckLogDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CnewLCheckLogDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CnewLCheckLogDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/**
 * @brief  Draw lines in Device Context
 *
 * @pre    None.
 * @post   Lines are drawed.
 * @param  None.
 * @return None.
 */
void CnewLCheckLogDlg::DrawLine(CDC* dc, COLORREF color, int x1, int y1, int x2, int y2)
{
	CPen pen;
	pen.CreatePen(PS_DOT,2,color);
	CPen* PenOrg = dc->SelectObject(&pen);

	dc->MoveTo(x1,y1);
	dc->LineTo(x2,y2);

	dc->SelectObject(PenOrg);
	pen.DeleteObject();
}

/**
 * @brief  Write texts in Device Context
 *
 * @pre    None.
 * @post   Texts are write.
 * @param  None.
 * @return None.
 */
void CnewLCheckLogDlg::WriteText(CDC* dc, COLORREF color, int x, int y, TCHAR text[])
{
	// Initializes a CFont object with the specified characteristics.
	CFont font;
	VERIFY(font.CreateFont(
	   12,                        // nHeight
	   0,                         // nWidth
	   0,                         // nEscapement
	   0,                         // nOrientation
	   FW_NORMAL,                 // nWeight
	   FALSE,                     // bItalic
	   FALSE,                     // bUnderline
	   0,                         // cStrikeOut
	   ANSI_CHARSET,              // nCharSet
	   OUT_DEFAULT_PRECIS,        // nOutPrecision
	   CLIP_DEFAULT_PRECIS,       // nClipPrecision
	   DEFAULT_QUALITY,           // nQuality
	   DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
	   _T("Arial")));             // lpszFacename

	dc->SetTextColor(color);
	dc->TextOut(x, y, text);

	CFont* defFont = dc->SelectObject(&font);
	dc->SelectObject(defFont);
	font.DeleteObject();
}

/**
 * @brief  Draw points to display number values in Device Context
 *
 * @pre    None.
 * @post   Points are draw.
 * @param  None.
 * @return None.
 */

void CnewLCheckLogDlg::DrawPoint(CDC* dc, COLORREF color, int x, int y)
{
	int size = 1;
	CPen	pen;
	pen.CreatePen(PS_SOLID,3,color);
	CPen* PenOrg = dc->SelectObject(&pen);

	dc->Ellipse(x-size,y-size,x+size,y+size);

	dc->SelectObject(PenOrg);
	pen.DeleteObject();
}
