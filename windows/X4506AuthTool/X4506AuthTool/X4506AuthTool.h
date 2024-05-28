
// X4506AuthTool.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CX4506AuthToolApp:
// See X4506AuthTool.cpp for the implementation of this class
//

class CX4506AuthToolApp : public CWinApp
{
public:
	CX4506AuthToolApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CX4506AuthToolApp theApp;
