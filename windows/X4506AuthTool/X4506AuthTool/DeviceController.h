#pragma once

#include <controller_device.h>
#include <OpenApiTestBase.h>

#include <map>
#include <vector>
#include <thread>

#define ORG_DATA_HOME_PATH	"../data"

#define CONTROLLER_TEST_FLAG_RUN		0
#define CONTROLLER_TEST_FLAG_STOP		1
#define CONTROLLER_TEST_FLAG_STOPPING	2

#define CONTROLLER_HTTP_PORT1		65000
#define HOME_DEVICE_HTTP_PORT1		65001
#define HOME_DEVICE_HTTP_PORT2		65002

class CX4506AuthToolDlg;
class OpenApiTestBase;
class X4506TestMessage;

struct DeviceSupportList {
	std::string m_type;
	int m_device_id;
	std::string m_conf;
};

class DeviceController
{
private:
	CX4506AuthToolDlg* m_parent;
	ControllerDevice* m_controller;

	std::string m_base_dir;
	std::string m_conf_filename;
	int m_target_device_id;

	std::string m_target_device_ip;
	int m_target_device_port;
	int m_test_flag;
	OpenApiTestBase* m_test;

public:
	DeviceController();
	~DeviceController();

	static const std::vector<struct DeviceSupportList> DeviceController::getDeviceSupport();

	int init(CX4506AuthToolDlg* parent);
	int run();
	void stop();
	int getTestStatus();
	int setDeviceId(int device_id);
	int setDeviceAddress(std::string addr, int port);
	int setEnable(int idx, bool is_enable);

	int startTest();
	void stopTest();

	std::vector<struct TestItem> getTestItems();
	void testEventCallback(struct TestItem item);
	void setGuideText(std::string str);

};

