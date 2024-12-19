
#include "home_device_lib.h"
#include "http_server.h"
#include "serial_device.h"
#include "trace.h"
#include "http_request.h"
#include "http_response.h"
#include "prevent_crime_ext_device.h"
#include "tools.h"

#include <json/json.h>

#include <string.h>
#include <string>
#include <vector>

#ifdef WIN32
#include <win_porting.h>
#else
#include <unistd.h>
#endif

bool HomeDevice::m_class_init = false;
std::map<int, std::string> HomeDevice::m_error_description;
std::mutex HomeDevice::m_mutex;

bool home_device_http_callback(void* instance, HttpRequest* request, HttpResponse* response)
{
	HomeDevice* home_device;

	home_device = (HomeDevice*)instance;
	return home_device->recvHttp(request, response);
}

void home_device_httpu_callback(void* instance, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	HomeDevice* home_device;

	home_device = (HomeDevice*)instance;
	return home_device->recvHttpu(message, sock_info, from_addr);
}

HomeDevice::HomeDevice()
{
	m_mutex.lock();
	if (m_class_init == false) {
		m_class_init = true;

		m_error_description[ERROR_CODE_NO_SUCH_DEVICE] = "no such device";
		m_error_description[ERROR_CODE_INTERNAL_ERROR] = "internal operation error";
		m_error_description[ERROR_CODE_INVALID_COMMAND_TYPE] = "invalid command type";
		m_error_description[ERROR_CODE_INVALID_PARAMETER] = "invalid parameter";
	}
	m_mutex.unlock();
}

HomeDevice::~HomeDevice()
{
	HomeDevice::stop();

	if(m_serial_device != NULL){
		delete m_serial_device;
		m_serial_device = NULL;
	}
	if(m_http_server != NULL){
		delete m_http_server;
		m_http_server = NULL;
	}
}

int HomeDevice::init(std::string json_str, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value json_obj;
	std::string err;
	int length;

	length = json_str.length();
	if (parseJson(json_str.c_str(), json_str.c_str() + length, &json_obj, &err) == false) {
		return -1;
	}

	return init(json_obj, sub_ids, http_port);
}

int HomeDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	std::string err;
	Json::Value tmp_json_obj;
	Json::Value tmp_json_obj2;

	m_serial_device = NULL;
	m_http_server = NULL;
	m_run_flag = false;
	m_get_cmd_tid = NULL;
	m_send_adv_tid = NULL;
	m_process_discovery_tid = NULL;

	m_serial_callback_instance = NULL;
	m_before_process_cmd_callback = NULL;
	m_before_send_cmd_callback = NULL;
	m_after_process_cmd_callback = NULL;


	m_http_callback_instance = NULL;
	m_recv_before_process_http_callback = NULL;
	m_recv_after_process_http_callback = NULL;

	m_httpu_callback_instance = NULL;
	m_recv_before_process_httpu_callback = NULL;
	m_recv_after_process_httpu_callback = NULL;

	m_debug_level = 99;
	if(conf_json["DebugLevel"].type() == Json::intValue){
		m_debug_level = conf_json["DebugLevel"].asInt();
	}
	traceSetLevel(m_debug_level);

	trace("device init: \n%s", conf_json.toStyledString().c_str());
	if (http_port == -1) {
		if (conf_json["Http"].type() == Json::objectValue) {
			tmp_json_obj = conf_json["Http"];
			if (tmp_json_obj["Port"].type() == Json::intValue) {
				m_http_port = tmp_json_obj["Port"].asInt();
			}
			else {
				tracee("there is no http port.");
				return -1;
			}
		}
		else {
			tracee("there is no http.");
			return -1;
		}
	}
	else {
		m_http_port = http_port;
	}

	if(conf_json["Device"].type() == Json::objectValue){
		tmp_json_obj = conf_json["Device"];

		if(tmp_json_obj["DeviceID"].type() == Json::intValue){
			m_device_id = tmp_json_obj["DeviceID"].asInt();
			m_device_name = getDeviceNameString(m_device_id);
		}
		else{
			tracee("there is no device id.");
			return -1;
		}

		if (sub_ids.size() == 0) {
			if (tmp_json_obj["SubDeviceIDs"].type() == Json::arrayValue) {
				for (unsigned int i = 0; i < tmp_json_obj["SubDeviceIDs"].size(); i++) {
					if (tmp_json_obj["SubDeviceIDs"][i].type() == Json::intValue) {
						m_sub_ids.push_back(tmp_json_obj["SubDeviceIDs"][i].asInt());
					}
				}
			}
			else {
				tracee("there is no subid.");
				return -1;
			}
		}
		else {
			m_sub_ids = sub_ids;
		}
	}
	else{
		tracee("there is no device.");
		return -1;
	}

	m_sub_gid = m_sub_ids[0] & 0xf0;

	//trace("serial: %s, %d", m_serial_port.c_str(), m_serial_speed);
	trace("http: %d", m_http_port);

	m_serial_device = new SerialDevice();
	//if(m_serial_device->init(m_serial_port, m_serial_speed) < 0){
	if(m_serial_device->init() < 0){
		tracee("serial device (multicast) init failed.");
		delete m_serial_device;
		return -1;
	}

	m_http_server = new HttpServer();
	if (m_http_server->init(m_http_port) < 0) {
		tracee("http server init failed.");
		return -1;
	}
	m_http_server->setCallback(this, home_device_http_callback);

	m_httpu_server = new HttpuServer();
	if (m_httpu_server->init(HTTPU_DISCOVERY_ADDR, HTTPU_DISCOVERY_PORT) < 0) {
		tracee("httpu server init failed.");
		return -1;
	}
	m_httpu_server->setCallback(this, home_device_httpu_callback);

	m_data_home_path = DATA_HOME_PATH;
	m_ui_home_path = m_data_home_path + "/web";

	return 0;
}

int HomeDevice::finalizeInit()
{
	struct DeviceInformation dev_info;
	std::vector<std::string> my_addrs;
	Json::Value obj_conf, obj_confs, obj_tmp;
	Json::Value obj_addrs;

	dev_info.m_device_id = m_device_id;
	dev_info.m_device_name = m_device_name;
	dev_info.m_sub_ids = m_sub_ids;
	dev_info.m_sub_gid = m_sub_gid;
	dev_info.m_type = DEVICE_TYPE_BOTH;
	m_devices.push_back(dev_info);

	my_addrs = m_httpu_server->getMyAddrs();
	for (unsigned int i = 0; i < my_addrs.size(); i++) {
		obj_addrs.append(my_addrs[i]);
	}

	obj_conf["Name"] = "DebugLevel";
	obj_conf["Type"] = "integer";
	obj_conf["Value"] = m_debug_level;
	obj_confs.append(obj_conf);

	obj_conf["Name"] = "DeviceID";
	obj_conf["Type"] = "string";
	obj_conf["Value"] = int2hex_str(m_device_id);
	obj_confs.append(obj_conf);

	obj_tmp = Json::arrayValue;
	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		obj_tmp.append(int2hex_str(m_sub_ids[i]));
	}
	obj_conf["Name"] = "DeviceSubIDs";
	obj_conf["Type"] = "stringarray";
	obj_conf["Value"] = obj_tmp;
	obj_confs.append(obj_conf);

	obj_conf["Name"] = "DeviceName";
	obj_conf["Type"] = "string";
	obj_conf["Value"] = m_device_name;
	obj_confs.append(obj_conf);

	obj_conf["Name"] = "Addresses";
	obj_conf["Type"] = "stringarray";
	obj_conf["Value"] = obj_addrs;
	obj_confs.append(obj_conf);

	obj_conf["Name"] = "HttpPort";
	obj_conf["Type"] = "integer";
	obj_conf["Value"] = m_http_port;
	obj_confs.append(obj_conf);

	obj_conf["Name"] = "Version";
	obj_conf["Type"] = "integer";
	obj_conf["Value"] = 1;
	obj_confs.append(obj_conf);

	m_obj_configuration["Configuration"] = obj_confs;

	return 0;
}

