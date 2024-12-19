#include "stdafx.h"
#include "OpenApiTestBase.h"

#include "DeviceController.h"
#include "OpenApiTestLight.h"

#include <trace.h>
#include <tools.h>
#include <win_porting.h>
#include <controller_device.h>
#include <home_device_lib.h>
#include <http_request.h>
#include <http_response.h>

#include <thread>

#include <fstream>
#include <iostream>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

using nlohmann::json;
using nlohmann::json_uri;
using nlohmann::json_schema::json_validator;

/*
bool openapi_base_before_http_callback(void* instance, HomeDevice* device, HttpRequest* request, HttpResponse* response)
{
	OpenApiTestBase* test;
	test = (OpenApiTestBase*)instance;
	return test->recvBeforeHttpCallback(device, request, response);
}

bool openapi_base_after_http_callback(void* instance, HomeDevice* device, HttpRequest* request, HttpResponse* response)
{
	OpenApiTestBase* test;
	test = (OpenApiTestBase*)instance;
	return test->recvAfterHttpCallback(device, request, response);
}

bool openapi_base_before_httpu_callback(void* instance, HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	OpenApiTestBase* test;
	test = (OpenApiTestBase*)instance;
	return test->recvBeforeHttpuCallback(device, message, sock_info, from_addr);
}

bool openapi_base_after_httpu_callback(void* instance, HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	OpenApiTestBase* test;
	test = (OpenApiTestBase*)instance;
	return test->recvAfterHttpuCallback(device, message, sock_info, from_addr);
}
*/

OpenApiTestBase::OpenApiTestBase()
{
}


OpenApiTestBase::~OpenApiTestBase()
{
}


int OpenApiTestBase::init(DeviceController* parent, ControllerDevice* controller, HomeDevice* device1, HomeDevice* device2)
{
	struct TestItem test_item;

	m_tid = NULL;

	m_parent = parent;
	m_controller = controller;

	test_item.m_label = "_base_1_";
	test_item.m_name = std::string("1. 전체 명세 조회");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_2_";
	test_item.m_name = std::string("2. 장치 명세 조회");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_3_";
	test_item.m_name = std::string("3. 그룹 명세 조회");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_4_";
	test_item.m_name = std::string("4. 개별 명세 조회");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_5_";
	test_item.m_name = std::string("5. 전체 상태 조회");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_6_";
	test_item.m_name = std::string("6. 장치 상태 조회");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_7_";
	test_item.m_name = std::string("7. 그룹 상태 조회");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_8_";
	test_item.m_name = std::string("8. 개별 상태 조회");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_9_";
	test_item.m_name = std::string("9. 전체 제어");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_10_";
	test_item.m_name = std::string("10. 장치 제어");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_11_";
	test_item.m_name = std::string("11. 그룹 제어");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	test_item.m_label = "_base_12_";
	test_item.m_name = std::string("12. 개별 제어");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	addTestItem(test_item);

	return 0;
}

void OpenApiTestBase::setDeviceId(int device_id, int device_sub_id, int virtual_device_sub_id1, int virtual_device_sub_id2)
{
	std::string err_str;
	std::string device_name;
	std::string test_filepath;
	std::string control_info_filepath;
	Json::Value jobj_tmp;

	m_device_id = device_id;
	m_log_fp = createLogFile(m_device_id, -1);

	device_name = HomeDevice::getDeviceNameString(m_device_id);
	test_filepath = ORG_DATA_HOME_PATH;
	switch (m_device_id) {
	case SYSTEMAIRCON_DEVICE_ID:
		test_filepath += std::string("/control_test_system_aircon.json");
		break;
	case LIGHT_DEVICE_ID:
		test_filepath += std::string("/control_test_light.json");
		break;
	case GASVALVE_DEVICE_ID:
		test_filepath += std::string("/control_test_gasvalve.json");
		break;
	case CURTAIN_DEVICE_ID:
		test_filepath += std::string("/control_test_curtain.json");
		break;
	case REMOTEINSPECTOR_DEVICE_ID:
		test_filepath += std::string("/control_test_remote_inspector.json");
		break;
	case DOORLOCK_DEVICE_ID:
		test_filepath += std::string("/control_test_doorlock.json");
		break;
	case VANTILATOR_DEVICE_ID:
		test_filepath += std::string("/control_test_vantilator.json");
		break;
	case BREAKER_DEVICE_ID:
		test_filepath += std::string("/control_test_breaker.json");
		break;
	case PREVENTCRIMEEXT_DEVICE_ID:
		test_filepath += std::string("/control_test_prevent_crime_ext.json");
		break;
	case BOILER_DEVICE_ID:
		test_filepath += std::string("/control_test_boiler.json");
		break;
	case TEMPERATURECONTROLLER_DEVICE_ID:
		test_filepath += std::string("/control_test_temperature_controller.json");
		break;
	case POWERGATE_DEVICE_ID:
		test_filepath += std::string("/control_test_powergate.json");
		break;
	case PHONE_DEVICE_ID:
		test_filepath += std::string("/control_test_phone.json");
		break;
	case ENTRANCE_DEVICE_ID:
		test_filepath += std::string("/control_test_entrance.json");
		break;
	case MICROWAVEOVEN_DEVICE_ID:
	case DISHWASHER_DEVICE_ID:
	case DRUMWASHER_DEVICE_ID:
	case ZIGBEE_DEVICE_ID:
	case POWERMETER_DEVICE_ID:
	default:
		tracee("invalid device ID: %x", m_device_id);
		return;
	}
	
	control_info_filepath = ORG_DATA_HOME_PATH + std::string("/control_info.json");
	if (parseJson(control_info_filepath, &jobj_tmp, &err_str) == false) {
		tracee("parse control info. failed: %s", err_str.c_str());
		m_control_info = Json::Value();
	}
	else {
		trace("tmp_obj: %s", jobj_tmp.toStyledString().c_str());
		if (jobj_tmp.isMember(device_name) == false) {
			tracee("there is no field for: %s", device_name.c_str());
			m_control_info = Json::Value();
		}
		else {
			m_control_info = jobj_tmp[device_name];
		}
	}

	if (parseJson(test_filepath, &m_test_control_cmds, &err_str) == false) {
		tracee("parse control info. failed: %s, %s", test_filepath.c_str(), err_str.c_str());
		m_test_control_cmds = Json::Value();
		m_test_control_cmds["individual"] = Json::Value();
		m_test_control_cmds["group"] = Json::Value();
		m_test_control_cmds["all"] = Json::Value();
	}

	return;
}

void OpenApiTestBase::setDeviceAddress(std::string addr, int port)
{
	m_device_ip = addr;
	m_device_port = port;
	m_base_url = std::string("http://") + m_device_ip + std::string(":") + std::to_string(m_device_port);
	m_base_url += std::string("/smarthome/v1");
}

