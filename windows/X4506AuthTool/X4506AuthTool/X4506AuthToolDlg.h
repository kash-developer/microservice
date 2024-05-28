
// X4506AuthToolDlg.h : header file
//

#pragma once

#include "DeviceController.h"
#include "afxwin.h"

#include <string>
#include <vector>
#include <queue>
#include <thread>
#include "afxcmn.h"

#include <DeviceController.h>

struct TestProgress {
	std::string m_name;
	int m_result;
	std::string m_description;
};

// CX4506AuthToolDlg dialog
class CX4506AuthToolDlg : public CDialogEx
{
// Construction
public:
	CX4506AuthToolDlg(CWnd* pParent = NULL);	// standard constructor
	~CX4506AuthToolDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_X4506AUTHTOOL_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	int init();

private:
	DeviceController m_controller;
	std::vector<struct TestItem> m_test_items;
	bool m_run_flag;
	std::thread* m_tid;
	std::thread* m_tid2;

	std::queue<struct TestItem> m_event_queue;

	bool m_is_win_initialized;
	RECT m_list_rect;

	CComboBox m_combo_device_id;
	CEdit m_edit_device_ip;
	CEdit m_edit_device_port;
	CButton m_btn_start;
	CListCtrl m_list_test_list;
	CButton m_checkbox_device_test;
	CEdit m_edit_guide;

private:
	std::string testProgress2String();

public:
	void testEventCallback(struct TestItem event);
	void checkTestStoppedThread();
	void setGuideText(std::string str);

	void getInputThread();

public:
	afx_msg void OnCbnSelchangeComboDeviceId();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedCheckTestTypeDevice();
	afx_msg void OnBnClickedButtonShowLog();
};