void HomeDevice::setDataHomePath(std::string path)
{
	trace("set data home: %s", path.c_str());
	m_data_home_path = path;
	m_ui_home_path = m_data_home_path + "/web";

	setCmdInfo();
}

int HomeDevice::getDeviceId()
{
	return m_device_id;
}

void HomeDevice::setCmdInfo()
{
}

void HomeDevice::setStatus()
{
	return;
}

Json::Value HomeDevice::getStatus(std::string device_id, std::string sub_id)
{
	Json::Value ret;
	Json::Value devs, dev, sub_devs, sub_dev;
	bool is_found;
	int gid_int, sub_id_int;

	trace("get status: %s, %s", device_id.c_str(), sub_id.c_str());

	sub_id_int = hex_str2int(sub_id);
	gid_int = sub_id_int & 0xf0;
	sub_id_int &= 0x0f;

	if ((gid_int != m_sub_gid) && (gid_int != 0xf0)){
		ret = Json::nullValue;
		return ret;
	}

	if ((device_id.empty() == true) || (device_id.compare("0xff") == 0)) {
		ret = m_obj_status;
	}
	else if ((sub_id.empty() == true) || (sub_id_int == 0x0f)) {
		devs = m_obj_status["DeviceList"];
		is_found = false;
		for (unsigned int i = 0; i < devs.size(); i++) {
			dev = devs[i];
			if (dev["DeviceID"].asString().compare(device_id) == 0) {
				is_found = true;
				break;
			}
		}

		if (is_found == false) {
			ret = Json::nullValue;
			return ret;
		}

		ret["SubDeviceList"] = dev["SubDeviceList"];
	}
	else {
		devs = m_obj_status["DeviceList"];
		is_found = false;
		for (unsigned int i = 0; i < devs.size(); i++) {
			dev = devs[i];
			if (dev["DeviceID"].asString().compare(device_id) == 0) {
				is_found = true;
				break;
			}
		}

		if (is_found == false) {
			ret = Json::nullValue;
			return ret;
		}

		sub_devs = dev["SubDeviceList"];

		is_found = false;
		for (unsigned int i = 0; i < sub_devs.size(); i++) {
			sub_dev = sub_devs[i];
			if (sub_dev["SubDeviceID"].asString().compare(sub_id) == 0) {
				is_found = true;
				break;
			}
		}

		if (is_found == false) {
			ret = Json::nullValue;
			return ret;
		}

		ret["Status"] = sub_dev["Status"];
		trace("status: \n%s", ret["Status"].asString().c_str());
	}

	return ret;
}

Json::Value HomeDevice::getCharacteristic(std::string device_id, std::string sub_id)
{
	Json::Value ret;
	Json::Value devs, dev, sub_devs, sub_dev;
	bool is_found;
	int gid_int, sub_id_int;

	trace("get characteristic: %s, %s", device_id.c_str(), sub_id.c_str());

	sub_id_int = hex_str2int(sub_id);
	gid_int = sub_id_int & 0xf0;
	sub_id_int &= 0x0f;

	if ((gid_int != m_sub_gid) && (gid_int != 0xf0)) {
		ret = Json::nullValue;
		return ret;
	}

	if ((device_id.empty() == true) || (device_id.compare("0xff") == 0)) {
		ret = m_obj_characteristic;
	}
	else if ((sub_id.empty() == true) || (sub_id_int == 0x0f)) {
		devs = m_obj_characteristic["DeviceList"];
		is_found = false;
		for (unsigned int i = 0; i < devs.size(); i++) {
			dev = devs[i];
			if (dev["DeviceID"].asString().compare(device_id) == 0) {
				is_found = true;
				break;
			}
		}

		if (is_found == false) {
			ret = Json::nullValue;
			return ret;
		}

		ret["SubDeviceList"] = dev["SubDeviceList"];
	}
	else {
		devs = m_obj_characteristic["DeviceList"];
		is_found = false;
		for (unsigned int i = 0; i < devs.size(); i++) {
			dev = devs[i];
			if (dev["DeviceID"].asString().compare(device_id) == 0) {
				is_found = true;
				break;
			}
		}

		if (is_found == false) {
			ret = Json::nullValue;
			return ret;
		}

		sub_devs = dev["SubDeviceList"];

		is_found = false;
		for (unsigned int i = 0; i < sub_devs.size(); i++) {
			sub_dev = sub_devs[i];
			if (sub_dev["SubDeviceID"].asString().compare(sub_id) == 0) {
				is_found = true;
				break;
			}
		}

		if (is_found == false) {
			ret = Json::nullValue;
			return ret;
		}

		ret["Characteristic"] = sub_dev["Characteristic"];
	}

	return ret;

}

Json::Value HomeDevice::createErrorResponse(int code, std::string description)
{
	Json::Value ret;
	Json::Value error;

	error["Code"] = code;
	if (description.empty() == true) {
		error["Description"] = getErrorDescription(code);
	}
	else {
		error["Description"] = description;
	}

	ret["Error"] = error;

	return ret;
}

std::string HomeDevice::getErrorDescription(int code)
{
	std::string ret;
	std::map<int, std::string>::iterator it;

	it = m_error_description.find(code);
	if (it == m_error_description.cend()) {
		return ret;
	}
	ret = it->second;

	return ret;
}

int HomeDevice::run()
{
	if(m_run_flag == true){
		return 0;
	}

	m_run_flag = true;

	if(m_serial_device->run() < 0){
		tracee("run serial device failed.");
		return -1;
	}

	if (m_http_server->run() < 0) {
		tracee("run http server failed.");
		return -1;
	}

	if (m_httpu_server->run() < 0) {
		tracee("run http server failed.");
		return -1;
	}

	m_get_cmd_tid = new std::thread(&HomeDevice::runSerialCommandReceiverThread, this);
	m_send_adv_tid = new std::thread(&HomeDevice::runSendAdvertisementThread, this);
	m_process_discovery_tid = new std::thread(&HomeDevice::runProcessDiscoveryThread, this);

	return 0;
}