int OpenApiTestBase::run()
{
	m_run_flag = true;
	m_tid = new std::thread([=] { this->testThread(); });

	return 0;
}

void OpenApiTestBase::stop()
{
	trace("let's stop test.");
	m_run_flag = false;
	if (m_tid != NULL) {
		m_tid->join();
		delete m_tid;
		m_tid = NULL;
	}
}

void OpenApiTestBase::addTestItem(struct TestItem item)
{
	item.m_id = m_test_items.size();
	m_test_items.push_back(item);
}

void OpenApiTestBase::testThread()
{
	struct TestItem complete_event;

	m_parent->setGuideText("Testing device test...");
	tracee("it's test_device: %d", m_test_items.size());

	if (getDeviceInfo() < 0) {
		writeLog("getDeviceInfo failed.");
		writeLog("test exit.");
		AfxMessageBox(L"Get device information failed. Test exit.");
	}
	else {
		for (unsigned int i = 0; i < m_test_items.size(); i++) {
			if (m_run_flag == false) {
				break;
			}
			m_test_items[i].m_result = TEST_RESULT_NOT_TESTED;
			m_test_items[i].m_description = std::string("SKIPPED");

			if (m_test_items[i].m_enable == false) {
				m_test_items[i].m_result = TEST_RESULT_SKIPPED;
				m_test_items[i].m_description = std::string("SKIPPED");
			}
			else if ((m_test_items[i].m_enable == true) && (m_test_items[i].m_test_type == TEST_TYPE_DEVICE)) {
				trace("\n");
				trace("test: %s", m_test_items[i].m_name.c_str());
				deviceTestFunctions(&(m_test_items[i]));
			}
			if ((m_parent != NULL) && (m_test_items[i].m_result != TEST_RESULT_NOT_TESTED)) {
				m_parent->testEventCallback(m_test_items[i]);
			}
			//sleep(1);
		}
	}

	if (m_run_flag == true) {
		m_parent->setGuideText("Testing device... Send messages for the message format test.");
		waitTest();
	}
	trace("end of test_device.");

	//send complete event
	if (m_parent != NULL) {
		complete_event.m_id = TEST_EVENT_COMPLETE;
		complete_event.m_label = "";
		complete_event.m_name = "";
		complete_event.m_test_type = TEST_EVENT_COMPLETE;
		complete_event.m_enable = true;
		complete_event.m_result = TEST_EVENT_COMPLETE;
		complete_event.m_description = "";

		m_parent->testEventCallback(complete_event);
	}

	if (m_log_fp != NULL) {
		fclose(m_log_fp);
	}
	m_log_fp = NULL;

	m_run_flag = false;

	return;
}

void OpenApiTestBase::waitTest()
{
	bool finished_flag;
	std::vector<struct TestItem> old_items;

	old_items = m_test_items;

	while (true) {
		if (m_run_flag == false) {
			break;
		}

		finished_flag = true;
		for (unsigned int i = 0; i < m_test_items.size(); i++) {
			if ((m_parent != NULL) && (m_test_items[i].m_result != TEST_RESULT_NOT_TESTED)) {
				if ((m_test_items[i].m_result != old_items[i].m_result) ||
					(m_test_items[i].m_description.compare(old_items[i].m_description) != 0)) {
					old_items[i] = m_test_items[i];
					m_parent->testEventCallback(m_test_items[i]);
					trace("test completed: %d, %s", i, m_test_items[i].m_name.c_str());
				}
			}
		}
		for (unsigned int i = 0; i < m_test_items.size(); i++) {
			if ((m_test_items[i].m_enable == true) && (m_test_items[i].m_result == TEST_RESULT_NOT_TESTED)) {
				//trace("not tested: %d, %s", i, m_test_items[i].m_description.c_str());
				finished_flag = false;
				break;
			}
		}

		if (finished_flag == true) {
			break;
		}

		sleep(1);
	}

	return;
}

int OpenApiTestBase::deviceTestFunctions(struct TestItem* item)
{
	int ret;
	std::string log;

	ret = TEST_RESULT_NOT_TESTED;

	log = std::string("Test Name: ") + item->m_name + std::string("\n");

	if (item->m_label.compare("_base_1_") == 0) {
		writeLog(log);
		ret = test_base_1(item);
	}
	else if (item->m_label.compare("_base_2_") == 0) {
		writeLog(log);
		ret = test_base_2(item);
	}
	else if (item->m_label.compare("_base_3_") == 0) {
		writeLog(log);
		ret = test_base_3(item);
	}
	else if (item->m_label.compare("_base_4_") == 0) {
		writeLog(log);
		ret = test_base_4(item);
	}
	else if (item->m_label.compare("_base_5_") == 0) {
		writeLog(log);
		ret = test_base_5(item);
	}
	else if (item->m_label.compare("_base_6_") == 0) {
		writeLog(log);
		ret = test_base_6(item);
	}
	else if (item->m_label.compare("_base_7_") == 0) {
		writeLog(log);
		ret = test_base_7(item);
	}
	else if (item->m_label.compare("_base_8_") == 0) {
		writeLog(log);
		ret = test_base_8(item);
	}
	else if (item->m_label.compare("_base_9_") == 0) {
		writeLog(log);
		ret = test_base_9(item);
	}
	else if (item->m_label.compare("_base_10_") == 0) {
		writeLog(log);
		ret = test_base_10(item);
	}
	else if (item->m_label.compare("_base_11_") == 0) {
		writeLog(log);
		ret = test_base_11(item);
	}
	else if (item->m_label.compare("_base_12_") == 0) {
		writeLog(log);
		ret = test_base_12(item);
	}
	else {
		log = std::string("undefined test: ") + item->m_label;
		writeLog(log);
	}

	return ret;
}

bool OpenApiTestBase::recvBeforeHttpCallback(HomeDevice* device, HttpRequest* request, HttpResponse* response)
{
	return true;
}

bool OpenApiTestBase::recvAfterHttpCallback(HomeDevice* device, HttpRequest* request, HttpResponse* response)
{
	return true;
}

bool OpenApiTestBase::recvBeforeHttpuCallback(HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	return true;
}

bool OpenApiTestBase::recvAfterHttpuCallback(HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	return true;
}

std::vector<struct TestItem> OpenApiTestBase::getTestItems()
{
	return m_test_items;
}

int OpenApiTestBase::setEnable(int idx, bool is_enable)
{
	if (idx >= (int)m_test_items.size()) {
		tracee("invalid idx: %d / %d", idx, m_test_items.size());
		return -1;
	}
	m_test_items[idx].m_enable = is_enable;

	return 0;
}

