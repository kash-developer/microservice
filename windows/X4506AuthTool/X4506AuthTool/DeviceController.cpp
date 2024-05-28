
#include "stdafx.h"
#include "DeviceController.h"

#include <serial_device.h>
#include <home_device_lib.h>
#include <trace.h>
#include <tools.h>

#include <controller_device.h>
#include <forwarder_lib.h>
#include <win_porting.h>

#include "X4506AuthToolDlg.h"
#include "OpenApiTestLight.h"

#include <json/json.h>

#include <Windows.h>

#include <light_device.h>
#include <doorlock_device.h>
#include <vantilator_device.h>
#include <gasvalve_device.h>
#include <curtain_device.h>
#include <boiler_device.h>
#include <temperature_controller_device.h>
#include <breaker_device.h>
#include <prevent_crime_ext_device.h>
#include <system_aircon_device.h>
#include <powergate_device.h>
#include <remote_inspector_device.h>

#include <json/json.h>

#include <stdio.h>
#include <string>
#include <sys/stat.h>


#include <iostream>

#ifdef WIN32
#else
#include <unistd.h>
#endif

HomeDevice* createHomeDevice(std::string filename);

#define MAX_DEVICE_LENGTH	13
static struct DeviceSupportList g_device_list[] = {
	{ "SystemAircon", SYSTEMAIRCON_DEVICE_ID, "system_aircon.json" },
	{ "Light", LIGHT_DEVICE_ID, "light.json" },
	{ "GasValve", GASVALVE_DEVICE_ID, "gasvalve.json" },
	{ "Curtain", CURTAIN_DEVICE_ID, "curtain.json" },
	{ "RemoteInspector", REMOTEINSPECTOR_DEVICE_ID, "remote_inspector.json" },
	{ "DoorLock", DOORLOCK_DEVICE_ID, "doorlock.json" },
	{ "Vantilator", VANTILATOR_DEVICE_ID, "vantilator.json" },
	{ "Breaker", BREAKER_DEVICE_ID, "breaker.json" },
	{ "Boiler", BOILER_DEVICE_ID, "boiler.json" },
	{ "TemperatureController", TEMPERATURECONTROLLER_DEVICE_ID, "temperature_controller.json" },
	{ "PowerGate", POWERGATE_DEVICE_ID, "powergate.json" },
	{ "PreventCrimeExt", PREVENTCRIMEEXT_DEVICE_ID, "prevent_crime_ext.json" },
	{ "Phone", PHONE_DEVICE_ID, "phone.json" }
};

const std::vector<struct DeviceSupportList> DeviceController::getDeviceSupport()
{
	std::vector<struct DeviceSupportList> device_list;
	struct DeviceSupportList device;

	for (int i = 0; i < MAX_DEVICE_LENGTH; i++) {
		device = g_device_list[i];
		device_list.push_back(device);
	}

	return device_list;
}

DeviceController::DeviceController()
{
}


DeviceController::~DeviceController()
{
	stop();
	
	if (m_controller != NULL) {
		delete m_controller;
		m_controller = NULL;
	}

	if (m_test != NULL) {
		delete m_test;
		m_test = NULL;
	}
}

int DeviceController::init(CX4506AuthToolDlg* parent)
{
	std::string filename;
	std::string base_dir;
	std::string err;
	Json::Value conf_json;
	std::vector<int> sub_ids;

	traceSetLevel(99);

	m_parent = parent;
	m_test = NULL;
	m_test_flag = CONTROLLER_TEST_FLAG_STOP;
	base_dir = ORG_DATA_HOME_PATH;
	m_target_device_id = -1;

	//run controller
	filename = base_dir + "/controller.json";
	if (parseJson(filename, &conf_json, &err) == false) {
		tracee("parse conf file failed: %s", filename.c_str());
		tracee("%s", err.c_str());
		AfxMessageBox(L"controller init failed.");
		return -1;
	}
	m_controller = new ControllerDevice();
	sub_ids.push_back(0x01);
	if (m_controller->init(conf_json, sub_ids, CONTROLLER_HTTP_PORT1) < 0) {
		tracee("controller init failed: %s", filename.c_str());
		AfxMessageBox(L"controller init failed.");
		return -1;
	}
	m_controller->setDataHomePath(base_dir);

	return 0;
}

int DeviceController::run()
{
	if (m_controller->run() < 0) {
		tracee("run controller failed.");
		//m_forwarder->stop();
		return -1;
	}

	return 0;
}