void HomeDevice::stop()
{
	if(m_run_flag == false){
		return;
	}

	m_run_flag = false;

	m_serial_device->stop();
	m_http_server->stop();

	if(m_get_cmd_tid != NULL){
		m_get_cmd_tid->join();
		delete m_get_cmd_tid;
		m_get_cmd_tid = NULL;
	}

	if (m_send_adv_tid != NULL) {
		m_send_adv_tid->join();
		delete m_send_adv_tid;
		m_send_adv_tid = NULL;
	}

	if (m_process_discovery_tid != NULL) {
		m_process_discovery_tid->join();
		delete m_process_discovery_tid;
		m_process_discovery_tid = NULL;
	}

	return;
}

void HomeDevice::setCallback(void* instance, ProcessSerialCommandCallback callback1, SendSerialCommandCallback callback2, ProcessSerialCommandCallback callback3)
{
	m_serial_callback_instance = instance;
	m_before_process_cmd_callback = callback1;
	m_before_send_cmd_callback = callback2;
	m_after_process_cmd_callback = callback3;
}

void HomeDevice::setCallback(void* instance, recvHttpCallback before_callback, recvHttpCallback after_callback)
{
	m_http_callback_instance = instance;
	m_recv_before_process_http_callback = before_callback;
	m_recv_after_process_http_callback = after_callback;
}

void HomeDevice::setCallback(void* instance, recvHttpuCallback before_callback, recvHttpuCallback after_callback)
{
	m_httpu_callback_instance = instance;
	m_recv_before_process_httpu_callback = before_callback;
	m_recv_after_process_httpu_callback = after_callback;
}

bool HomeDevice::recvHttp(HttpRequest* request, HttpResponse* response)
{
	bool ret;

	if (m_recv_before_process_http_callback != NULL) {
		if (m_recv_before_process_http_callback(m_http_callback_instance, this, request, response) == false) {
			return true;
		}
	}

	ret = recvHttpRequest(request, response);

	if (m_recv_after_process_http_callback != NULL) {
		if (m_recv_after_process_http_callback(m_http_callback_instance, this, request, response) == false) {
			return ret;
		}
	}

	return ret;
}

void HomeDevice::recvHttpu(HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	if (m_recv_before_process_httpu_callback != NULL) {
		if (m_recv_before_process_httpu_callback(m_http_callback_instance, this, message, sock_info, from_addr) == false) {
			return;
		}
	}

	recvHttpuMessage(message, sock_info, from_addr);

	if (m_recv_after_process_httpu_callback != NULL) {
		if (m_recv_after_process_httpu_callback(m_http_callback_instance, this, message, sock_info, from_addr) == false) {
			return;
		}
	}
}

std::vector<struct OpenApiCommandInfo> HomeDevice::parseOpenApiCommand_device_group(int device_id, int sub_id, Json::Value obj)
{
	std::vector<struct OpenApiCommandInfo> cmds;
	struct OpenApiCommandInfo cmd;
	Json::Value obj_controls;
	int new_sub_id;
	std::string str_sub_id;

	if (obj.isMember("SubDeviceID") == false) {
		tracee("there is no SubDeviceID");
		return cmds;
	}
	str_sub_id = obj["SubDeviceID"].asString();
	new_sub_id = hex_str2int(str_sub_id);

	if (obj.isMember("Control") == false) {
		tracee("there is no Control");
		return cmds;
	}

	obj_controls = obj["Control"];
	if (obj_controls.type() != Json::arrayValue) {
		tracee("Control is not array");
		return cmds;
	}

	for (unsigned int i = 0; i < obj_controls.size(); i++) {
		cmd.m_device_id = device_id;
		cmd.m_sub_id = new_sub_id;
		cmd.m_obj = obj_controls[i];

		cmds.push_back(cmd);
	}

	return cmds;
}

std::vector<struct OpenApiCommandInfo> HomeDevice::parseOpenApiCommand_deviceList(int device_id, int sub_id, Json::Value obj)
{
	std::vector<struct OpenApiCommandInfo> cmds, tmp_cmds;
	Json::Value obj_subs;
	int new_device_id;
	std::string str_dev_id;

	if (obj.isMember("DeviceID") == false) {
		tracee("there is no DeviceID");
		return cmds;
	}
	str_dev_id = obj["DeviceID"].asString();
	new_device_id = hex_str2int(str_dev_id);

	if (obj.isMember("SubDeviceList") == false) {
		tracee("there is no SubDeviceList");
		return cmds;
	}

	obj_subs = obj["SubDeviceList"];
	if (obj_subs.type() != Json::arrayValue) {
		tracee("SubDeviceList is not array");
		return cmds;
	}

	for (unsigned int i = 0; i < obj_subs.size(); i++) {
		tmp_cmds = parseOpenApiCommand_device_group(new_device_id, 0xff, obj_subs[i]);
		cmds.insert(cmds.end(), tmp_cmds.begin(), tmp_cmds.end());
	}

	return cmds;
}

std::vector<struct OpenApiCommandInfo> HomeDevice::parseOpenApiCommand(int device_id, int sub_id, Json::Value obj)
{
	std::vector<struct OpenApiCommandInfo> cmds, tmp_cmds;
	struct OpenApiCommandInfo cmd;
	Json::Value obj_devices, obj_subs, obj_controls;

	if (obj.isMember("DeviceList") == true) {
		obj_devices = obj["DeviceList"];
		for (unsigned int i = 0; i < obj_devices.size(); i++) {
			tmp_cmds = parseOpenApiCommand_deviceList(device_id, sub_id, obj_devices[i]);
			cmds.insert(cmds.end(), tmp_cmds.begin(), tmp_cmds.end());
		}
	}
	else if (obj.isMember("SubDeviceList") == true) {
		obj_subs = obj["SubDeviceList"];
		for (unsigned int i = 0; i < obj_subs.size(); i++) {
			tmp_cmds = parseOpenApiCommand_device_group(device_id, sub_id, obj_subs[i]);
			cmds.insert(cmds.end(), tmp_cmds.begin(), tmp_cmds.end());
		}
	}
	else if (obj.isMember("Control") == true) {
		obj_controls = obj["Control"];
		for (unsigned int i = 0; i < obj_controls.size(); i++) {
			cmd.m_device_id = device_id;
			cmd.m_sub_id = sub_id;
			cmd.m_obj = obj_controls[i];

			cmds.push_back(cmd);
		}
	}

	return cmds;
}

int HomeDevice::serializeOpenApiResponse_device_group(Json::Value& obj_ret, int sub_id, Json::Value obj)
{
	Json::Value obj_tmp;
	std::string ret_str_id, str_id;
	int ret_int_id, int_id;
	bool is_found;

	for (unsigned int i = 0; i < obj.size(); i++) {
		is_found = false;

		str_id = obj[i]["SubDeviceID"].asString();
		int_id = hex_str2int(str_id);

		for (unsigned int j = 0; j < obj_ret.size(); j++) {
			ret_str_id = obj_ret[j]["SubDeviceID"].asString();
			ret_int_id = hex_str2int(ret_str_id);

			if (int_id == ret_int_id) {
				for (unsigned int k = 0; k < obj[i]["Control"].size(); k++) {
					obj_ret[j]["Control"].append(obj[i]["Control"][k]);
				}
				is_found = true;
				break;
			}
		}

		if (is_found == false) {
			obj_tmp.clear();
			obj_tmp["SubDeviceID"] = str_id;
			for (unsigned int j = 0; j < obj[i]["Control"].size(); j++) {
				obj_tmp["Control"].append(obj[i]["Control"][j]);
			}
			obj_ret.append(obj_tmp);
		}
	}

	return 0;
}