struct TestItem* OpenApiTestBase::getTestItem_label(std::vector<struct TestItem*> items, std::string label)
{
	struct TestItem* ret;

	ret = NULL;
	for (unsigned int i = 0; i < items.size(); i++) {
		//trace("compare: %s - %s", label.c_str(), items[i]->m_label.c_str());
		if (items[i]->m_label.compare(label) == 0) {
			ret = items[i];
			break;
		}
	}

	trace("getTestItem_label: %s: %x", label.c_str(), ret);
	return ret;
}

int OpenApiTestBase::getDeviceStatus(int device_id, int device_sub_id, Json::Value* jobj)
{
	Json::Value ret;

	ret = m_controller->getDeviceStatus(device_id, device_sub_id);

	if (ret["errorCode"].asInt() < 0) {
		switch (ret["errorCode"].asInt()) {
		case ERROR_CODE_SEND_FAILED:
			tracee("send get status command failed");
			break;
		case ERROR_CODE_NO_RESPONSE:
			tracee("no response");
			break;
		case ERROR_CODE_INVALID_MESSAGE:
			tracee("invalid response message");
			break;
		default:
			tracee("light returns unknown error: %d", ret["errorCode"].asInt());
		}
		return -1;
	}

	*jobj = ret;
	trace("device status: 0x%02x, 0x%02x", device_id, device_sub_id);
	trace("\n%s", jobj->toStyledString().c_str());

	return 0;
}

#include <ctime>
extern CString g_log_filename;
FILE* OpenApiTestBase::createLogFile(int dev_id, int sub_id)
{
	FILE* fp;
	CString filename;
	CString time_str;
	errno_t err;

	filename.Format(L"0x%02x_", dev_id);
	time_str = COleDateTime::GetCurrentTime().Format(L"%y%m%d_%H%M%S");
	filename = CString(TEST_LOG_DIR) + filename + time_str + CString(".log");
	//filename = CString("./") + filename + time_str + CString(".log");

	err = fopen_s(&fp, CStringA(filename), "w");

	g_log_filename = filename;

	return fp;
}

int OpenApiTestBase::writeLog(std::string log_str)
{
	CString time_str;
	std::string ll;

	if (m_log_fp == NULL) {
		return 0;
	}
	time_str = COleDateTime::GetCurrentTime().Format(L"%H:%M:%S");
	ll = std::string("[") + std::string(CStringA(time_str)) + std::string("] ");
	ll += log_str + std::string("\r\n");

	fwrite(ll.c_str(), 1, ll.length(), m_log_fp);

	return 0;
}

Json::Value OpenApiTestBase::sendPutRequest(std::string url, Json::Value body)
{
	HttpRequest* req;
	HttpResponse* res;
	uint8_t* tmp_body;
	long len;
	std::string mime_type;
	Json::Value jobj_body;
	std::string err;
	char* str;

	req = new HttpRequest();
	req->setMethod("PUT");
	req->setUrl(url);
	req->setBody((uint8_t*)body.toStyledString().c_str(), body.toStyledString().length(), "appication/json");
	res = req->sendRequest();

	writeLog("Send Request---------");
	req->serialize((uint8_t**)&str);
	writeLog(str);
	delete[] str;

	if (res == NULL) {
		delete req;
		writeLog("Send Request Failed");
		tracee("send request failed.");
		return Json::Value::null;
	}
	if (res->getResponseCode(NULL, NULL) != 200) {
		delete req;
		writeLog("Send Request Failed");
		tracee("send request failed.");
		return Json::Value::null;
	}

	writeLog("Received Response---------");
	res->serialize((uint8_t**)&str);
	writeLog(str);
	delete[] str;

	len = res->getBody(&tmp_body, &mime_type);

	if (len <= 0) {
		delete req;
		delete res;
		tracee("there is no response body.");
		return Json::Value::null;
	}
	else if (parseJson((const char*)tmp_body, (const char*)(tmp_body + len), &jobj_body, &err) == false) {
		delete req;
		delete res;
		tracee("parse body failed: %s\n%s", err.c_str(), (char*)tmp_body);
		return Json::Value::null;
	}

	delete req;
	delete res;

	return jobj_body;
}


Json::Value OpenApiTestBase::sendGetRequest(std::string url)
{
	HttpRequest* req;
	HttpResponse* res;
	uint8_t* tmp_body;
	long len;
	std::string mime_type;
	Json::Value jobj_body;
	std::string err;
	char* str;

	req = new HttpRequest();
	req->setMethod("GET");
	req->setUrl(url);
	res = req->sendRequest();

	writeLog("Send Request---------");
	req->serialize((uint8_t**)&str);
	writeLog(str);
	delete[] str;

	if (res == NULL) {
		delete req;
		writeLog("Send Request Failed");
		tracee("send request failed.");
		return Json::Value::null;
	}

	writeLog("Received Response---------");
	res->serialize((uint8_t**)&str);
	writeLog(str);
	delete[] str;

	len = res->getBody(&tmp_body, &mime_type);

	if (parseJson((const char*)tmp_body, (const char*)(tmp_body + len), &jobj_body, &err) == false) {
		delete req;
		delete res;
		tracee("parse body failed: %s\n", err.c_str());
		if (tmp_body == NULL) {
			tracee("no body.");
		}
		else {
			tracee("\n%s", (char*)tmp_body);
		}
		return Json::Value::null;
	}

	delete req;
	delete res;

	return jobj_body;
}

int OpenApiTestBase::getDeviceInfo()
{
	Json::Value jobj_tmp, jobj_tmp2;
	Json::Value jobj_status;
	std::string url;
	std::string tmp_str;
	int sub_id, group_id;
	bool is_found;

	writeLog("get the Information of sub devices---------");
	
	m_device_sub_ids.clear();
	m_device_groups.clear();

	url = m_base_url + std::string("/status/" + int2hex_str(m_device_id));
	jobj_status = sendGetRequest(url);

	if (jobj_status == Json::Value::null) {
		tracee("send GET request failed: %s", url.c_str());
		writeLog("send GET request failed.");
		return -1;
	}

	if (jobj_status.isMember("SubDeviceList") == false) {
		tracee("invalid status: %s", jobj_status.toStyledString().c_str());
		writeLog("invalid status.");
		return -1;
	}

	jobj_tmp = jobj_status["SubDeviceList"];
	if (jobj_tmp.isArray() == false) {
		tracee("invalid status: %", jobj_status.toStyledString().c_str());
		writeLog("invalid status.");
		return -1;
	}

	for (unsigned int i = 0; i < jobj_tmp.size(); i++) {
		jobj_tmp2 = jobj_tmp[i];
		if (jobj_tmp2.isMember("SubDeviceID") == false) {
			tracee("invalid SubDevice: %s", jobj_tmp2.toStyledString().c_str());
			continue;
		}
		sub_id = hex_str2int(jobj_tmp2["SubDeviceID"].asString());
		m_device_sub_ids.push_back(sub_id);
	}

	for (unsigned int i = 0; i < m_device_sub_ids.size(); i++) {
		is_found = false;
		group_id = m_device_sub_ids[i] | 0x0f;
		for (unsigned int j = 0; j < m_device_groups.size(); j++) {
			if (group_id == m_device_groups[j]) {
				is_found = true;
				break;
			}
		}
		if (is_found == false) {
			m_device_groups.push_back(group_id);
		}
	}

	tmp_str = "Sub-Devices: ";
	for (unsigned int i = 0; i < m_device_sub_ids.size(); i++) {
		tmp_str += int2hex_str(m_device_sub_ids[i]) + std::string(", ");
	}
	writeLog(tmp_str);
	tmp_str = "Sub-Groups: ";
	for (unsigned int i = 0; i < m_device_groups.size(); i++) {
		tmp_str += int2hex_str(m_device_groups[i]) + std::string(", ");
	}
	writeLog(tmp_str);
	writeLog("end of getting the number of sub devices---------");

	return 0;
}