void DeviceController::stop()
{
	trace("let's stop device controller");
	stopTest();
	trace("after stopTest");

	m_controller->stop();
	trace("device controller stopped.");
}

int DeviceController::getTestStatus()
{
	return m_test_flag;
}

int DeviceController::setDeviceId(int device_id)
{
	std::string err;
	Json::Value conf_json;
	std::vector<struct TestItem> test_items;

	trace("set device id: 0x%02x", device_id);

	m_base_dir = ORG_DATA_HOME_PATH;

	if (m_test_flag != CONTROLLER_TEST_FLAG_STOP) {
		tracee("test is not stopped.");
		AfxMessageBox(L"Test is not stopped.");
		return -1;
	}

	if (m_target_device_id == device_id) {
		return 0;
	}

	if (m_test != NULL) {
		delete m_test;
		m_test = NULL;
	}

	m_target_device_id = device_id;

	//create device & test_device
	switch (m_target_device_id) {
	case SYSTEMAIRCON_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case LIGHT_DEVICE_ID:
		//m_test = new OpenApiTestLight();
		m_test = new OpenApiTestBase();
		break;
	case GASVALVE_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case CURTAIN_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case REMOTEINSPECTOR_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case DOORLOCK_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case VANTILATOR_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case BREAKER_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case PREVENTCRIMEEXT_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case BOILER_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case TEMPERATURECONTROLLER_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case POWERGATE_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case PHONE_DEVICE_ID:
		m_test = new OpenApiTestBase();
		break;
	case MICROWAVEOVEN_DEVICE_ID:
	case DISHWASHER_DEVICE_ID:
	case DRUMWASHER_DEVICE_ID:
	case POWERMETER_DEVICE_ID:
	case ZIGBEE_DEVICE_ID:
	default:
		tracee("invalid device id: %x", m_target_device_id);
		AfxMessageBox(L"invalid target device id.");
		return -1;
	}

	//initialize test device
	if (m_test->init(this, m_controller, NULL, NULL) < 0) {
		delete m_test;
		m_test = NULL;

		tracee("test device init failed.");
		AfxMessageBox(L"test device init failed.");

		return -1;
	}

	return 0;
}

int DeviceController::setDeviceAddress(std::string addr, int port)
{
	m_target_device_ip = addr;
	m_target_device_port = port;

	m_test->setDeviceAddress(m_target_device_ip, m_target_device_port);
	return 0;
}

int DeviceController::setEnable(int idx, bool is_enable)
{
	return m_test->setEnable(idx, is_enable);
}

int DeviceController::startTest()
{
	struct TestItem event;

	if (m_test_flag != CONTROLLER_TEST_FLAG_STOP) {
		tracee("test is running...");
		AfxMessageBox(L"Test is running.");
		return -1;
	}


	m_test_flag = CONTROLLER_TEST_FLAG_RUN;
	trace("start test: %x", m_target_device_id);

	m_test->setDeviceId(m_target_device_id, -1, -1, -1);

	if (m_test->run() < 0) {
			tracee("start test message failed.");
			AfxMessageBox(L"start test message failed.");
			m_test_flag = CONTROLLER_TEST_FLAG_STOP;

			return -1;
	}

	return 0;
}

void DeviceController::stopTest()
{
	if (m_test_flag == CONTROLLER_TEST_FLAG_STOP) {
		trace("test already stopped: 0x%02x", m_target_device_id);
		return;
	}

	m_test_flag = CONTROLLER_TEST_FLAG_STOPPING;

	if (m_test != NULL) {
		m_test->stop();
	}

	m_test_flag = CONTROLLER_TEST_FLAG_STOP;

	return;
}

void DeviceController::testEventCallback(struct TestItem event)
{
	trace("call callback: %d, %s, %d, %s", event.m_id, event.m_name.c_str(), event.m_result, event.m_description.c_str());
	m_parent->testEventCallback(event);
	if (event.m_id == TEST_EVENT_COMPLETE) {
		/*
		m_device1->stop();
		if (m_virtual_device_sub_id2 != -1) {
			m_device2->stop();
		}
		*/
		m_test_flag = CONTROLLER_TEST_FLAG_STOP;
	}
}

std::vector<struct TestItem> DeviceController::getTestItems()
{
	return m_test->getTestItems();
}


void DeviceController::setGuideText(std::string str)
{
	m_parent->setGuideText(str);
}