int HomeDevice::serializeOpenApiResponse_deviceList(Json::Value& obj_ret, int device_id, int sub_id, Json::Value obj)
{
	Json::Value obj_tmp;
	std::string str_id;
	int int_id;
	bool is_found;

	is_found = false;
	for (unsigned int i = 0; i < obj_ret.size(); i++) {
		str_id = obj_ret[i]["DeviceID"].asString();
		int_id = hex_str2int(str_id);

		if (device_id == int_id) {
			serializeOpenApiResponse_device_group(obj_ret[i]["SubDeviceList"], sub_id, obj);
			is_found = true;
			break;
		}
	}

	if (is_found == false) {
		obj_tmp["DeviceID"] = int2hex_str(device_id);
		obj_tmp["SubDeviceList"] = Json::Value(Json::arrayValue);
		serializeOpenApiResponse_device_group(obj_tmp["SubDeviceList"], sub_id, obj);
		obj_ret.append(obj_tmp);
	}

	trace("make res: \n%s \n%x \n%s", obj_ret.toStyledString().c_str(), sub_id, obj.toStyledString().c_str());

	return 0;
}

std::string HomeDevice::serializeOpenApiResponse(std::vector<struct OpenApiCommandInfo> responses, Json::Value obj_req)
{
	Json::Value obj_ret, controls, obj_tmp;

	if (obj_req.isMember("DeviceList") == true) {
		obj_ret["DeviceList"] = Json::Value(Json::arrayValue);
		for (unsigned int i = 0; i < responses.size(); i++) {
			serializeOpenApiResponse_deviceList(obj_ret["DeviceList"], responses[i].m_device_id, responses[i].m_sub_id, responses[i].m_obj);
		}
	}
	else if (obj_req.isMember("SubDeviceList") == true) {
		obj_ret["SubDeviceList"] = Json::Value(Json::arrayValue);
		for (unsigned int i = 0; i < responses.size(); i++) {
			serializeOpenApiResponse_device_group(obj_ret["SubDeviceList"], responses[i].m_sub_id, responses[i].m_obj);
		}
	}
	else {
		for (unsigned int i = 0; i < responses.size(); i++) {
			if ((responses[i].m_sub_id & 0x0f) == 0x0f) {
				obj_ret["SubDeviceList"] = Json::Value(Json::arrayValue);
				for (unsigned int j = 0; j < responses.size(); j++) {
					serializeOpenApiResponse_device_group(obj_ret["SubDeviceList"], responses[j].m_sub_id, responses[j].m_obj);
				}
			}
			else {
				for (unsigned int j = 0; j < responses[i].m_obj.size(); j++) {
					for (unsigned int k = 0; k < responses[i].m_obj[j]["Control"].size(); k++) {
						obj_ret["Control"].append(responses[i].m_obj[j]["Control"][k]);
					}
				}
			}
		}
	}

	return obj_ret.toStyledString();
}

bool HomeDevice::recvHttpRequest(HttpRequest* request, HttpResponse* response)
{
	Json::Value json_obj, json_obj2;
	std::string err;
	std::string res_str;

	std::string method;
	std::string org_path;
	std::string mime_type;
	int len;
	char* body;

	std::string json_str;
	std::string filepath;
	std::string mime;

	size_t pos;
	std::string tmp_str;
	std::vector<std::string> paths;

	std::vector<struct OpenApiCommandInfo> openapi_cmds;
	std::vector<struct OpenApiCommandInfo> openapi_responses;
	struct OpenApiCommandInfo openapi_response;
	std::string str_device_id, str_sub_id, str_cmd;
	int device_id, sub_id;

	request->getMethod(&method);
	request->getPath(&org_path);

	trace("get http request: %s, %s\n", method.c_str(), org_path.c_str());

	tmp_str = org_path;
	while ((pos = tmp_str.find("/")) != std::string::npos) {
		if (tmp_str.substr(0, pos).size() == 0) {
			tmp_str.erase(0, 1);
			continue;
		}
		paths.push_back(tmp_str.substr(0, pos));
		tmp_str.erase(0, pos + 1);
	}
	if (tmp_str.size() != 0) {
		paths.push_back(tmp_str);
	}

	if (paths.size() == 0) {
		paths.push_back("smarthome");
		paths.push_back("v1");
		paths.push_back("ui");
	}

	//normal file
	if ((paths.size() < 2) || (paths[0].compare("smarthome") != 0) || (paths[1].compare("v1") != 0)) {
		filepath = m_ui_home_path + org_path;
		trace("get file: %s", filepath.c_str());
		res_str = getFileString(filepath, mime);
		if (res_str.empty() == true) {
			response->setResponseCode(404, "Not Found");
			return true;
		}
		response->setResponseCode(200, "OK");
		response->setBody((uint8_t*)res_str.c_str(), res_str.size(), mime.c_str());

		return true;
	}

	//Open API
	device_id = sub_id = 0xff;
	str_device_id = "0xff";
	str_sub_id = "0xff";

	str_cmd = paths[2];
	if (paths.size() > 3) {
		str_device_id = paths[3];
		device_id = hex_str2int(str_device_id);
	}
	if (paths.size() > 4) {
		str_sub_id = paths[4];
		sub_id = hex_str2int(str_sub_id);
	}
	

	if (method.compare("GET") == 0) {
		if (str_cmd.compare("ui") == 0) {
			if (m_device_id == CONTROLLER_DEVICE_ID) {
				filepath = m_ui_home_path + "/devices/controller.html";
			}
			else {
				filepath = m_ui_home_path + "/devices/ui.html";
			}
			res_str = getFileString(filepath, mime);
			if (res_str.empty() == true) {
				response->setResponseCode(500, "Internal Server Error");
				return true;
			}
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "text/html");
		}
		else if (str_cmd.compare("configuration") == 0) {
			setStatus();
			res_str = m_obj_configuration.toStyledString();
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
		}
		else if (str_cmd.compare("characteristic") == 0) {
			json_obj = getCharacteristic(str_device_id, str_sub_id);
			if (json_obj.type() == Json::nullValue){
				response->setResponseCode(404, "Not Found");
				return true;
			}
			res_str = json_obj.toStyledString();

			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
		}
		else if (str_cmd.compare("status") == 0) {
			setStatus();
			json_obj = getStatus(str_device_id, str_sub_id);
			if (json_obj.type() == Json::nullValue) {
				response->setResponseCode(404, "Not Found");
				return true;
			}
			res_str = json_obj.toStyledString();

			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
		}
		else if(str_cmd.compare("cmdInfo") == 0){
			res_str = m_obj_cmd.toStyledString();
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
		}
		else { 
			filepath = m_ui_home_path + org_path;
			res_str = getFileString(filepath, mime);
			if(res_str.empty() == true){
				response->setResponseCode(404, "Not Found");
				return true;
			}
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), mime.c_str());
		}
	}
	if(method.compare("PUT") == 0){
		if (str_cmd.compare("control") == 0) {
			len = request->getBody((uint8_t**)&body, &mime_type);

			if (parseJson(body, body + len, &json_obj, &err) == false) {
				response->setResponseCode(400, "Bad Request");
				res_str = createErrorResponse(ERROR_CODE_INVALID_MESSAGE).toStyledString();
				response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
				return true;
			}
			trace("body: \n%s\n", json_obj.toStyledString().c_str());

			openapi_responses = std::vector<struct OpenApiCommandInfo>();
			openapi_responses.clear();
			openapi_cmds = parseOpenApiCommand(device_id, sub_id, json_obj);
			trace("after parseOpenApiCommand. num: %d", openapi_cmds.size());

			for (unsigned int i = 0; i < openapi_cmds.size(); i++) {
				/*
				if ((openapi_cmds[i].m_sub_id & 0x0f) == 0x0f) {
					for (unsigned int j = 0; j < m_sub_ids.size(); j++) {
						json_obj2 = Json::Value();
						processHttpCommand(openapi_cmds[i].m_device_id, m_sub_ids[j], openapi_cmds[i].m_obj, json_obj2);
						openapi_response.m_device_id = openapi_cmds[i].m_device_id;
						openapi_response.m_sub_id = m_sub_ids[j];
						openapi_response.m_obj = json_obj2;
						openapi_responses.push_back(openapi_response);
					}
				}
				else {
					json_obj2 = Json::Value();
					processHttpCommand(openapi_cmds[i].m_device_id, openapi_cmds[i].m_sub_id, openapi_cmds[i].m_obj, json_obj2);
					openapi_response.m_device_id = openapi_cmds[i].m_device_id;
					openapi_response.m_sub_id = openapi_cmds[i].m_sub_id;
					openapi_response.m_obj = json_obj2;
					openapi_responses.push_back(openapi_response);
				}
				*/
				json_obj2 = Json::Value();
				processHttpCommand(openapi_cmds[i].m_device_id, openapi_cmds[i].m_sub_id, openapi_cmds[i].m_obj, json_obj2);
				openapi_response.m_device_id = openapi_cmds[i].m_device_id;
				openapi_response.m_sub_id = openapi_cmds[i].m_sub_id;
				openapi_response.m_obj = json_obj2;
				openapi_responses.push_back(openapi_response);
			}
			//res_str = getStatus(str_device_id, str_sub_id).toStyledString();
			res_str = serializeOpenApiResponse(openapi_responses, json_obj);
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");

			trace("control response: \n%s\n", res_str.c_str());
		}
		else {
			response->setResponseCode(404, "Not Found");
			return true;
		}
	}

	return true;
}

