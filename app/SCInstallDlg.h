// SCInstallDlg.h : header file
//

#if !defined(AFX_SCINSTALLDLG_H__FC37C9BD_1F37_4DF3_95B2_88837B6AB3CD__INCLUDED_)
#define AFX_SCINSTALLDLG_H__FC37C9BD_1F37_4DF3_95B2_88837B6AB3CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CSCInstallDlg dialog

class CSCInstallDlg : public CDialog
{
// Construction
public:
	CSCInstallDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CSCInstallDlg)
	enum { IDD = IDD_SCINSTALL_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCInstallDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CSCInstallDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCINSTALLDLG_H__FC37C9BD_1F37_4DF3_95B2_88837B6AB3CD__INCLUDED_)
