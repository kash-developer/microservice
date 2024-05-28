
// X4506AuthToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "X4506AuthTool.h"
#include "X4506AuthToolDlg.h"
#include "afxdialogex.h"

#include "DeviceController.h"
#include <trace.h>
#include <thread>
#include <string>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _DEBUG
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CX4506AuthToolDlg dialog



CX4506AuthToolDlg::CX4506AuthToolDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_X4506AUTHTOOL_DIALOG, pParent)
{
	m_is_win_initialized = false;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CX4506AuthToolDlg::~CX4506AuthToolDlg()
{
	m_run_flag = false;
	if (m_tid != NULL) {
		m_tid->join();
		delete m_tid;
		m_tid = NULL;
	}
}

void CX4506AuthToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_DEVICE_ID, m_combo_device_id);
	DDX_Control(pDX, IDC_BUTTON_START, m_btn_start);
	DDX_Control(pDX, IDC_LIST_TEST_LIST, m_list_test_list);
	DDX_Control(pDX, IDC_CHECK_TEST_TYPE_DEVICE, m_checkbox_device_test);
	DDX_Control(pDX, IDC_EDIT_GUIDE, m_edit_guide);
	DDX_Control(pDX, IDC_EDIT_DEVICE_IP, m_edit_device_ip);
	DDX_Control(pDX, IDC_EDIT_DEVICE_PORT, m_edit_device_port);
}

BEGIN_MESSAGE_MAP(CX4506AuthToolDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_COMBO_DEVICE_ID, &CX4506AuthToolDlg::OnCbnSelchangeComboDeviceId)
	ON_BN_CLICKED(IDC_BUTTON_START, &CX4506AuthToolDlg::OnBnClickedButtonStart)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CHECK_TEST_TYPE_DEVICE, &CX4506AuthToolDlg::OnBnClickedCheckTestTypeDevice)
	ON_BN_CLICKED(IDC_BUTTON_SHOW_LOG, &CX4506AuthToolDlg::OnBnClickedButtonShowLog)
END_MESSAGE_MAP()


// CX4506AuthToolDlg message handlers

BOOL CX4506AuthToolDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
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
	//return TRUE;
	RECT win_rect, list_rect;

	traceSetLevel(99);

	GetWindowRect(&win_rect);
	m_list_test_list.GetWindowRect(&list_rect);
	trace("win: %d, %d, %d, %d", win_rect.left, win_rect.top, win_rect.right, win_rect.bottom);
	trace("list pos: %d, %d, %d, %d", list_rect.left, list_rect.top, list_rect.right, list_rect.bottom);
	trace("list size: %d, %d, %d, %d", list_rect.left, list_rect.top, list_rect.right - list_rect.left, list_rect.bottom - list_rect.top);

	m_list_rect = list_rect;
	m_list_rect.top -= 30;
	m_list_rect.right = m_list_rect.left;
	m_list_rect.bottom = m_list_rect.left;
	trace("list2: %d, %d, %d, %d", m_list_rect.left, m_list_rect.top, m_list_rect.right, m_list_rect.bottom);

	if (init() < 0) {
		return FALSE;
	}

	m_is_win_initialized = true;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CX4506AuthToolDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (m_is_win_initialized == true) {
		//trace("list: %d, %d, %d, %d", m_list_rect.left, m_list_rect.top, cx - m_list_rect.right - m_list_rect.left, cy - m_list_rect.bottom - m_list_rect.top);
		m_list_test_list.MoveWindow(m_list_rect.left, m_list_rect.top, cx - m_list_rect.right - m_list_rect.left, cy - m_list_rect.bottom - m_list_rect.top);
	}
}