void HomeDevice::runSendAdvertisementThread()
{
	int sleep_count;

	sendAdvertisement();

	sleep_count = HTTPU_ADVERTISE_INTERVAL;
	while (true) {
		if (m_run_flag != true) {
			break;
		}
		sleep(1);
		sleep_count++;

		if (sleep_count < HTTPU_ADVERTISE_INTERVAL) {
			continue;
		}

		sleep_count = 0;
		sendAdvertisement();
	}
}

int HomeDevice::sendAdvertisement()
{
	HttpuMessage message;
	Json::Value adv, jobj;
	std::vector<struct UdpSocketInfo> socket_infos;
	int sleep_time;

	socket_infos = m_httpu_server->getSocketInfos();

	jobj["DeviceID"] = int2hex_str(m_device_id);
	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		jobj["DeviceSubIDs"].append(int2hex_str(m_sub_ids[i]));
	}
	jobj["DeviceName"] = m_device_name;

	message.setRequest();
	message.setMethod("POST");
	message.setPath("/advertisement");

	srand((unsigned int)time(NULL));
	for (int i = 0; i < HTTPU_DISCOVERY_NUMBER; i++) {
		for (unsigned int j = 0; j < socket_infos.size(); j++) {
			jobj["BaseURL"] = std::string("http://") +
				socket_infos[j].m_addr +
				std::string(":") +
				std::to_string(m_http_port) +
				std::string("/");

			adv.clear();
			adv["Advertisement"] = jobj;
			message.setBody((uint8_t*)adv.toStyledString().c_str(), adv.toStyledString().length(), std::string("application/json"));

#ifdef __ANDROID__
			m_httpu_server->send(socket_infos[j].m_socket, "255.255.255.255", HTTPU_DISCOVERY_PORT, &message);
#else
			m_httpu_server->send(socket_infos[j].m_socket, HTTPU_DISCOVERY_ADDR, HTTPU_DISCOVERY_PORT, &message);
#endif
		}

		sleep_time = (rand() / RAND_MAX) * 400 + 100;
		usleep(sleep_time);
	}

	return 0;
}

void HomeDevice::runProcessDiscoveryThread()
{
	std::vector<struct DiscoveryResponseHistory> discovery_histories;
	struct DiscoveryResponseHistory history;
	struct DiscoveryRequestInfo dis_req;
	time_t cur_time;
	bool in_history, add_flag;
	int device_id;

	HttpuMessage message;
	Json::Value jobj_dis_res, jobj_dev_list, jobj_devs, jobj_dev, jobj_sub_devs;

	std::string to_addr;
	unsigned short to_port;

	while (true) {
		if (m_run_flag != true) {
			break;
		}
		if (m_discovery_req_q.size() == 0) {
			usleep(1000000);
			continue;
		}

		cur_time = time(NULL);
		dis_req = m_discovery_req_q.front();
		m_discovery_req_q.pop();

		in_history = false;
		//remove old history
		for (unsigned int i = 0; i < discovery_histories.size(); i++) {
			history = discovery_histories[i];
			if (cur_time - history.m_last_sent > 3) {
				discovery_histories.erase(discovery_histories.begin() + i);
				i--;
				continue;
			}
		}
		//check in history
		for (unsigned int i = 0; i < discovery_histories.size(); i++) {
			history = discovery_histories[i];
			if ((history.m_req_info.m_from_addr.sin_addr.s_addr == dis_req.m_from_addr.sin_addr.s_addr) &&
							(history.m_req_info.m_from_addr.sin_port == dis_req.m_from_addr.sin_port) &&
							(history.m_req_info.m_from_interface.m_addr.compare(dis_req.m_from_interface.m_addr) == 0) &&
							(history.m_req_info.m_type.compare(dis_req.m_type) == 0) &&
							(history.m_req_info.m_value.compare(dis_req.m_value) == 0)) {
				in_history = true;
				break;
			}
		}

		//send discovery response
		if (in_history == false) {
			history.m_req_info = dis_req;
			history.m_last_sent = cur_time;
			discovery_histories.push_back(history);

			jobj_devs.clear();
			jobj_dev_list.clear();
			jobj_dis_res.clear();

			//make response
			for (unsigned int i = 0; i < m_devices.size(); i++) {
				add_flag = false;
				if (dis_req.m_type.compare("All") == 0) {
					add_flag = true;
				}
				else if (dis_req.m_type.compare("DeviceID") == 0) {
					device_id = hex_str2int(dis_req.m_value);
					if (device_id == m_devices[i].m_device_id) {
						add_flag = true;
					}
				}
				else if (dis_req.m_type.compare("DeviceName") == 0) {
					if (dis_req.m_value.compare(m_devices[i].m_device_name) == 0) {
						add_flag = true;
					}
				}
				else {
					;
				}

				if (m_device_id == CONTROLLER_DEVICE_ID) {
					if (m_devices[i].m_device_id == CONTROLLER_DEVICE_ID) {
						add_flag = false;
					}
					if ((m_devices[i].m_type & DEVICE_TYPE_SERIAL) == 0) {
						add_flag = false;
					}
				}

				if (add_flag == true) {
					jobj_dev.clear();
					jobj_sub_devs.clear();

					jobj_dev["DeviceID"] = int2hex_str(m_devices[i].m_device_id);
					for (unsigned int j = 0; j < m_devices[i].m_sub_ids.size(); j++) {
						jobj_sub_devs.append(int2hex_str(m_devices[i].m_sub_ids[j]));
					}
					jobj_dev["DeviceSubIDs"] = jobj_sub_devs;
					jobj_dev["DeviceName"] = m_devices[i].m_device_name;
					jobj_dev["BaseURL"] = std::string("http://") + dis_req.m_from_interface.m_addr + std::string(":") + std::to_string(m_http_port) + std::string("/");
					jobj_dev["Alive"] = true;

					jobj_devs.append(jobj_dev);
				}
			}
			jobj_dev_list["DeviceList"] = jobj_devs;
			jobj_dis_res["DiscoveryResponse"] = jobj_dev_list;

			//send
			if (jobj_devs.size() > 0) {
				message.setResponseCode(200, "OK");
				message.setBody((uint8_t*)jobj_dis_res.toStyledString().c_str(), jobj_dis_res.toStyledString().length(), std::string("application/json"));

				to_addr = inet_ntoa(dis_req.m_from_addr.sin_addr);
				to_port = ntohs(dis_req.m_from_addr.sin_port);
				m_httpu_server->send(-1, to_addr.c_str(), to_port, &message);
				trace("send discovery response: %d", jobj_dev_list.size());
			}
		}
	}
}