static void loader(const json_uri &uri, json &schema)
{
	std::string filename = uri.path();
	std::ifstream lf(filename);
	if (!lf.good())
		throw std::invalid_argument("could not open " + uri.url() + " tried with " + filename);
	try {
		lf >> schema;
	}
	catch (const std::exception &e) {
		throw e;
	}
}

class custom_error_handler : public nlohmann::json_schema::basic_error_handler
{
	std::ostringstream oss;

	void error(const nlohmann::json::json_pointer &ptr, const json &instance, const std::string &message) override
	{
		nlohmann::json_schema::basic_error_handler::error(ptr, instance, message);
		oss << "ERROR: '" << ptr << "' - '" << instance << "': " << message << "\n";
		tracee("%s", oss.str().c_str());
	}
};

bool OpenApiTestBase::checkCharacteristic(int type, Json::Value jobj)
{
	std::string schema_path;

	json schema;
	json document;
	json_validator validator(loader, nlohmann::json_schema::default_string_format_check);
	custom_error_handler err;
	std::istringstream iss;
	std::ostringstream oss;

	switch (type) {
	case TEST_REQUEST_ALL:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_char_all.json");
		break;
	case TEST_REQUEST_DEVICE:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_char_device.json");
		break;
	case TEST_REQUEST_GROUP:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_char_group.json");
		break;
	case TEST_REQUEST_INDIVIDUAL:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_char_individual.json");
		break;
	default:
		tracee("invalid characteristic type: %d", type);
		writeLog("invalid characteristic type");
		return false;
	}

	std::ifstream f(schema_path);
	if (!f.good()) {
		tracee("cound not open schema file: %s", schema_path.c_str());
		return false;
	}
	try {
		f >> schema;
	}
	catch (const std::exception &e) {
		oss << e.what() << " at " << f.tellg() << " - while parsing the schema\n";
		tracee("%s", oss.str().c_str());
		return false;
	}

	try {
		validator.set_root_schema(schema);
	}
	catch (const std::exception &e) {
		std::cerr << "setting root schema failed\n";
		std::cerr << e.what() << "\n";
	}


	try {
		iss.str(jobj.toStyledString());
		iss >> document;
	}
	catch (const std::exception &e) {
		std::cerr << "json parsing failed: " << e.what() << " at offset: " << std::cin.tellg() << "\n";
		return false;
	}

	validator.validate(document, err);
	if (err) {
		std::cerr << "schema validation failed\n";
		return false;
	}

	trace("validate characteristic success.");
	writeLog("validate characteristic success.");

	return true;
}


bool OpenApiTestBase::checkStatus(int type, Json::Value jobj)
{
	std::string schema_path;

	json schema;
	json document;
	json_validator validator(loader, nlohmann::json_schema::default_string_format_check);
	custom_error_handler err;
	std::istringstream iss;
	std::ostringstream oss;

	switch (type) {
	case TEST_REQUEST_ALL:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_status_all.json");
		break;
	case TEST_REQUEST_DEVICE:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_status_device.json");
		break;
	case TEST_REQUEST_GROUP:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_status_group.json");
		break;
	case TEST_REQUEST_INDIVIDUAL:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_status_individual.json");
		break;
	default:
		tracee("invalid status type: %d", type);
		writeLog("invalid status type");
		return false;
	}

	std::ifstream f(schema_path);
	if (!f.good()) {
		tracee("cound not open schema file: %s", schema_path.c_str());
		return false;
	}
	try {
		f >> schema;
	}
	catch (const std::exception &e) {
		oss << e.what() << " at " << f.tellg() << " - while parsing the schema\n";
		tracee("%s", oss.str().c_str());
		return false;
	}

	try {
		validator.set_root_schema(schema);
	}
	catch (const std::exception &e) {
		std::cerr << "setting root schema failed\n";
		std::cerr << e.what() << "\n";
	}


	try {
		iss.str(jobj.toStyledString());
		iss >> document;
	}
	catch (const std::exception &e) {
		std::cerr << "json parsing failed: " << e.what() << " at offset: " << std::cin.tellg() << "\n";
		return false;
	}

	validator.validate(document, err);
	if (err) {
		std::cerr << "schema validation failed\n";
		return false;
	}

	trace("validate status success.");
	writeLog("validate status success.");

	return true;
}