void CX4506AuthToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CX4506AuthToolDlg::OnPaint()
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
HCURSOR CX4506AuthToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#include "httpu_server.h"
int CX4506AuthToolDlg::init()
{
	std::vector<struct DeviceSupportList> device_list;
	std::string tmp_string;
	char hex_str[16];
	LVCOLUMN lvColumn;
	int index;
	int initial_width, initial_height;

	SetWindowText(_T("MicroService Test Tool"));

	//create controller
	if (m_controller.init(this) < 0) {
		AfxMessageBox(L"DeviceController init failed.");
		return -1;
	}

	//target device combo box
	//m_combo_device_id.AddString(CString("All Devices"));
	//m_combo_device_id.SetItemData(0, 0);

	device_list = DeviceController::getDeviceSupport();
	for (unsigned int i = 0; i < device_list.size(); i++) {
		sprintf_s(hex_str, 16, "0x%02x", device_list[i].m_device_id);
		tmp_string = device_list[i].m_type;
		tmp_string += std::string(" (") + std::string(hex_str) + std::string(")");

		m_combo_device_id.AddString(CString(tmp_string.c_str()));
		//m_combo_device_id.SetItemData(i + 1, device_list[i].m_device_id);
		m_combo_device_id.SetItemData(i, device_list[i].m_device_id);
	}
	m_combo_device_id.SetCurSel(0);

	//start button
	m_btn_start.SetWindowText(L"Start");

	//list control
	m_list_test_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

	index = 0;
	memset(&lvColumn, 0, sizeof(LVCOLUMN));

	lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvColumn.fmt = LVCFMT_RIGHT;
	lvColumn.cx = 250;
	lvColumn.pszText = _T("Test Name");
	m_list_test_list.InsertColumn(index, &lvColumn);
	index++;

	lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvColumn.fmt = LVCFMT_RIGHT;
	lvColumn.cx = 100;
	lvColumn.pszText = _T("Result");
	m_list_test_list.InsertColumn(index, &lvColumn);
	index++;

	lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvColumn.fmt = LVCFMT_RIGHT;
	lvColumn.cx = 300;
	lvColumn.pszText = _T("Description");
	m_list_test_list.InsertColumn(index, &lvColumn);
	index++;

	//run controller 
	if (m_controller.run() < 0) {
		AfxMessageBox(L"run controller failed.");
		return -1;
	}

	m_run_flag = true;
	m_tid = new std::thread(&CX4506AuthToolDlg::checkTestStoppedThread, this);
	m_tid2 = new std::thread(&CX4506AuthToolDlg::getInputThread, this);

	//*
	initial_width = 800;
	initial_height = 600;
	MoveWindow(0, 0, initial_width, initial_height);
	m_list_test_list.MoveWindow(m_list_rect.left, 
								m_list_rect.top, 
								initial_width - m_list_rect.right - m_list_rect.left - 18, 
								initial_height - m_list_rect.bottom - m_list_rect.top - 45);

	m_edit_device_ip.SetWindowText(L"127.0.0.1");
	m_edit_device_port.SetWindowText(L"28880");



	//set default device
	OnCbnSelchangeComboDeviceId();

	return 0;
}

void CX4506AuthToolDlg::getInputThread()
{
	char c;

	while (true) {
		c = getchar();
	}
}

void CX4506AuthToolDlg::setGuideText(std::string str)
{
	m_edit_guide.SetWindowText(CString(str.c_str()));
}

void CX4506AuthToolDlg::checkTestStoppedThread()
{
	int test_status;
	bool flag;
	struct TestItem event;
	CString tmp_str;
	std::string test_result_str;

	flag = false;
	while(m_run_flag) {
		if (m_event_queue.size() > 0) {
			event = m_event_queue.front();
			m_event_queue.pop();

			trace("event: %d: %d, %s", event.m_id, event.m_result, event.m_name.c_str());
			if (event.m_id == TEST_EVENT_COMPLETE) {
				trace("test complete event!!!!!");
			}
			else if (event.m_id >= (int)m_test_items.size()) {
				tracee("invalid test id: %d", event.m_id);
			}
			else {
				m_test_items[event.m_id] = event;

				if (event.m_result == TEST_RESULT_SUCCESS) {
					tmp_str = CString("success");
				}
				else if (event.m_result == TEST_RESULT_FAIL) {
					tmp_str = CString("fail");
				}
				else if (event.m_result == TEST_RESULT_SKIPPED) {
					tmp_str = CString("skip");
				}
				else {
					tmp_str = CString("not_tested");
					continue;
				}
				m_list_test_list.SetItemText(event.m_id, 1, tmp_str);
				m_list_test_list.SetItemText(event.m_id, 2, CString(event.m_description.c_str()));
			}
		}
		else {
			test_status = m_controller.getTestStatus();
			if (test_status == CONTROLLER_TEST_FLAG_RUN) {
				flag = true;
			}
			if (test_status == CONTROLLER_TEST_FLAG_STOP) {
				//trace("test complete. set button test start.");
				m_btn_start.SetWindowText(L"Start");
				flag = false;
				setGuideText("Set test cases, and press \"Start\" button.");
			}
			sleep(1);
		}
	}

	while (true) {
		if (m_event_queue.size() == 0) {
			break;
		}
		m_event_queue.pop();
	}

	return;
}