void HomeDevice::recvHttpuMessage(HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	uint8_t* body;
	std::string mime;
	int len;
	Json::Value jobj, jobj2, jobj3, jobj4;
	std::string path;
	std::string err_msg;
	struct DiscoveryRequestInfo req_info;

	int device_id;
	bool discovery_for_me;

	//trace("in home device recvHttpuMessage...");

	message->getPath(&path);

	if (path.compare("/discovery") == 0) {
		;
	}
	else {
		return;
	}

	len = ((HttpMessage*)message)->getBody(&body, &mime);
	
	//end of test
	if (parseJson((char*)body, (char*)(body + len), &jobj, &err_msg) == false) {
		trace("invalid httpu message: \n%s", body);
		return;
	}
	trace("received httpu: %s\n%s", path.c_str(), jobj.toStyledString().c_str());

	//process discovery
	if (path.compare("/discovery") == 0) {
		//if (m_device_id == CONTROLLER_DEVICE_ID) {
		//	return;
		//}

		jobj2 = jobj["Discovery"];
		if (jobj2.type() != Json::objectValue) {
			trace("invalid httpu message: \n%s", body);
			return;
		}
		
		jobj3 = jobj2["Type"];
		if (jobj3.type() != Json::stringValue) {
			trace("invalid httpu message: \n%s", body);
			return;
		}
		req_info.m_type = jobj3.asString();

		discovery_for_me = false;
		if (jobj3.asString().compare("All") == 0) {
			discovery_for_me = true;
		}
		else if (jobj3.asString().compare("DeviceID") == 0) {
			jobj4 = jobj3["DeviceID"];

			req_info.m_value = jobj4.asString();
			if (jobj4.type() != Json::stringValue) {
				trace("invalid httpu message: \n%s", body);
				return;
			}

			device_id = hex_str2int(jobj4.asString());
			if (m_device_id == device_id) {
				discovery_for_me = true;
			}
		}
		else if (jobj3.asString().compare("DeviceName") == 0) {
			jobj4 = jobj3["DeviceName"];

			req_info.m_value = jobj4.asString();
			if (jobj4.type() != Json::stringValue) {
				trace("invalid httpu message: \n%s", body);
				return;
			}

			if (m_device_name.compare(jobj4.asString()) == 0) {
				discovery_for_me = true;
			}
		}

		if (discovery_for_me == true) {
			req_info.m_from_interface = sock_info;
			req_info.m_from_addr = from_addr;
			m_discovery_req_q.push(req_info);
		}
	}
	else {
		;
	}

	return;
}

int HomeDevice::checkCommandToMe(int device_id, int sub_id)
{
	bool is_found;

	if (device_id == 0xff) {
		;
	}
	else if (device_id != m_device_id) {
		trace("invalid device id: %x", device_id);
		return -1;
	}

	if (sub_id == 0xff) {
		;
	}
	else if ((sub_id & 0xf0) != m_sub_gid) {
		trace("invalid subid: group: %x / %x", sub_id & 0xf0, m_sub_gid);
		return -1;
	}

	if ((sub_id & 0x0f) == 0x0f) {
		;
	}
	else {
		is_found = false;
		for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
			if (sub_id == m_sub_ids[i]) {
				is_found = true;
				break;
			}
		}
		if (is_found == false) {
			std::string tmp_str;
			for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
				if (i != 0) {
					tmp_str += std::string(", ");
				}
				tmp_str += int2hex_str(m_sub_ids[i]);
			}

			trace("invalid subid: device: %x / %x", sub_id, tmp_str.c_str());
			return -1;
		}
	}

	return 0;
}

void HomeDevice::runSerialCommandReceiverThread()
{
	int ret;
	SerialQueueInfo q_info;
	SerialCommandInfo cmd_info;

	while(true){
		if(m_run_flag == false){
			break;
		}

		ret = m_serial_device->get(&q_info);
		if(ret < 0){
#ifdef WIN32
			usleep(100);	//by windows oversleep
#else
			usleep(1000);
#endif
			continue;
		}

		if (HomeDevice::parseSerialCommand(q_info, &cmd_info) < 0) {
			trace("parse serial command failed.");
			std::string str = SerialDevice::printBytes(q_info.m_addr, q_info.m_len);
			tracee("%s", str.c_str());
			//delete[] q_info.m_addr;
			continue;
		}

		if (m_before_process_cmd_callback != NULL) {
			ret = m_before_process_cmd_callback(m_serial_callback_instance, this, &cmd_info);
			if (ret < 0) {
				continue;
			}
		}
		processSerialCommand(cmd_info);
		if (m_after_process_cmd_callback != NULL) {
			ret = m_after_process_cmd_callback(m_serial_callback_instance, this, &cmd_info);
		}
	}

	trace("serial command receiver stoppped.");
}

int HomeDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	if (m_device_id == CONTROLLER_DEVICE_ID) {
		;
	}
	else if (checkCommandToMe(device_id, sub_id) < 0) {
		return -1;
	}

	return 0;
}

int HomeDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (m_device_id == CONTROLLER_DEVICE_ID) {
		;
	}
	else if (checkCommandToMe(cmd_info.m_device_id, cmd_info.m_sub_id) < 0) {
		return -1;
	}
	else {
		std::string bytes_string;
		uint8_t* addr;

		addr = &cmd_info.m_frame[0];
		bytes_string = SerialDevice::printBytes(addr, cmd_info.m_frame.size());
		trace("%s received: %s", m_device_name.c_str(), bytes_string.c_str());
	}

	if (checkChecksum(&cmd_info) == false) {
		tracee("invalid checksum.");
		return -1;
	}

	return 0;
}

int HomeDevice::parseSerialCommand(SerialQueueInfo q_info, SerialCommandInfo* info)
{
	int good_len;
	uint8_t* cmd;
	int len;

	cmd = q_info.m_addr;
	len = q_info.m_len;

	info->m_data.clear();
	info->m_frame.clear();

	for (int i = 0; i < len; i++) {
		info->m_frame.push_back(q_info.m_addr[i]);
	}

	if(len < FRAME_DATALENGTH_IDX + 1){
		std::string bytes_string;
		bytes_string = SerialDevice::printBytes(q_info.m_addr, q_info.m_len);
		tracee("invalid command length: %d, %s", len, bytes_string.c_str());
		return -1;
	}
	good_len = FRAME_HEADER_LENGTH + FRAME_TAIL_LENGTH + cmd[FRAME_DATALENGTH_IDX];
	if(len != good_len){
		tracee("invalid command length: %d / %d", len, good_len);
		return -1;
	}

	info->m_header = cmd[0];
	info->m_device_id = cmd[1];
	info->m_sub_id = cmd[2];
	info->m_command_type = cmd[3];
	info->m_data_length = cmd[4];
	info->m_xor_sum = cmd[len-2];
	info->m_add_sum = cmd[len-1];

	for(int i= FRAME_HEADER_LENGTH; i<len-2; i++){
		info->m_data.push_back(cmd[i]);
	}

	info->m_arrived = q_info.m_arrived;
	info->m_response_delay = 0;
	info->m_serial_byte_delay = q_info.m_serial_byte_delay;

	return 0;
}

bool HomeDevice::checkChecksum(SerialCommandInfo* cmd_info, uint8_t* r_xor_sum, uint8_t* r_add_sum) {
	uint8_t add_sum, xor_sum;
	uint8_t bytes[255];
	std::string str;

	bytes[0] = cmd_info->m_header;
	bytes[1] = cmd_info->m_device_id;
	bytes[2] = cmd_info->m_sub_id;
	bytes[3] = cmd_info->m_command_type;
	bytes[4] = cmd_info->m_data_length;

	for (int i = 0; i < cmd_info->m_data_length; i++) {
		bytes[FRAME_HEADER_LENGTH + i] = cmd_info->m_data[i];
	}
	SerialDevice::getChecksum(bytes, FRAME_HEADER_LENGTH + cmd_info->m_data_length, &xor_sum, &add_sum);

	if ((xor_sum != cmd_info->m_xor_sum) || (add_sum != cmd_info->m_add_sum)) {
		tracee("invalid checksum: %x:%x, %x:%x", cmd_info->m_xor_sum, xor_sum, cmd_info->m_add_sum, add_sum);
		str = SerialDevice::printBytes(bytes, FRAME_HEADER_LENGTH + cmd_info->m_data_length);
		trace("	- length: %d, %s", FRAME_HEADER_LENGTH + cmd_info->m_data_length, str.c_str());
		return false;
	}

	if (r_xor_sum != NULL) {
		*r_xor_sum = xor_sum;
	}
	if (r_add_sum != NULL) {
		*r_add_sum = add_sum;
	}

	return true;
}

int HomeDevice::sendSerialCommand(uint8_t* bytes, int len)
{
	std::string bytes_string;

	bytes_string = SerialDevice::printBytes(bytes, len);
	trace("%s send: %s", m_device_name.c_str(), bytes_string.c_str());

	return m_serial_device->send(bytes, len);
}

int HomeDevice::sendSerialResponse(SerialCommandInfo* req_info, int res_type, uint8_t* body_bytes, int len)
{
	uint8_t bytes[256];
	uint8_t xor_sum, add_sum;
	struct timeval tv;
	int dsec, dnsec;
	long need_sleep;
	static long interval;

	gettimeofday2(&tv);

	trace("arrive compare: %ld, %ld  -  %ld, %ld", tv.tv_sec, req_info->m_arrived.tv_sec, tv.tv_usec, req_info->m_arrived.tv_usec);
	dsec = tv.tv_sec - req_info->m_arrived.tv_sec;
	dnsec = tv.tv_usec - req_info->m_arrived.tv_usec;
	interval = (dsec * 1000) + (dnsec / 1000);

	need_sleep = (MIN_RESPONSE_DELAY - interval);
	trace("interval: %ld ms,    neep_sleep: %ld ms", interval, need_sleep);
	if (need_sleep > 0) {
#ifdef WIN32
		usleep((need_sleep / 3) * 1);
#else
		usleep((need_sleep/3) * 1000 );
#endif
	}
	trace("after sleep.");

	if (m_before_send_cmd_callback != NULL) {
		if (m_before_send_cmd_callback(m_serial_callback_instance, this, req_info, res_type, body_bytes, len) < 0) {
			return -1;
		}
	}

	memset(bytes, 0, 256);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = req_info->m_device_id;
	bytes[2] = req_info->m_sub_id;
	bytes[3] = res_type;
	bytes[4] = len;

	if(len > 0){
		memcpy(&bytes[5], body_bytes, len);
	}

	SerialDevice::getChecksum(bytes, FRAME_HEADER_LENGTH + len, &xor_sum, &add_sum);

	bytes[FRAME_HEADER_LENGTH + len] = xor_sum;
	bytes[FRAME_HEADER_LENGTH + len + 1] = add_sum;

	return sendSerialCommand(bytes, FRAME_HEADER_LENGTH + len + FRAME_TAIL_LENGTH);
}

