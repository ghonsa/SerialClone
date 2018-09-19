// SCInstall.h : main header file for the SCINSTALL application
//

#if !defined(AFX_SCINSTALL_H__E3DDC972_8BA2_44DE_BC5B_24F24D2662E5__INCLUDED_)
#define AFX_SCINSTALL_H__E3DDC972_8BA2_44DE_BC5B_24F24D2662E5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CSCInstallApp:
// See SCInstall.cpp for the implementation of this class
//

class CSCInstallApp : public CWinApp
{
public:
	CSCInstallApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCInstallApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CSCInstallApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCINSTALL_H__E3DDC972_8BA2_44DE_BC5B_24F24D2662E5__INCLUDED_)