void CX4506AuthToolDlg::OnCbnSelchangeComboDeviceId()
{
	// TODO: Add your control notification handler code here
	int test_status;
	int idx;
	int device_id;
	CString tmp_str;
	std::string device_sub_id_str;

	test_status = m_controller.getTestStatus();

	if (test_status != CONTROLLER_TEST_FLAG_STOP) {
		return;
	}

	idx = m_combo_device_id.GetCurSel();
	if (idx < 0) {
		AfxMessageBox(L"select device type.");
		return;
	}
	device_id = m_combo_device_id.GetItemData(idx);

	if (m_controller.setDeviceId(device_id) < 0) {
		tracee("setDeviceId failed.");
		return;
	}

	m_list_test_list.DeleteAllItems();
	m_test_items.clear();
	m_test_items = m_controller.getTestItems();
	trace("let's create test progress: %d", m_test_items.size());
	for (unsigned int i = 0; i < m_test_items.size(); i++) {
		//trace("insert column: %s", progress.m_name.c_str());
		m_list_test_list.InsertItem(i, CString(m_test_items[i].m_name.c_str()));
		m_list_test_list.SetItemText(i, 1, CString(""));
		m_list_test_list.SetItemText(i, 2, CString(""));
	}
	OnBnClickedCheckTestTypeDevice();

	//set target device sub id & virtual device sub id
	switch (device_id) {
	case SYSTEMAIRCON_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x11");
		m_edit_device_port.SetWindowText(L"28890");
		//m_edit_vdevice_sub_id1.SetWindowText(L"0x0a");
		//m_edit_vdevice_sub_id2.SetWindowText(L"0xe1");
		//m_edit_vdevice_sub_id2.ShowWindow(TRUE);
		break;
	case BOILER_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x11");
		m_edit_device_port.SetWindowText(L"28886");
		break;
	case LIGHT_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x11");
		m_edit_device_port.SetWindowText(L"28881");
		break;
	case REMOTEINSPECTOR_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x11");
		m_edit_device_port.SetWindowText(L"28892");
		break;
	case TEMPERATURECONTROLLER_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x11");
		m_edit_device_port.SetWindowText(L"28887");
		break;
	case POWERGATE_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x11");
		m_edit_device_port.SetWindowText(L"28891");
		break;
	case BREAKER_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x01");
		m_edit_device_port.SetWindowText(L"28888");
		break;
	case GASVALVE_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x01");
		m_edit_device_port.SetWindowText(L"28884");
		//m_edit_vdevice_sub_id1.SetWindowText(L"0x0a");
		//m_edit_vdevice_sub_id2.SetWindowText(L"-1");
		//m_edit_vdevice_sub_id2.ShowWindow(FALSE);
		break;
	case CURTAIN_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x01");
		m_edit_device_port.SetWindowText(L"28885");
		break;
	case DOORLOCK_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x01");
		m_edit_device_port.SetWindowText(L"28882");
		break;
	case VANTILATOR_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x01");
		m_edit_device_port.SetWindowText(L"28883");
		break;
	case PREVENTCRIMEEXT_DEVICE_ID:
		//m_edit_device_sub_id.SetWindowText(L"0x01");
		m_edit_device_port.SetWindowText(L"28889");
		break;
	case MICROWAVEOVEN_DEVICE_ID:
	case DRUMWASHER_DEVICE_ID:
	case DISHWASHER_DEVICE_ID:
	case ZIGBEE_DEVICE_ID:
	case POWERMETER_DEVICE_ID:
	default:
		//m_edit_device_sub_id.SetWindowText(L"-1");
		//m_edit_vdevice_sub_id1.SetWindowText(L"-1");
		break;
	}

	m_edit_device_port.SetWindowText(L"28880");

}