std::string HomeDevice::getDeviceNameString(int device_id)
{
	std::string ret;

	ret = "unknown";

	switch(device_id){
	case CONTROLLER_DEVICE_ID:
		ret = "Controller";
		break;
	case FORWARDER_DEVICE_ID:
		ret = "Forwarder";
		break;
	case SYSTEMAIRCON_DEVICE_ID:
		ret = "SystemAircon";
		break;
	case MICROWAVEOVEN_DEVICE_ID:
		ret = "MicrowaveOven";
		break;
	case DISHWASHER_DEVICE_ID:
		ret = "DishWasher";
		break;
	case DRUMWASHER_DEVICE_ID:
		ret = "DrumWasher";
		break;
	case LIGHT_DEVICE_ID:
		ret = "Light";
		break;
	case GASVALVE_DEVICE_ID:
		ret = "GasValve";
		break;
	case CURTAIN_DEVICE_ID:
		ret = "Curtain";
		break;
	case REMOTEINSPECTOR_DEVICE_ID:
		ret = "RemoteInspector";
		break;
	case DOORLOCK_DEVICE_ID:
		ret = "DoorLock";
		break;
	case VANTILATOR_DEVICE_ID:
		ret = "Vantilator";
		break;
	case BREAKER_DEVICE_ID:
		ret = "Breaker";
		break;
	case PREVENTCRIMEEXT_DEVICE_ID:
		ret = "PreventCrimeExt";
		break;
	case BOILER_DEVICE_ID:
		ret = "Boiler";
		break;
	case TEMPERATURECONTROLLER_DEVICE_ID:
		ret = "TemperatureController";
		break;
	case ZIGBEE_DEVICE_ID:
		ret = "Zigbee";
		break;
	case POWERMETER_DEVICE_ID:
		ret = "PowerMeter";
		break;
	case POWERGATE_DEVICE_ID:
		ret = "PowerGate";
		break;
	case PHONE_DEVICE_ID:
		ret = "Phone";
		break;
	case ENTRANCE_DEVICE_ID:
		ret = "Entrance";
		break;
	default:
		break;
	}

	return ret;
}

int HomeDevice::extractParametersByCommandInfo(int device_id, Json::Value cmd, Json::Value& extracted_params)
{
	Json::Value cmd_info;
	Json::Value controls, control, params, param, extracted_param;
	
	std::string cmd_type;
	Json::Value parameters, parameter;
	std::vector<int> subids;

	bool is_found;

	if (cmd["CommandType"] == Json::nullValue) {
		tracee("there is no CommandType field.");
		return -1;
	}
	cmd_type = cmd["CommandType"].asString();

	parameters = cmd["Parameters"];
	if (parameters == Json::nullValue) {
		tracee("there is no Parameters field.");
		return -1;
	}

	subids.push_back(0x11);
	cmd_info = generateCmdInfo(device_id, subids);
	controls = cmd_info["CommandInformation"];
	is_found = false;

	for (unsigned int i = 0; i < controls.size(); i++) {
		control = controls[i];
		if (control["CommandType"].asString().compare(cmd_type) == 0) {
			is_found = true;
			break;
		}
	}
	if (is_found == false) {
		tracee("there is no command: %s", cmd_type.c_str());
		return -1;
	}

	extracted_params.clear();
	params = control["Parameters"];
	for (unsigned int i = 0; i < params.size(); i++) {
		param = params[i];

		is_found = false;
		for (unsigned int j = 0; j < parameters.size(); j++) {
			parameter = parameters[j];

			if (param["Name"].asString().compare(parameter["Name"].asString()) == 0) {
				if (param["Type"].asString().compare(parameter["Type"].asString()) != 0) {
					trace("invalid data type: %s / %s", param["Type"].asString().c_str(), parameter["Type"].asString().c_str());
					extracted_params.clear();
					return -1;
				}
				extracted_params[parameter["Name"].asString()] = parameter;
				//extracted_params.append(parameter);
				is_found = true;
				break;
			}
		}
		if (is_found == false) {
			tracee("there is no parameter: %s", param["Name"].asString().c_str());
			extracted_params.clear();
			return -1;
		}
	}

	trace("ret_params: \n%s", extracted_params.toStyledString().c_str());

	return 0;
}

Json::Value HomeDevice::generateCmdInfo(int device_id, std::vector<int> sub_ids)
{
	Json::Value obj_control_infos, obj_control_info, obj_control_cmd, obj_control_params, obj_control_param, obj_control_targets;
	Json::Value obj_cmd;
	Json::Value obj_controlls, obj_control, obj_params, obj_param, obj_sub_ids;
	int sub_gid;
	int tmp_sub_id;
	std::string control_info_filename;
	std::string err_str, cmd_type, target;
	std::vector<std::string> cmd_types;
	std::string target_dev_name;

	target_dev_name = getDeviceNameString(device_id);

	sub_gid = sub_ids[0] & 0xf0;

	obj_cmd["DeviceID"] = device_id;
	obj_cmd["CommandInformation"] = Json::arrayValue;

	control_info_filename = m_data_home_path + std::string("/") + std::string("control_info.json");
	if (parseJson(control_info_filename, &obj_control_infos, &err_str) == false) {
		tracee("open or parse file failed: %s", control_info_filename.c_str());
		return obj_cmd;
	}

	if (obj_control_infos.isMember(target_dev_name) == false) {
		tracee("invalid device name or the device has no control info.: %s", target_dev_name.c_str());
		tracee("%s", obj_control_infos.toStyledString().c_str());
		return obj_cmd;
	}
	obj_control_info = obj_control_infos[target_dev_name];

	//get cmd types
	obj_controlls = Json::arrayValue;

	cmd_types = obj_control_info.getMemberNames();
	for (unsigned int i = 0; i < cmd_types.size(); i++) {
		obj_control.clear();
		obj_params.clear();
		obj_sub_ids.clear();

		cmd_type = cmd_types[i];
		obj_control_cmd = obj_control_info[cmd_type];
		trace("obj_control_cmd: %s", obj_control_cmd.toStyledString().c_str());
		obj_control["CommandType"] = obj_control_cmd["RequestCode"].asString();
		obj_control["HumanCommandType"] = cmd_type;

		if (obj_control_cmd.isMember("Target") == false) {
			tracee("there is no 'target' in: %s / %s", target_dev_name.c_str(), cmd_type.c_str());
			continue;
		}
		obj_control_targets = obj_control_cmd["Target"];
		for (unsigned int j = 0; j < obj_control_targets.size(); j++) {
			target = obj_control_targets[j].asString();
			if (target.compare("individual") == 0) {
				for (unsigned int k = 0; k < sub_ids.size(); k++) {
					tmp_sub_id = sub_ids[k];
					obj_sub_ids.append(tmp_sub_id);
				}
			}
			else if (target.compare("group") == 0) {
				obj_sub_ids.append(sub_ids[0] | 0x0f);
			}
			else if (target.compare("all") == 0) {
				obj_sub_ids.append(0xff);
			}
			else {
				tracee("invalid target: %s", target.c_str());
				continue;
			}
		}

		if (obj_control_cmd.isMember("RequestParameters") == false) {
			tracee("there is no 'requestParameters' in: %s / %s", target_dev_name.c_str(), cmd_type.c_str());
			continue;
		}

		//get parameters
		obj_control_params = obj_control_cmd["RequestParameters"];
		for (unsigned int j = 0; j < obj_control_params.size(); j++) {
			obj_control_param = obj_control_params[j];

			if ((obj_control_param.isMember("Name") == false) || (obj_control_param.isMember("Type") == false)) {
				tracee("there is no 'name' or 'type' in parameter: %s / %s", target_dev_name.c_str(), cmd_type.c_str());
				continue;
			}

			obj_param["Name"] = obj_control_param["Name"];
			obj_param["Type"] = obj_control_param["Type"];
			obj_params.append(obj_param);
		}

		obj_control["Parameters"] = obj_params;
		obj_control["SubDeviceIDs"] = obj_sub_ids;




		obj_controlls.append(obj_control);
	}

	obj_cmd["CommandInformation"] = obj_controlls;

	return obj_cmd;
}