bool OpenApiTestBase::checkControl(int type, Json::Value jobj_req, Json::Value jobj_res, int device_id, int sub_id)
{
	std::string schema_path;

	json schema;
	json document;
	json_validator validator(loader, nlohmann::json_schema::default_string_format_check);
	custom_error_handler err;
	std::istringstream iss;
	std::ostringstream oss;

	struct OpenApiCommandInfo cmd_info;
	std::vector<struct OpenApiCommandInfo> req_controls;
	std::vector<struct OpenApiCommandInfo> res_controls;

	switch (type) {
	case TEST_REQUEST_ALL:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_control_all.json");
		cmd_info.m_device_id = m_device_id;
		for (unsigned int i = 0; i < jobj_req["DeviceList"][0]["SubDeviceList"].size(); i++) {
			cmd_info.m_sub_id = hex_str2int(jobj_req["DeviceList"][0]["SubDeviceList"][i]["SubDeviceID"].asString());
			for (unsigned int j = 0; j < jobj_req["DeviceList"][0]["SubDeviceList"][i]["Control"].size(); j++) {
				cmd_info.m_obj = jobj_req["DeviceList"][0]["SubDeviceList"][i]["Control"][j];
				req_controls.push_back(cmd_info);
			}
		}
		for (unsigned int i = 0; i < jobj_res["DeviceList"][0]["SubDeviceList"].size(); i++) {
			cmd_info.m_sub_id = hex_str2int(jobj_res["DeviceList"][0]["SubDeviceList"][i]["SubDeviceID"].asString());
			for (unsigned int j = 0; j < jobj_res["DeviceList"][0]["SubDeviceList"][i]["Control"].size(); j++) {
				cmd_info.m_obj = jobj_res["DeviceList"][0]["SubDeviceList"][i]["Control"][j];
				res_controls.push_back(cmd_info);
			}
		}
		break;
	case TEST_REQUEST_DEVICE:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_control_device.json");
		cmd_info.m_device_id = m_device_id;
		for (unsigned int i = 0; i < jobj_req["SubDeviceList"].size(); i++) {
			cmd_info.m_sub_id = hex_str2int(jobj_req["SubDeviceList"][i]["SubDeviceID"].asString());
			for (unsigned int j = 0; j < jobj_req["SubDeviceList"][i]["Control"].size(); j++) {
				cmd_info.m_obj = jobj_req["SubDeviceList"][i]["Control"][j];
				req_controls.push_back(cmd_info);
			}
		}
		for (unsigned int i = 0; i < jobj_res["SubDeviceList"].size(); i++) {
			cmd_info.m_sub_id = hex_str2int(jobj_res["SubDeviceList"][i]["SubDeviceID"].asString());
			for (unsigned int j = 0; j < jobj_res["SubDeviceList"][i]["Control"].size(); j++) {
				cmd_info.m_obj = jobj_res["SubDeviceList"][i]["Control"][j];
				res_controls.push_back(cmd_info);
			}
		}
		break;
	case TEST_REQUEST_GROUP:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_control_group.json");
		cmd_info.m_device_id = m_device_id;
		cmd_info.m_sub_id = sub_id;
		for (unsigned int j = 0; j < jobj_req["Control"].size(); j++) {
			cmd_info.m_obj = jobj_req["Control"][j];
			req_controls.push_back(cmd_info);
		}
		for (unsigned int j = 0; j < jobj_res["Control"].size(); j++) {
			cmd_info.m_obj = jobj_res["Control"][j];
			res_controls.push_back(cmd_info);
		}
		break;
	case TEST_REQUEST_INDIVIDUAL:
		schema_path = ORG_DATA_HOME_PATH + std::string("/schema_control_individual.json");
		cmd_info.m_device_id = m_device_id;
		cmd_info.m_sub_id = sub_id;
		for (unsigned int j = 0; j < jobj_req["Control"].size(); j++) {
			cmd_info.m_obj = jobj_req["Control"][j];
			req_controls.push_back(cmd_info);
		}
		for (unsigned int j = 0; j < jobj_res["Control"].size(); j++) {
			cmd_info.m_obj = jobj_res["Control"][j];
			res_controls.push_back(cmd_info);
		}
		break;
	default:
		tracee("invalid control type: %d", type);
		writeLog("invalid control type");
		return false;
	}

	std::ifstream f(schema_path);
	if (!f.good()) {
		tracee("cound not open schema file: %s", schema_path.c_str());
		return false;
	}
	try {
		f >> schema;
	}
	catch (const std::exception &e) {
		oss << e.what() << " at " << f.tellg() << " - while parsing the schema\n";
		tracee("%s", oss.str().c_str());
		return false;
	}

	try {
		validator.set_root_schema(schema);
	}
	catch (const std::exception &e) {
		std::cerr << "setting root schema failed\n";
		std::cerr << e.what() << "\n";
	}


	try {
		iss.str(jobj_res.toStyledString());
		iss >> document;
	}
	catch (const std::exception &e) {
		std::cerr << "json parsing failed: " << e.what() << " at offset: " << std::cin.tellg() << "\n";
		return false;
	}

	validator.validate(document, err);
	if (err) {
		std::cerr << "schema validation failed\n";
		return false;
	}

	if (checkControlParameters(req_controls, res_controls) == false) {
		trace("check control parameters failed.");
		writeLog("check control parameters failed.");
		return false;
	}

	trace("validate control success.");
	writeLog("validate control success.");

	return true;
}

bool OpenApiTestBase::checkControlParameters(std::vector<struct OpenApiCommandInfo>& req_controls, std::vector<struct OpenApiCommandInfo>& res_controls)
{
	Json::Value c_info, res_info;
	std::vector<std::string> keys;
	bool is_found;
	std::string cmd_type, err_str, param_name;

	for (unsigned int i = 0; i < req_controls.size(); i++) {
		//trace("req: %x, %s", req_controls[i].m_sub_id, req_controls[i].m_obj.toStyledString().c_str());
		cmd_type = req_controls[i].m_obj["CommandType"].asString();

		//get response parameters in control_info
		is_found = false;
		if (m_control_info.isMember(cmd_type) == true) {
			c_info = m_control_info[cmd_type];
			is_found = true;
		}
		else {
			keys = m_control_info.getMemberNames();
			for (unsigned int j = 0; j < keys.size(); j++) {
				if (m_control_info[keys[j]]["RequestCode"].asString().compare(cmd_type) == 0) {
					c_info = m_control_info[keys[j]];
					is_found = true;
					break;
				}

			}
		}

		if (is_found == false) {
			err_str = std::string("there is no command type in control info: ") + req_controls[i].m_obj["CommandType"].asString();
			tracee("%s", err_str.c_str());
			writeLog(err_str);
			return false;
		}

		//get response
		is_found = false;
		for (unsigned int j = 0; j < res_controls.size(); j++) {
			if ((res_controls[j].m_obj["CommandType"].asString().compare(cmd_type) == 0) ||
					(res_controls[j].m_obj["CommandType"].asString().compare(c_info["ResponseCode"].asString()) == 0)) {
				res_info = res_controls[j].m_obj;
				is_found = true;
				break;
			}
		}

		if (is_found == false) {
			err_str = std::string("there is no response: ") + cmd_type + std::string(" or ") + c_info["ResponseCode"].asString();
			tracee("%s", err_str.c_str());
			writeLog(err_str);
			return false;
		}

		//trace("c_info: \n%s", c_info.toStyledString().c_str());
		//check response parameters;
		for (unsigned int j = 0; j < c_info["responseParameters"].size(); j++) {
			param_name = c_info["ResponseParameters"][j]["Name"].asString();

			is_found = false;
			for (unsigned int k = 0; k < res_info["Parameters"].size(); k++) {
				if (res_info["Parameters"][k]["Name"].asString().compare(param_name) == 0) {
					is_found = true;
					break;
				}
			}

			if (is_found == false) {
				err_str = std::string("there is no param in response: ") + param_name;
				tracee("%s", err_str.c_str());
				writeLog(err_str);
				return false;
			}

			err_str = std::string("check parameter ok: ") + 
				int2hex_str(req_controls[i].m_device_id) + std::string("/") + 
				int2hex_str(req_controls[i].m_sub_id) + std::string("/") + param_name;
			trace("%s", err_str.c_str());
			writeLog(err_str);
		}
	}
	trace("check all parameters OK.");
	writeLog("check all parameters OK.");

	return true;
}

