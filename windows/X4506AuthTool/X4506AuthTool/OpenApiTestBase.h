#pragma once

#include <stdio.h>
#include <thread>
#include <vector>
#include <string>

#include <controller_device.h>
#include <home_device_lib.h>
#include <TestBase.h>

#define TEST_LOG_DIR "../Debug/logs/"

#define TEST_REQUEST_ALL		0
#define TEST_REQUEST_DEVICE		1
#define TEST_REQUEST_GROUP		2
#define TEST_REQUEST_INDIVIDUAL	3

class DeviceController;
class ControllerDevice;
class HomeDevice;

class OpenApiTestCommon;
class OpenApiTestLight;

class OpenApiTestBase
{
protected:
	int m_device_id;
	//int m_device_sub_id;
	std::vector<int> m_device_sub_ids;
	std::vector<int> m_device_groups;
	//int m_sub_device_number;
	std::string m_device_ip;
	int m_device_port;
	//int m_virtual_device_sub_id1;
	//int m_virtual_device_sub_id2;

	std::string m_base_url;
	std::string m_log_filename;

	bool m_run_flag;
	std::thread* m_tid;
	DeviceController* m_parent;
	ControllerDevice* m_controller;
	//HomeDevice* m_device1;
	//HomeDevice* m_device2;

	std::vector<struct TestItem> m_test_items;

	Json::Value m_test_control_cmds;
	Json::Value m_control_info;

	FILE* m_log_fp;

protected:
	int getDeviceInfo();

	FILE* createLogFile(int dev_id, int sub_id);
	int writeLog(std::string log_str);

	Json::Value sendPostRequest(std::string url, Json::Value body);
	Json::Value sendGetRequest(std::string url);
	bool checkCharacteristic(int type, Json::Value jobj);
	bool checkStatus(int type, Json::Value jobj);
	bool checkControl(int type, Json::Value jobj_req, Json::Value jobj_res, int device_id = 0xff, int sub_id = 0xff);
	bool checkControlParameters(std::vector<struct OpenApiCommandInfo>& req_controls, 
								std::vector<struct OpenApiCommandInfo>& res_controls);

	int test_base_1(struct TestItem* item);
	int test_base_2(struct TestItem* item);
	int test_base_3(struct TestItem* item);
	int test_base_4(struct TestItem* item);
	int test_base_5(struct TestItem* item);
	int test_base_6(struct TestItem* item);
	int test_base_7(struct TestItem* item);
	int test_base_8(struct TestItem* item);
	int test_base_9(struct TestItem* item);
	int test_base_10(struct TestItem* item);
	int test_base_11(struct TestItem* item);
	int test_base_12(struct TestItem* item);

public:
	OpenApiTestBase();
	~OpenApiTestBase();

	virtual int init(DeviceController* parent, ControllerDevice* controller, HomeDevice* device1, HomeDevice* device2);
	int run();
	void stop();

	void addTestItem(struct TestItem item);

	void testThread();
	void waitTest();

	std::vector<struct TestItem> getTestItems();
	void setDeviceId(int device_id, int device_sub_id, int virtual_device_sub_id1, int virtual_device_sub_id2);
	void setDeviceAddress(std::string addr, int port);
	int setEnable(int idx, bool is_enable);

	virtual int deviceTestFunctions(struct TestItem* item);
	virtual bool recvBeforeHttpCallback(HomeDevice* device, HttpRequest* request, HttpResponse* response);
	virtual bool recvAfterHttpCallback(HomeDevice* device, HttpRequest* request, HttpResponse* response);
	virtual bool recvBeforeHttpuCallback(HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);
	virtual bool recvAfterHttpuCallback(HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);

	static struct TestItem* getTestItem_label(std::vector<struct TestItem*> items, std::string label);

	int getDeviceStatus(int device_id, int device_sub_id, Json::Value* jobj);


};