void CX4506AuthToolDlg::OnBnClickedButtonStart()
{
	int test_status;
	CString tmp_str;
	std::string device_ip;
	int device_port;
	int item_count;
	bool is_check;

	test_status = m_controller.getTestStatus();

	if (test_status == CONTROLLER_TEST_FLAG_STOP) {
		m_edit_device_ip.GetWindowText(tmp_str);
		device_ip = std::string(CStringA(tmp_str));

		m_edit_device_port.GetWindowText(tmp_str);
		device_port = std::stoi(std::string(CStringA(tmp_str)));

		if (m_controller.setDeviceAddress(device_ip, device_port) < 0) {
			tracee("set devie address failed.");
			AfxMessageBox(L"Set Device address failed.");
			return;
		}

		//set test enable/disable
		item_count = m_list_test_list.GetItemCount();
		for (int i = 0; i < item_count; i++) {
			is_check = m_list_test_list.GetCheck(i);
			m_controller.setEnable(i, is_check);

			m_list_test_list.SetItemText(i, 1, CString(""));
			m_list_test_list.SetItemText(i, 2, CString(""));
		}

		//reset result
		for (unsigned int i = 0; i < m_test_items.size(); i++) {
			m_test_items[i].m_result = 0;
			m_test_items[i].m_description = std::string("");
		}
		//clear the result and description of the list
		for (unsigned int i = 0; i < m_test_items.size(); i++) {
			m_list_test_list.SetItemText(i, 1, CString(""));
			m_list_test_list.SetItemText(i, 2, CString(""));
		}

		//start
		if (m_controller.startTest() < 0) {
			AfxMessageBox(L"Start test failed.");
			return;
		}

		m_btn_start.SetWindowText(L"Stop");
	}
	else if (test_status == CONTROLLER_TEST_FLAG_RUN) {
		m_controller.stopTest();
	}

	return;
}


void CX4506AuthToolDlg::OnBnClickedButtonStop()
{
	// TODO: Add your control notification handler code here
}

void CX4506AuthToolDlg::testEventCallback(struct TestItem event)
{
	trace("device controller callback: %d, %s, %d, %s", event.m_id, event.m_name.c_str(), event.m_result, event.m_description.c_str());
	m_event_queue.push(event);
}

std::string CX4506AuthToolDlg::testProgress2String()
{
	std::string ret;

	ret = "";
	for (unsigned int i = 0; i < m_test_items.size(); i++) {
		ret += m_test_items[i].m_name + std::string("---------");
		if (m_test_items[i].m_result == 0) {
			ret += std::string("TESTING, ");
		}
		else if (m_test_items[i].m_result > 0) {
			ret += std::string("SUCCESS, ");
		}
		else {
			ret += std::string("FAULT, ");
		}
		ret += m_test_items[i].m_description;
		ret += std::string("\r\n");
	}

	return ret;
}


void CX4506AuthToolDlg::OnBnClickedCheckTestTypeDevice()
{
	// TODO: Add your control notification handler code here
	int check;
	
	check = m_checkbox_device_test.GetCheck();

	trace("test device clicked: %d", check);
	for (unsigned int i = 0; i < m_test_items.size(); i++) {
		if (m_test_items[i].m_test_type == TEST_TYPE_DEVICE) {
			m_list_test_list.SetCheck(i, check);
		}
	}
}

/*
void CX4506AuthToolDlg::OnBnClickedCheckTestTypeController()
{
	// TODO: Add your control notification handler code here
	int check;
	
	check = m_checkbox_controller_test.GetCheck();

	trace("test controller clicked: %d", check);
	for (unsigned int i = 0; i < m_test_items.size(); i++) {
		if (m_test_items[i].m_test_type == TEST_TYPE_CONTROLLER) {
			m_list_test_list.SetCheck(i, check);
		}
	}
}
*/

CString g_log_filename;
void CX4506AuthToolDlg::OnBnClickedButtonShowLog()
{
	// TODO: Add your control notification handler code here
	if (g_log_filename.GetLength() == 0) {
		printf("no log...\n");
		return;
	}
	
	g_log_filename.Replace(L"/", L"\\\\");
	ShellExecuteW(0, 0, (LPCWSTR)g_log_filename, 0, 0, SW_SHOWNORMAL);
}