int OpenApiTestBase::test_base_1(struct TestItem* item)
{
	Json::Value jobj_char;
	std::string url;

	url = m_base_url + std::string("/characteristic");
	jobj_char = sendGetRequest(url);

	if (jobj_char == Json::Value::null) {
		tracee("send GET request failed: %s", url.c_str());
		item->m_result = TEST_RESULT_FAIL;
		item->m_description = "send GET request failed.";

		writeLog("send GET request failed.");

		return item->m_result;
	}

	if (checkCharacteristic(TEST_REQUEST_ALL, jobj_char) == false) {
		tracee("check characteristic failed: %s", jobj_char.toStyledString().c_str());
		item->m_result = TEST_RESULT_FAIL;
		item->m_description = "check characteristic failed.";

		writeLog("check characteristic failed.");

		return item->m_result;
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_2(struct TestItem* item)
{
	Json::Value jobj_char;
	std::string url;

	url = m_base_url + std::string("/characteristic/" + int2hex_str(m_device_id));
	jobj_char = sendGetRequest(url);

	if (jobj_char == Json::Value::null) {
		tracee("send GET request failed: %s", url.c_str());
		item->m_result = TEST_RESULT_FAIL;
		item->m_description = "send GET request failed.";

		return item->m_result;
	}

	if (checkCharacteristic(TEST_REQUEST_DEVICE, jobj_char) == false) {
		tracee("check characteristic failed: %s", jobj_char.toStyledString().c_str());
		item->m_result = TEST_RESULT_FAIL;
		item->m_description = "send GET request failed.";

		return item->m_result;
	}

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_3(struct TestItem* item)
{
	Json::Value jobj_char;
	std::string url;
	int group_id;

	if (m_device_groups.size() == 0) {
		trace("the device has not a group: skip");
		writeLog("the device has not a group: skip");
		item->m_result = TEST_RESULT_SKIPPED;
		return item->m_result;
	}

	for (unsigned int i = 0; i < m_device_groups.size(); i++) {
		group_id = m_device_groups[i];

		url = m_base_url + std::string("/characteristic/" + int2hex_str(m_device_id) + "/" + int2hex_str(group_id));
		jobj_char = sendGetRequest(url);

		if (jobj_char == Json::Value::null) {
			tracee("send GET request failed: %s", url.c_str());
			item->m_result = TEST_RESULT_FAIL;
			item->m_description = "send GET request failed.";

			writeLog("send GET request failed.");

			return item->m_result;
		}

		if (checkCharacteristic(TEST_REQUEST_GROUP, jobj_char) == false) {
			tracee("check characteristic failed: %s", jobj_char.toStyledString().c_str());
			item->m_result = TEST_RESULT_FAIL;
			item->m_description = "check characteristic failed.";

			writeLog("check characteristic failed.");

			return item->m_result;
		}
	}
	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_4(struct TestItem* item)
{
	Json::Value jobj_char;
	std::string url;
	int sub_id;

	for (unsigned int i = 0; i < m_device_sub_ids.size(); i++) {
		sub_id = m_device_sub_ids[i];
		url = m_base_url + std::string("/characteristic/" + int2hex_str(m_device_id) + "/" + int2hex_str(sub_id));
		jobj_char = sendGetRequest(url);

		if (jobj_char == Json::Value::null) {
			tracee("send GET request failed: %s", url.c_str());
			item->m_result = TEST_RESULT_FAIL;
			item->m_description = "send GET request failed.";

			return item->m_result;
		}

		if (checkCharacteristic(TEST_REQUEST_INDIVIDUAL, jobj_char) == false) {
			tracee("send get request failed: %s", jobj_char.toStyledString().c_str());
			item->m_result = TEST_RESULT_FAIL;
			item->m_description = "send GET request failed.";

			return item->m_result;
		}
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_5(struct TestItem* item)
{
	Json::Value jobj_char;
	std::string url;

	url = m_base_url + std::string("/status");
	jobj_char = sendGetRequest(url);

	if (jobj_char == Json::Value::null) {
		tracee("send GET request failed: %s", url.c_str());
		item->m_result = TEST_RESULT_FAIL;
		item->m_description = "send GET request failed.";

		writeLog("send GET request failed.");

		return item->m_result;
	}

	if (checkStatus(TEST_REQUEST_ALL, jobj_char) == false) {
		tracee("check status failed: %s", jobj_char.toStyledString().c_str());
		item->m_result = TEST_RESULT_FAIL;
		item->m_description = "check status failed.";

		writeLog("check status failed.");

		return item->m_result;
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_6(struct TestItem* item)
{
	Json::Value jobj_char;
	std::string url;

	url = m_base_url + std::string("/status/" + int2hex_str(m_device_id));
	jobj_char = sendGetRequest(url);

	if (jobj_char == Json::Value::null) {
		tracee("send GET request failed: %s", url.c_str());
		item->m_result = TEST_RESULT_FAIL;
		item->m_description = "send GET request failed.";

		writeLog("send GET request failed.");

		return item->m_result;
	}

	if (checkStatus(TEST_REQUEST_DEVICE, jobj_char) == false) {
		tracee("check status failed: %s", jobj_char.toStyledString().c_str());
		item->m_result = TEST_RESULT_FAIL;
		item->m_description = "check status failed.";

		writeLog("check status failed.");

		return item->m_result;
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_7(struct TestItem* item)
{
	Json::Value jobj_char;
	std::string url;
	int group_id;

	if (m_device_groups.size() == 0) {
		trace("the device has not a group: skip");
		writeLog("the device has not a group: skip");
		item->m_result = TEST_RESULT_SKIPPED;
		return item->m_result;
	}

	for (unsigned int i = 0; i < m_device_groups.size(); i++) {
		group_id = m_device_groups[i];

		url = m_base_url + std::string("/status/" + int2hex_str(m_device_id) + "/" + int2hex_str(group_id));
		jobj_char = sendGetRequest(url);

		if (jobj_char == Json::Value::null) {
			tracee("send GET request failed: %s", url.c_str());
			item->m_result = TEST_RESULT_FAIL;
			item->m_description = "send GET request failed.";

			writeLog("send GET request failed.");

			return item->m_result;
		}

		if (checkStatus(TEST_REQUEST_GROUP, jobj_char) == false) {
			tracee("check status failed: %s", jobj_char.toStyledString().c_str());
			item->m_result = TEST_RESULT_FAIL;
			item->m_description = "check status failed.";

			writeLog("check status failed.");

			return item->m_result;
		}
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_8(struct TestItem* item)
{
	Json::Value jobj_char;
	std::string url;
	int sub_id;

	for (unsigned int i = 0; i < m_device_sub_ids.size(); i++) {
		sub_id = m_device_sub_ids[i];
		url = m_base_url + std::string("/status/" + int2hex_str(m_device_id) + "/" + int2hex_str(sub_id));
		jobj_char = sendGetRequest(url);

		if (jobj_char == Json::Value::null) {
			tracee("send GET request failed: %s", url.c_str());
			item->m_result = TEST_RESULT_FAIL;
			item->m_description = "send GET request failed.";

			writeLog("send GET request failed.");

			return item->m_result;
		}

		if (checkStatus(TEST_REQUEST_INDIVIDUAL, jobj_char) == false) {
			tracee("check status failed: %s", jobj_char.toStyledString().c_str());
			item->m_result = TEST_RESULT_FAIL;
			item->m_description = "check status failed.";

			writeLog("check status failed.");

			return item->m_result;
		}
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_9(struct TestItem* item)
{
	Json::Value jobj_control;
	Json::Value jobj_sub;
	Json::Value jobj_sub_list;
	Json::Value jobj_dev;
	Json::Value jobj_dev_list;
	std::string url;
	int sub_id;

	Json::Value jobj_res;

	url = m_base_url + std::string("/control");

	writeLog("send individual control.");
	if (m_test_control_cmds.isMember("individual") == true) {
		for (unsigned int i = 0; i < m_test_control_cmds["individual"].size(); i++) {
			jobj_control.clear();
			jobj_dev_list.clear();
			jobj_dev.clear();
			jobj_sub_list.clear();

			for (unsigned int sub_id_idx = 0; sub_id_idx < m_device_sub_ids.size(); sub_id_idx++) {
				jobj_sub.clear();

				sub_id = m_device_sub_ids[sub_id_idx];
				jobj_sub["SubDeviceID"] = int2hex_str(sub_id);
				jobj_sub["Control"].append(m_test_control_cmds["individual"][i]);

				jobj_sub_list.append(jobj_sub);
			}

			jobj_dev["DeviceID"] = int2hex_str(m_device_id);
			jobj_dev["SubDeviceList"] = jobj_sub_list;
			jobj_dev_list.append(jobj_dev);
			jobj_control["DeviceList"] = jobj_dev_list;

			jobj_res = sendPutRequest(url, jobj_control);
			if (jobj_res == Json::Value::null) {
				tracee("send post request failed: %s", url.c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "send PUT request failed.";

				writeLog("send PUT request failed.");

				return item->m_result;
			}

			if (checkControl(TEST_REQUEST_ALL, jobj_control, jobj_res) == false) {
				tracee("check control failed: %s", jobj_res.toStyledString().c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "check control failed.";

				writeLog("check control failed.");

				return item->m_result;
			}
		}
	}

	writeLog("send group control.");
	if (m_test_control_cmds.isMember("group") == true) {
		for (unsigned int i = 0; i < m_test_control_cmds["group"].size(); i++) {
			jobj_control.clear();
			jobj_dev_list.clear();
			jobj_dev.clear();
			jobj_sub_list.clear();
			jobj_sub.clear();

			for (unsigned int j = 0; j < m_device_groups.size(); j++) {
				sub_id = m_device_groups[j];
				jobj_sub["SubDeviceID"] = int2hex_str(sub_id);
				jobj_sub["Control"].append(m_test_control_cmds["group"][i]);

				jobj_sub_list.append(jobj_sub);
			}

			jobj_dev["DeviceID"] = int2hex_str(m_device_id);
			jobj_dev["SubDeviceList"] = jobj_sub_list;
			jobj_dev_list.append(jobj_dev);
			jobj_control["DeviceList"] = jobj_dev_list;

			jobj_res = sendPutRequest(url, jobj_control);
			if (jobj_res == Json::Value::null) {
				tracee("send post request failed: %s", url.c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "send PUT request failed.";

				writeLog("send PUT request failed.");

				return item->m_result;
			}

			if (checkControl(TEST_REQUEST_ALL, jobj_control, jobj_res) == false) {
				tracee("check control failed: %s", jobj_res.toStyledString().c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "check control failed.";

				writeLog("check control failed.");

				return item->m_result;
			}
		}
	}

	writeLog("send all control.");
	if (m_test_control_cmds.isMember("all") == true) {
		for (unsigned int i = 0; i < m_test_control_cmds["all"].size(); i++) {
			jobj_control.clear();
			jobj_dev_list.clear();
			jobj_dev.clear();
			jobj_sub_list.clear();
			jobj_sub.clear();

			sub_id = 0xff;
			jobj_sub["SubDeviceID"] = int2hex_str(sub_id);
			jobj_sub["Control"].append(m_test_control_cmds["all"][i]);

			jobj_sub_list.append(jobj_sub);

			jobj_dev["DeviceID"] = int2hex_str(m_device_id);
			jobj_dev["SubDeviceList"] = jobj_sub_list;
			jobj_dev_list.append(jobj_dev);
			jobj_control["DeviceList"] = jobj_dev_list;

			jobj_res = sendPutRequest(url, jobj_control);
			if (jobj_res == Json::Value::null) {
				tracee("send post request failed: %s", url.c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "send PUT request failed.";

				writeLog("send PUT request failed.");

				return item->m_result;
			}

			if (checkControl(TEST_REQUEST_ALL, jobj_control, jobj_res) == false) {
				tracee("check control failed: %s", jobj_res.toStyledString().c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "check control failed.";

				writeLog("check control failed.");

				return item->m_result;
			}
		}
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_10(struct TestItem* item)
{
	Json::Value jobj_control;
	Json::Value jobj_sub;
	Json::Value jobj_sub_list;
	std::string url;
	int sub_id;

	Json::Value jobj_res;

	url = m_base_url + std::string("/control/") + int2hex_str(m_device_id);

	writeLog("send individual control.");
	if (m_test_control_cmds.isMember("individual") == true) {
		for (unsigned int i = 0; i < m_test_control_cmds["individual"].size(); i++) {
			jobj_control.clear();
			jobj_sub_list.clear();

			for (unsigned int sub_id_idx = 0; sub_id_idx < m_device_sub_ids.size(); sub_id_idx++) {
				jobj_sub.clear();

				sub_id = m_device_sub_ids[sub_id_idx];
				jobj_sub["SubDeviceID"] = int2hex_str(sub_id);
				jobj_sub["Control"].append(m_test_control_cmds["individual"][i]);

				jobj_sub_list.append(jobj_sub);
			}

			jobj_control["SubDeviceList"] = jobj_sub_list;

			jobj_res = sendPutRequest(url, jobj_control);
			if (jobj_res == Json::Value::null) {
				tracee("send post request failed: %s", url.c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "send PUT request failed.";

				writeLog("send PUT request failed.");

				return item->m_result;
			}

			if (checkControl(TEST_REQUEST_DEVICE, jobj_control, jobj_res) == false) {
				tracee("check control failed: %s", jobj_res.toStyledString().c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "check control failed.";

				writeLog("check control failed.");

				return item->m_result;
			}
		}
	}

	writeLog("send group control.");
	if (m_test_control_cmds.isMember("group") == true) {
		for (unsigned int i = 0; i < m_test_control_cmds["group"].size(); i++) {
			jobj_control.clear();
			jobj_sub_list.clear();
			jobj_sub.clear();

			for (unsigned int j = 0; j < m_device_groups.size(); j++) {
				sub_id = m_device_groups[j];
				jobj_sub["SubDeviceID"] = int2hex_str(sub_id);
				jobj_sub["Control"].append(m_test_control_cmds["group"][i]);

				jobj_sub_list.append(jobj_sub);
			}

			jobj_control["SubDeviceList"] = jobj_sub_list;

			jobj_res = sendPutRequest(url, jobj_control);
			if (jobj_res == Json::Value::null) {
				tracee("send post request failed: %s", url.c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "send PUT request failed.";

				writeLog("send PUT request failed.");

				return item->m_result;
			}

			if (checkControl(TEST_REQUEST_DEVICE, jobj_control, jobj_res) == false) {
				tracee("check control failed: %s", jobj_res.toStyledString().c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "check control failed.";

				writeLog("check control failed.");

				return item->m_result;
			}
		}
	}

	writeLog("send all control.");
	if (m_test_control_cmds.isMember("all") == true) {
		for (unsigned int i = 0; i < m_test_control_cmds["all"].size(); i++) {
			jobj_control.clear();
			jobj_sub_list.clear();
			jobj_sub.clear();

			sub_id = 0xff;
			jobj_sub["SubDeviceID"] = int2hex_str(sub_id);
			jobj_sub["Control"].append(m_test_control_cmds["all"][i]);

			jobj_sub_list.append(jobj_sub);

			jobj_control["SubDeviceList"] = jobj_sub_list;

			jobj_res = sendPutRequest(url, jobj_control);
			if (jobj_res == Json::Value::null) {
				tracee("send post request failed: %s", url.c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "send PUT request failed.";

				writeLog("send PUT request failed.");

				return item->m_result;
			}

			if (checkControl(TEST_REQUEST_DEVICE, jobj_control, jobj_res) == false) {
				tracee("check control failed: %s", jobj_res.toStyledString().c_str());
				item->m_result = TEST_RESULT_FAIL;
				item->m_description = "check control failed.";

				writeLog("check control failed.");

				return item->m_result;
			}
		}
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}

int OpenApiTestBase::test_base_11(struct TestItem* item)
{
	Json::Value jobj_control;
	Json::Value jobj_sub;
	std::string url;
	Json::Value jobj_res;
	int group_id;

	if (m_device_groups.size() == 0) {
		trace("the device has not a group: skip");
		writeLog("the device has not a group: skip");
		item->m_result = TEST_RESULT_SKIPPED;
		return item->m_result;
	}

	for (unsigned int i = 0; i < m_device_groups.size(); i++) {
		group_id = m_device_groups[i];

		url = m_base_url + std::string("/control/") + int2hex_str(m_device_id) + "/" + int2hex_str(group_id);
		writeLog("send group control.");
		if (m_test_control_cmds.isMember("group") == true) {
			for (unsigned int i = 0; i < m_test_control_cmds["group"].size(); i++) {
				jobj_control.clear();
				jobj_sub.clear();

				jobj_sub["SubDeviceID"] = int2hex_str(group_id);
				jobj_sub["Control"].append(m_test_control_cmds["group"][i]);

				jobj_control["SubDeviceList"].append(jobj_sub);

				jobj_res = sendPutRequest(url, jobj_control);
				if (jobj_res == Json::Value::null) {
					tracee("send post request failed: %s", url.c_str());
					item->m_result = TEST_RESULT_FAIL;
					item->m_description = "send PUT request failed.";

					writeLog("send PUT request failed.");

					return item->m_result;
				}

				if (checkControl(TEST_REQUEST_GROUP, jobj_control, jobj_res, m_device_id, group_id) == false) {
					tracee("check control failed: %s", jobj_res.toStyledString().c_str());
					item->m_result = TEST_RESULT_FAIL;
					item->m_description = "check control failed.";

					writeLog("check control failed.");

					return item->m_result;
				}
			}
		}
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}


int OpenApiTestBase::test_base_12(struct TestItem* item)
{
	Json::Value jobj_control;
	Json::Value jobj_sub;
	std::string url;
	int sub_id;

	Json::Value jobj_res;


	writeLog("send individual control.");
	if (m_test_control_cmds.isMember("individual") == true) {
		for (unsigned int i = 0; i < m_test_control_cmds["individual"].size(); i++) {
			jobj_control.clear();

			for (unsigned int sub_id_idx = 0; sub_id_idx < m_device_sub_ids.size(); sub_id_idx++) {
				jobj_sub.clear();

				sub_id = m_device_sub_ids[sub_id_idx];
				url = m_base_url + std::string("/control/") + int2hex_str(m_device_id) + "/" + int2hex_str(sub_id);

				jobj_sub["Control"].append(m_test_control_cmds["individual"][i]);
				jobj_control = jobj_sub;

				jobj_res = sendPutRequest(url, jobj_control);
				if (jobj_res == Json::Value::null) {
					tracee("send post request failed: %s", url.c_str());
					item->m_result = TEST_RESULT_FAIL;
					item->m_description = "send PUT request failed.";

					writeLog("send PUT request failed.");

					return item->m_result;
				}

				if (checkControl(TEST_REQUEST_INDIVIDUAL, jobj_control, jobj_res, m_device_id, sub_id) == false) {
					tracee("check control failed: %s", jobj_res.toStyledString().c_str());
					item->m_result = TEST_RESULT_FAIL;
					item->m_description = "check control failed.";

					writeLog("check control failed.");

					return item->m_result;
				}
			}
		}
	}

	writeLog("test success.");

	item->m_result = TEST_RESULT_SUCCESS;
	item->m_description = "success";
	return item->m_result;
}




