
#include "controller_device.h"
#include "trace.h"
#include "tools.h"
#include "prevent_crime_ext_device.h"
#include "remote_inspector_device.h"

#include <string>
#include <http_message.h>
#include <http_request.h>
#include <http_response.h>
#include <json/json.h>

#include <win_porting.h>

#ifdef WIN32
#else
#include <unistd.h>
#endif
 

void controller_device_httpu_callback(void* instance, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	ControllerDevice* controller_device;

	controller_device = (ControllerDevice*)instance;
	return controller_device->processDiscoveryResponse(message, sock_info, from_addr);
}

ControllerDevice::ControllerDevice() : HomeDevice()
{
	m_store_message = false;
	m_discovery_httpu_server = NULL;
}

ControllerDevice::~ControllerDevice()
{
}

int ControllerDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj;
	Json::Value tmp_json_obj2;
	std::vector<std::string> registered_devices;
	std::string tmp_str;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	ControllerStatus status;
	struct DeviceInformation device_info;
	bool is_fault;

	trace("in controller init.");

	if(HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0){
		tracee("controller init failed.");
		return -1;
	}

	m_store_message = false;

	m_discovery_httpu_server = new HttpuServer();
	if (m_discovery_httpu_server->init(NULL, DISCOVERY_RESPONSE_HTTPU_PORT) < 0) {
		tracee("httpu server for discovery failed.");
		delete m_discovery_httpu_server;
		m_discovery_httpu_server = NULL;
		return -1;
	}
	m_discovery_httpu_server->setCallback(this, controller_device_httpu_callback);

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		status.m_sub_id = m_sub_ids[i];
		m_statuses.push_back(status);
	}

	m_characteristic.m_version = -1;
	m_characteristic.m_company_code = -1;

	if (conf_json[m_device_name].type() == Json::objectValue) {
		tmp_json_obj = conf_json[m_device_name];

		if (tmp_json_obj["Characteristics"].type() == Json::objectValue) {
			tmp_json_obj = tmp_json_obj["Characteristics"];

			if (tmp_json_obj["Version"].type() == Json::intValue) {
				m_characteristic.m_version = tmp_json_obj["Version"].asInt();
				trace("set version: %d, %d", m_characteristic.m_version, tmp_json_obj["Version"].asInt());
			}
			if (tmp_json_obj["CompanyCode"].type() == Json::intValue) {
				m_characteristic.m_company_code = tmp_json_obj["CompanyCode"].asInt();
				trace("set company code: %d, %d", m_characteristic.m_company_code, tmp_json_obj["CompanyCode"].asInt());
			}
		}
	}


	if (conf_json["Devices"].type() != Json::arrayValue) {
		tracee("invalid devices.");
		delete m_discovery_httpu_server;
		m_discovery_httpu_server = NULL;
		return -1;
	}

	tmp_json_obj = conf_json["Devices"];
	for (unsigned int i = 0; i < tmp_json_obj.size(); i++) {
		tmp_str = tmp_json_obj[i].asString();
		registered_devices.push_back(tmp_str);
	}

	for (unsigned int i = 0; i < registered_devices.size(); i++) {
		device_info.m_sub_ids.clear();

		if (conf_json[registered_devices[i]].type() != Json::objectValue) {
			tracee("it is not json object: %s", registered_devices[i].c_str());
			continue;
		}

		tmp_json_obj = conf_json[registered_devices[i]];

		if ((tmp_json_obj.isMember("DeviceID") == false) || (tmp_json_obj.isMember("SubDeviceIDs") == false)) {
			tracee("there is no DeviceID or SubDeviceIDs field.");
			continue;
		}

		if (tmp_json_obj["DeviceID"].type() != Json::intValue) {
			tracee("invalid device id: %s", registered_devices[i].c_str());
			continue;
		}

		device_info.m_device_id = tmp_json_obj["DeviceID"].asInt();
		device_info.m_device_name = getDeviceNameString(device_info.m_device_id);

		if (tmp_json_obj["SubDeviceIDs"].type() != Json::arrayValue) {
			tracee("invalid sub device id: %s", registered_devices[i].c_str());
			continue;
		}
		if (tmp_json_obj["SubDeviceIDs"].size() == 0) {
			tracee("invalid sub device id: %s", registered_devices[i].c_str());
			continue;
		}
		is_fault = false;
		for (unsigned int j = 0; j < tmp_json_obj["SubDeviceIDs"].size(); j++) {
			if (tmp_json_obj["SubDeviceIDs"][j].type() != Json::intValue) {
				is_fault = true;
				break;
			}
			device_info.m_sub_ids.push_back(tmp_json_obj["SubDeviceIDs"][j].asInt());
		}
		if (is_fault == true) {
			tracee("invalid sub device ID");
			continue;
		}
		device_info.m_sub_gid = device_info.m_sub_ids[0] & 0xf0;
		device_info.m_base_url = std::string("");
		device_info.m_type = DEVICE_TYPE_SERIAL;

		trace("add device: %s, 0x%02x, %d", device_info.m_device_name.c_str(), device_info.m_device_id, device_info.m_sub_ids.size());
		m_devices.push_back(device_info);
	}

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		obj_chars.clear();

		obj_char["Name"] = "Version";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_version;
		obj_chars.append(obj_char);

		obj_char["Name"] = "CompanyCode";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_company_code;
		obj_chars.append(obj_char);

		sub_device.clear();
		sub_device["SubDeviceID"] = int2hex_str(m_sub_ids[i]);
		sub_device["Characteristic"] = obj_chars;
		sub_devices.append(sub_device);
	}

	device["DeviceID"] = int2hex_str(m_device_id);
	device["SubDeviceList"] = sub_devices;
	devices.append(device);
	m_obj_characteristic["DeviceList"] = device;

	trace("characteristic: \n%s", m_obj_characteristic.toStyledString().c_str());

	setCmdInfo();
	setStatus();
	finalizeInit();

	return 0;
}

void ControllerDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
	trace("cmd_info: \n%s", m_obj_cmd.toStyledString().c_str());
}

void ControllerDevice::setStatus()
{
	Json::Value devices, device, sub_devices, sub_device, statuses, status;

	HomeDevice::setStatus();

	statuses.clear();
	sub_devices.clear();
	devices.clear();

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		statuses = Json::arrayValue;
		sub_device["SubDeviceID"] = int2hex_str(m_statuses[i].m_sub_id);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);
	}
	device["DeviceID"] = int2hex_str(m_device_id);
	device["SubDeviceList"] = sub_devices;
	devices.append(device);
	m_obj_status["DeviceList"] = devices;

	trace("set status: \n%s", m_obj_status.toStyledString().c_str());
}


Json::Value ControllerDevice::getDeviceList()
{
	Json::Value device;
	Json::Value devices;
	Json::Value ret;

	//for each devices
	for (unsigned int i = 0; i < m_devices.size(); i++) {
		if (m_devices[i].m_device_id == CONTROLLER_DEVICE_ID) {
			continue;
		}

		device["DeviceName"] = m_devices[i].m_device_name;
		device["DeviceID"] = int2hex_str(m_devices[i].m_device_id);
		device["DeviceSubIDs"] = Json::arrayValue;
		for (unsigned int j = 0; j < m_devices[i].m_sub_ids.size(); j++) {
			device["DeviceSubIDs"].append(int2hex_str(m_devices[i].m_sub_ids[j]));
		}
		device["BaseUrl"] = m_devices[i].m_base_url;
		device["Type"] = m_devices[i].m_type;

		devices.append(device);
	}

	ret["DeviceList"] = devices;

	return ret;
}
int ControllerDevice::run()
{
	Json::Value status_obj;
	std::vector<struct DeviceInformation> new_devices;

	trace("run controller device.");

	if (HomeDevice::run() < 0) {
		tracee("home device run failed.");
		return -1;
	}

	trace("registered device number: %d", m_devices.size());
	printDeviceList();
	
	if (m_discovery_httpu_server->run() < 0) {
		tracee("discovery httpu server run failed.");
		return -1;
	}

	sendDiscovery();
	printDeviceList();

	return 0;
}

void ControllerDevice::stop()
{
	HomeDevice::stop();
}

int ControllerDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	return controlDevice(device_id, sub_id, cmd_obj, res_obj);
}

int ControllerDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	trace("in process command...");

	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) == 0) {
		return -1;
	}

	if (m_store_message == true) {
		m_serial_cmd_mutex.lock();
		m_serial_cmds.push_back(cmd_info);
		trace("store cmd.");
		m_serial_cmd_mutex.unlock();
	}
	else {
		trace("skip cmd.");

	}

	return 0;
}

int ControllerDevice::getCommand(int device_id, int sub_id, int cmd_type, SerialCommandInfo* cmd_info)
{
	int ret;
	struct timeval start;
	long sec_diff, msec_diff;
	int resend_count;

	trace("waiting for response cmd: 0x%02x, 0x%02x, 0x%02x", device_id, sub_id, cmd_type);

	ret = -1;
	resend_count = 0;
	m_store_message = true;
	gettimeofday2(&start);

	for (int retry = 0; retry < 2; retry++) {
		for (int try_count = 0; try_count < COMMAND_RESPONSE_TIMEOUT * 10; try_count++) {
			m_serial_cmd_mutex.lock();
			for (unsigned int i = 0; i < m_serial_cmds.size(); i++) {
				if (m_serial_cmds[i].m_device_id != device_id) {
					continue;
				}

				if (m_serial_cmds[i].m_sub_id != sub_id) {
					continue;
				}

				if (m_serial_cmds[i].m_command_type != cmd_type) {
					//trace("not cmd id: %x",cmd_type);
					continue;
				}

				ret = 0;
				*cmd_info = m_serial_cmds[i];

				sec_diff = (cmd_info->m_arrived.tv_sec - start.tv_sec) * 1000;
				msec_diff = (cmd_info->m_arrived.tv_usec - start.tv_usec) / 1000;
				cmd_info->m_response_delay = sec_diff + msec_diff;
				trace("response delay: %ld", cmd_info->m_response_delay);
				//m_serial_cmds.erase(m_serial_cmds.begin() + i);
				break;
			}
			m_serial_cmds.clear();

			m_serial_cmd_mutex.unlock();

			if (ret == 0) {
				break;
			}
			usleep(100000);
		}

		m_store_message = false;

		//check checksum and re-send command once more
		if (ret == 0) {
			if (HomeDevice::checkChecksum(cmd_info) == false) {
				trace("invalid checksum.");
				ret = -1;
				if (resend_count < 1) {
					trace("re-send the request.");
					//HomeDevice::sendSerialCommand(m_last_sent_bytes, m_last_sent_bytes_len);
					sendSerialCommand(m_last_sent_bytes, m_last_sent_bytes_len);
					m_store_message = true;
					resend_count++;
				}
				else {
					trace("resend give-up");
				}
			}
			else {
				break;
			}
		}
		else {
			break;
		}
	}

	m_store_message = false;
	if (ret < 0) {
		trace("not found response cmd: 0x%02x, 0x%02x, 0x%02x", device_id, sub_id, cmd_type);
	}
	else {
		trace("found response cmd: 0x%02x, 0x%02x, 0x%02x", device_id, sub_id, cmd_type);
	}

	return ret;
}

bool ControllerDevice::recvHttpRequest(HttpRequest* request, HttpResponse* response)
{
	std::string res_str;
	std::string err;

	std::string method;
	std::string mime_type;
	std::string org_path;
	//int len;
	//char* body;

	std::string json_str;
	std::string filepath;
	std::string mime;

	size_t pos;
	std::string tmp_str;
	std::vector<std::string> paths;

	Json::Value json_obj;
	bool is_processed;

	std::string str_device_id, str_sub_id, str_cmd;
	int device_id, sub_id;
	std::vector<int> sub_ids;

	request->getMethod(&method);
	request->getPath(&org_path);

	trace("in controller, get http request: %s, %s\n", method.c_str(), org_path.c_str());

	tmp_str = org_path;
	while((pos = tmp_str.find("/")) != std::string::npos){
		if(tmp_str.substr(0, pos).size() == 0){
			tmp_str.erase(0, 1);
			continue;
		}
		paths.push_back(tmp_str.substr(0, pos));
		tmp_str.erase(0, pos + 1);
	}
	if(tmp_str.size() != 0){
		paths.push_back(tmp_str);
	}

	if(paths.size() == 0){
		paths.push_back("smarthome");
		paths.push_back("v1");
		paths.push_back("ui");
	}

	is_processed = false;

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
	trace("device_id: %x, sub_id: %x", device_id, sub_id);

	if(method.compare("GET") == 0){
		if (str_cmd.compare("devicelist") == 0) {
			json_obj = getDeviceList();
			res_str = json_obj.toStyledString();
			if (res_str.empty() == true) {
				response->setResponseCode(500, "Internal Server Error");
				return true;
			}
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
			is_processed = true;
		}
		else if (str_cmd.compare("controllerui") == 0) {
			filepath = m_ui_home_path + "/devices/controller.html";
			res_str = getFileString(filepath, mime);
			if (res_str.empty() == true) {
				response->setResponseCode(500, "Internal Server Error");
				return true;
			}
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "text/html");
		}
		else if (str_cmd.compare("ui") == 0) {
			filepath = m_ui_home_path + "/devices/ui.html";
			res_str = getFileString(filepath, mime);
			if (res_str.empty() == true) {
				response->setResponseCode(500, "Internal Server Error");
				return true;
			}
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "text/html");
		}
		else if (str_cmd.compare("status") == 0) {
			json_obj = getDeviceStatus(device_id, sub_id);
			res_str = json_obj.toStyledString();
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
		}
		else if (str_cmd.compare("characteristic") == 0) {
			json_obj = getDeviceCharacteristic(device_id, sub_id);
			if (json_obj.type() == Json::nullValue) {
				response->setResponseCode(404, "Not Found");
				return true;
			}
			res_str = json_obj.toStyledString();

			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
		}
		else if(str_cmd.compare("cmdInfo") == 0){
			trace("get cmdInfo");
			sub_ids.clear();
			for(unsigned int i=0; i<m_devices.size(); i++){
				//trace("comp device_id: %x : %x", device_id, m_devices[i].m_device_id);
				if(device_id == m_devices[i].m_device_id){
					if ((sub_id & 0x0f) == 0x0f) {
						sub_ids = m_devices[i].m_sub_ids;
					}
					else {
						for (unsigned int j = 0; j < m_devices[i].m_sub_ids.size(); j++) {
							if (sub_id == m_devices[i].m_sub_ids[j]) {
								sub_ids.push_back(sub_id);
							}
						}
					}
				}
			}

			if (sub_ids.size() == 0) {
				trace("device not found: %x", device_id);
				response->setResponseCode(404, "Not Found");
				return true;
			}

			json_obj = generateCmdInfo(device_id, sub_ids);
			res_str = json_obj.toStyledString();
			response->setResponseCode(200, "OK");
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
		}
		else {
			response->setResponseCode(404, "Not Found");
			return true;
		}
		is_processed = true;
	}
	else if(method.compare("POST") == 0){
		/*
		len = request->getBody((uint8_t**)&body, &mime_type);

		if(parseJson(body,  body + len, &json_obj, &err) == false){
			response->setResponseCode(400, "Bad Request");
			res_str = "{\"error\": \"" + err + "\"}";
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
			return true;
		}

		trace("received control: %s", json_obj.toStyledString().c_str());

		if(controlDevice(device_id, sub_id, json_obj) < 0){
			response->setResponseCode(500, "Internal Server Error");
			res_str = "{\"error\": \"" + err + "\"}";
			response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");
			return true;
		}
		json_obj = getDeviceStatus(device_id, sub_id);
		res_str = json_obj.toStyledString();
		response->setResponseCode(200, "OK");
		response->setBody((uint8_t*)res_str.c_str(), res_str.size(), "application/json");

		is_processed = true;
		*/
	}

	if(is_processed == true){
		return true;
	}

	return HomeDevice::recvHttpRequest(request, response);
}

Json::Value ControllerDevice::makeErrorValues(int error_code)
{
	int new_error_code;
	Json::Value values, value;
	std::string error_description;

	new_error_code = error_code;
	switch (error_code) {
	case ERROR_CODE_SUCCESS:
		error_description = std::string("success");
		break;
	case ERROR_CODE_NO_SUCH_DEVICE:
		error_description = std::string("No such device");
		break;
	case ERROR_CODE_INTERNAL_ERROR:
		error_description = std::string("Internal error");
		break;
	case ERROR_CODE_INVALID_MESSAGE:
		error_description = std::string("Invalid message");
		break;
	case ERROR_CODE_INVALID_COMMAND_TYPE:
		error_description = std::string("Invalid command type");
		break;
	case ERROR_CODE_INVALID_PARAMETER:
		error_description = std::string("Invalid parameter");
		break;
	case ERROR_CODE_SEND_FAILED:
		error_description = std::string("Send message failed");
		break;
	case ERROR_CODE_NO_RESPONSE:
		error_description = std::string("No response");
		break;
	default:
		new_error_code = ERROR_CODE_UNKNOWN;
		error_description = std::string("Unknown error");
		break;
	}

	values.clear();

	value["Name"] = "ErrorCode";
	value["Type"] = "integer";
	value["Value"] = new_error_code;
	values.append(value);

	value["Name"] = "ErrorDescription";
	value["Type"] = "string";
	value["Value"] = error_description;
	values.append(value);

	return values;
}


Json::Value ControllerDevice::getDeviceCharacteristic(int device_id, int sub_id)
{
	Json::Value ret, tmp_ret, tmp_individual_ret;
	Json::Value devices, device, sub_devices, sub_device, characteristics, characteristic;
	std::vector<DeviceInformation> device_infos;
	int req_device_id, req_sub_id;
	bool is_found;

	if (device_id == 0xff) {
		device_infos = m_devices;
	}
	else if (sub_id == 0xff) {
		for (unsigned int i = 0; i < m_devices.size(); i++) {
			if (m_devices[i].m_device_id == device_id) {
				device_infos.push_back(m_devices[i]);
			}
		}
	}
	else {
		for (unsigned int i = 0; i < m_devices.size(); i++) {
			if (m_devices[i].m_device_id == device_id) {
				if (((sub_id & 0x0f) == 0x0f) && ((sub_id & 0xf0) == m_devices[i].m_sub_gid)) {
					device_infos.push_back(m_devices[i]);
					break;
				}
				else {
					is_found = false;
					for (unsigned int j = 0; j < m_devices[i].m_sub_ids.size(); j++) {
						if (sub_id == m_devices[i].m_sub_ids[j]) {
							device_infos.push_back(m_devices[i]);
							break;
						}
					}
					if (is_found == true) {
						break;
					}
				}
			}
		}
	}

	devices.clear();
	devices = Json::arrayValue;
	for (unsigned int i = 0; i < device_infos.size(); i++) {
		device.clear();

		is_found = false;
		for (unsigned int j = 0; j < devices.size(); j++) {
			if (hex_str2int(devices[j]["DeviceID"].asString()) == device_infos[i].m_device_id) {
				is_found = true;
			}
		}
		if (is_found == false) {
			device["DeviceID"] = int2hex_str(device_infos[i].m_device_id);
			device["SubDeviceList"] = Json::arrayValue;
			devices.append(device);
		}
	}

	ret.clear();
	for (unsigned int i = 0; i < device_infos.size(); i++) {
		req_device_id = device_id;
		if (req_device_id == 0xff) {
			req_device_id = device_infos[i].m_device_id;
		}

		req_sub_id = sub_id;
		if (req_sub_id == 0xff) {
			req_sub_id = device_infos[i].m_sub_gid | 0x0f;
		}

		trace("get characteristic: %x, %x", req_device_id, req_sub_id);
		tmp_ret = Json::arrayValue;

		switch (req_device_id) {
		case SYSTEMAIRCON_DEVICE_ID:
			tmp_individual_ret = getDeviceCharacteristic_systemAircon(req_device_id, req_sub_id, device_infos[i]);
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					trace("add sub id: %x", device_infos[i].m_sub_ids[j]);
					tmp_individual_ret[0]["SubDeviceID"] = int2hex_str(device_infos[i].m_sub_ids[j]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = tmp_individual_ret;
			}
			break;
		case MICROWAVEOVEN_DEVICE_ID:
			tmp_ret = getDeviceCharacteristic_microwaveOven(req_device_id, req_sub_id, device_infos[i]);
			break;
		case DISHWASHER_DEVICE_ID:
			tmp_ret = getDeviceCharacteristic_dishWasher(req_device_id, req_sub_id, device_infos[i]);
			break;
		case DRUMWASHER_DEVICE_ID:
			tmp_ret = getDeviceCharacteristic_drumWasher(req_device_id, req_sub_id, device_infos[i]);
			break;
		case LIGHT_DEVICE_ID:
			tmp_individual_ret = getDeviceCharacteristic_light(req_device_id, req_sub_id, device_infos[i]);
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					tmp_individual_ret[0]["SubDeviceID"] = int2hex_str(device_infos[i].m_sub_ids[j]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = tmp_individual_ret;
			}
			break;
		case GASVALVE_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceCharacteristic_gasValve(req_device_id, req_sub_id, device_infos[i]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = getDeviceCharacteristic_gasValve(req_device_id, req_sub_id, device_infos[i]);
			}
			break;
		case CURTAIN_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceCharacteristic_curtain(req_device_id, req_sub_id, device_infos[i]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = getDeviceCharacteristic_curtain(req_device_id, req_sub_id, device_infos[i]);
			}
			break;
		case REMOTEINSPECTOR_DEVICE_ID:
			tmp_individual_ret = getDeviceCharacteristic_remoteInspector(req_device_id, req_sub_id, device_infos[i]);
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					tmp_individual_ret[0]["SubDeviceID"] = int2hex_str(device_infos[i].m_sub_ids[j]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = tmp_individual_ret;
			}
			break;
		case DOORLOCK_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceCharacteristic_doorLock(req_device_id, req_sub_id, device_infos[i]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = getDeviceCharacteristic_doorLock(req_device_id, req_sub_id, device_infos[i]);
			}
			break;
		case VANTILATOR_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceCharacteristic_vantilator(req_device_id, req_sub_id, device_infos[i]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = getDeviceCharacteristic_vantilator(req_device_id, req_sub_id, device_infos[i]);
			}
			break;
		case BREAKER_DEVICE_ID:
			tmp_individual_ret = getDeviceCharacteristic_breaker(req_device_id, req_sub_id, device_infos[i]);
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					tmp_individual_ret[0]["SubDeviceID"] = int2hex_str(device_infos[i].m_sub_ids[j]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = tmp_individual_ret;
			}
			break;
		case PREVENTCRIMEEXT_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceCharacteristic_preventCrimeExt(req_device_id, req_sub_id, device_infos[i]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = getDeviceCharacteristic_preventCrimeExt(req_device_id, req_sub_id, device_infos[i]);
				if ((req_sub_id & 0x0f) == 0x0f) {
					for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
						tmp_individual_ret[0]["SubDeviceID"] = int2hex_str(device_infos[i].m_sub_ids[j]);
						tmp_ret.append(tmp_individual_ret[0]);
					}
				}
			}
			break;
		case BOILER_DEVICE_ID:
			tmp_individual_ret = getDeviceCharacteristic_boiler(req_device_id, req_sub_id, device_infos[i]);
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					tmp_individual_ret[0]["SubDeviceID"] = int2hex_str(device_infos[i].m_sub_ids[j]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = tmp_individual_ret;
			}
			break;
		case TEMPERATURECONTROLLER_DEVICE_ID:
			tmp_individual_ret = getDeviceCharacteristic_temperatureController(req_device_id, req_sub_id, device_infos[i]);
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					tmp_individual_ret[0]["SubDeviceID"] = int2hex_str(device_infos[i].m_sub_ids[j]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = tmp_individual_ret;
			}
			break;
		case ZIGBEE_DEVICE_ID:
			tmp_ret = getDeviceCharacteristic_zigbee(req_device_id, req_sub_id, device_infos[i]);
			break;
		case POWERMETER_DEVICE_ID:
			tmp_ret = getDeviceCharacteristic_powerMeter(req_device_id, req_sub_id, device_infos[i]);
			break;
		case POWERGATE_DEVICE_ID:
			tmp_individual_ret = getDeviceCharacteristic_powerGate(req_device_id, req_sub_id, device_infos[i]);
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					tmp_individual_ret[0]["SubDeviceID"] = int2hex_str(device_infos[i].m_sub_ids[j]);
					tmp_ret.append(tmp_individual_ret[0]);
				}
			}
			else {
				tmp_ret = tmp_individual_ret;
			}
			break;
		default:
			tracee("there is no such device: %x", device_id);
			characteristics.clear();
			characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
			sub_device["SubDeviceID"] = int2hex_str(sub_id);
			sub_device["Characteristic"] = characteristics;
			tmp_ret.append(sub_device);
			break;
		}

		trace("tmp_ret: \n%s", tmp_ret.toStyledString().c_str());
		for (unsigned int j = 0; j < devices.size(); j++) {
			if (hex_str2int(devices[j]["DeviceID"].asString()) == device_infos[i].m_device_id) {
				for (unsigned int k = 0; k < tmp_ret.size(); k++) {
					devices[j]["SubDeviceList"].append(tmp_ret[k]);
				}
			}
		}
	}

	if (device_id == 0xff) {
		ret["DeviceList"] = devices;
	}
	else if (sub_id == 0xff) {
		for (unsigned int i = 0; i < devices.size(); i++) {
			if (hex_str2int(devices[i]["DeviceID"].asString()) == device_id) {
				ret["SubDeviceList"] = devices[i]["SubDeviceList"];
				break;
			}
		}
	}
	else {
		if ((sub_id & 0x0f) == 0x0f) {
			for (unsigned int i = 0; i < devices.size(); i++) {
				if (hex_str2int(devices[i]["DeviceID"].asString()) == device_id) {
					ret["SubDeviceList"] = devices[i]["SubDeviceList"];
					break;
				}
			}
		}
		else {
			for (unsigned int i = 0; i < devices.size(); i++) {
				if (hex_str2int(devices[i]["DeviceID"].asString()) == device_id) {
					if (devices[i]["SubDeviceList"].size() > 0) {
						ret["Characteristic"] = devices[i]["SubDeviceList"][0]["Characteristic"];
						break;
					}
				}
			}
		}
	}

	return ret;
}


Json::Value ControllerDevice::getDeviceCharacteristic_systemAircon(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;
	int req_sub_id;
	double tmp_double;
	bool tmp_bool;

	SerialCommandInfo cmd_info;

	req_sub_id = (sub_id & 0xf0) + 0x0f;

	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	characteristic["Name"] = "IndoorUnitNumber";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[2];
	characteristics.append(characteristic);

	tmp_double = cmd_info.m_data[4] & 0x7f;
	cmd_info.m_data[4] & 0x80 ? tmp_double += 0.5 : tmp_double;
	characteristic["Name"] = "CoolingMaxTemperature";
	characteristic["Type"] = "integer";
	characteristic["Value"] = tmp_double;
	characteristics.append(characteristic);

	tmp_double = cmd_info.m_data[5] & 0x7f;
	tmp_double += cmd_info.m_data[5] & 0x80 ? 0.5 : 0;
	characteristic["Name"] = "CoolingMinTemperature";
	characteristic["Type"] = "integer";
	characteristic["Value"] = tmp_double;
	characteristics.append(characteristic);

	tmp_bool = cmd_info.m_data[3] & 0x10 ? true : false;
	characteristic["Name"] = "DecimalPoint";
	characteristic["Type"] = "boolean";
	characteristic["Value"] = tmp_bool;
	characteristics.append(characteristic);

	sub_device["Characteristic"] = characteristics;
	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);

	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_microwaveOven(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_dishWasher(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_drumWasher(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_light(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	if ((sub_id & 0xf0) == 0) {
		req_sub_id = sub_id;
	}
	else {
		req_sub_id = (sub_id & 0xf0) + 0x0f;
	}
	//just get characteristic. useless
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic.clear();
	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	characteristic["Name"] = "OnOffDeviceNumber";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[2];
	characteristics.append(characteristic);

	sub_device["Characteristic"] = characteristics;
	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);

	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_gasValve(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic.clear();
	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	sub_device["Characteristic"] = characteristics;
	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);

	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_curtain(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		characteristic["Name"] = "ErrorDescription";
		characteristic["Type"] = "string";
		characteristic["Value"] = "Invalid command";
		characteristics.append(characteristic);

		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic.clear();
	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "boolean";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	characteristic["Name"] = "Characteristic";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[2];
	characteristics.append(characteristic);

	sub_device["Characteristic"] = characteristics;
	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);

	sub_devices.append(sub_device);

	return 	sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_remoteInspector(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = 0x0f;
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	characteristic["Name"] = "WaterCurrentLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[2] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "WaterAccumulatedLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[2] & 0x0f);
	characteristics.append(characteristic);

	characteristic["Name"] = "WaterCurrentIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[3] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "WaterAccumulatedIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[3] & 0x0f);
	characteristics.append(characteristic);

	characteristic["Name"] = "GasCurrentLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[4] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "GasAccumulatedLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[4] & 0x0f);
	characteristics.append(characteristic);

	characteristic["Name"] = "GasCurrentIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[5] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "GasAccumulatedIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[5] & 0x0f);
	characteristics.append(characteristic);

	characteristic["Name"] = "ElectricityCurrentLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[6] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "ElectricityAccumulatedLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[6] & 0x0f);
	characteristics.append(characteristic);

	characteristic["Name"] = "ElectricityCurrentIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[7] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "ElectricityAccumulatedIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[7] & 0x0f);
	characteristics.append(characteristic);

	characteristic["Name"] = "HotWaterCurrentLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[8] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "HotWaterAccumulatedLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[8] & 0x0f);
	characteristics.append(characteristic);

	characteristic["Name"] = "HotWaterCurrentIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[9] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "HotWaterAccumulatedIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[9] & 0x0f);
	characteristics.append(characteristic);

	characteristic["Name"] = "HeatCurrentLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[10] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "HeatAccumulatedLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[10] & 0x0f);
	characteristics.append(characteristic);

	characteristic["Name"] = "HeatCurrentIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[11] & 0xf0) >> 4;
	characteristics.append(characteristic);

	characteristic["Name"] = "HeatAccumulatedIntegerLength";
	characteristic["Type"] = "integer";
	characteristic["Value"] = (cmd_info.m_data[11] & 0x0f);
	characteristics.append(characteristic);


	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_doorLock(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value device, sub_devices, sub_device, characteristics, characteristic;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;
	//get charactoristic
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	characteristic["Name"] = "ForcedSetEmergencyOff";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[2];
	characteristics.append(characteristic);


	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_vantilator(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;
	//send to get characteristic
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	characteristic["Name"] = "MaxAirVolume";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[2];
	characteristics.append(characteristic);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_breaker(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value ret;
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;
	//send to get characteristic
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_boiler(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	int req_sub_id;
	SerialCommandInfo cmd_info;
	double tmp_double;

	req_sub_id = (sub_id & 0xf0) + 0x0f;
	//send to get characteristic
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	tmp_double = cmd_info.m_data[3] & 0x7f;
	tmp_double += cmd_info.m_data[3] & 0x80 ? 0.5 : 0;
	characteristic["Name"] = "MaxTemperature";
	characteristic["Type"] = "integer";
	characteristic["Value"] = tmp_double;
	characteristics.append(characteristic);

	tmp_double = cmd_info.m_data[4] & 0x7f;
	tmp_double += cmd_info.m_data[4] & 0x80 ? 0.5 : 0;
	characteristic["Name"] = "MinTemperature";
	characteristic["Type"] = "integer";
	characteristic["Value"] = tmp_double;
	characteristics.append(characteristic);

	characteristic["Name"] = "DecimalPoint";
	characteristic["Type"] = "boolean";
	characteristic["Value"] = cmd_info.m_data[5] & 0x10 ? true : false;
	characteristics.append(characteristic);

	characteristic["Name"] = "OutgoingMode";
	characteristic["Type"] = "boolean";
	characteristic["Value"] = cmd_info.m_data[5] & 0x02 ? true : false;
	characteristics.append(characteristic);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_preventCrimeExt(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;
	Json::Value tmp_obj;
	std::string tmp_string;
	int tmp_value;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;
	//send to get characteristic
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	characteristic["Name"] = "CapableSetType";
	characteristic["Type"] = "booleanarray";
	tmp_obj.clear();
	for (int i = 0; i<PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; i++) {
		tmp_value = cmd_info.m_data[2] & (0x01 << i);
		tmp_obj.append(tmp_value ? true : false);
	}
	characteristic["Value"] = tmp_obj;
	characteristics.append(characteristic);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_temperatureController(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	int req_sub_id;
	SerialCommandInfo cmd_info;
	double tmp_double;

	req_sub_id = (sub_id & 0xf0) + 0x0f;
	//send to get characteristic
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic["Name"] = "Version";
	characteristic["Type"] = "interger";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "interger";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	tmp_double = cmd_info.m_data[3] & 0x7f;
	tmp_double += cmd_info.m_data[3] & 0x80 ? 0.5 : 0;
	characteristic["Name"] = "MaxTemperature";
	characteristic["Type"] = "number";
	characteristic["Value"] = tmp_double;
	characteristics.append(characteristic);

	tmp_double = cmd_info.m_data[4] & 0x7f;
	tmp_double += cmd_info.m_data[4] & 0x80 ? 0.5 : 0;
	characteristic["Name"] = "MinTemperature";
	characteristic["Type"] = "number";
	characteristic["Value"] = tmp_double;
	characteristics.append(characteristic);

	characteristic["Name"] = "DecimalPoint";
	characteristic["Type"] = "boolean";
	characteristic["Value"] = cmd_info.m_data[5] & 0x10 ? true : false;
	characteristics.append(characteristic);

	characteristic["Name"] = "OutgoingMode";
	characteristic["Type"] = "boolean";
	characteristic["Value"] = cmd_info.m_data[5] & 0x02 ? true : false;
	characteristics.append(characteristic);

	characteristic["Name"] = "ControllerNumber";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[6];
	characteristics.append(characteristic);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_zigbee(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_powerMeter(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;

	characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceCharacteristic_powerGate(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, characteristics, characteristic;
	std::string tmp_string;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	if (sub_id & 0xf0) {
		req_sub_id = (sub_id & 0xf0) + 0x0f;
	}
	else {
		req_sub_id = sub_id;
	}
	//send to get characteristic
	if (sendGetDeviceCharacteristic(device_id, req_sub_id) < 0) {
		tracee("send get device characteristic failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x8f, &cmd_info) < 0) {
		tracee("get command response failed.");
		characteristics = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Characteristic"] = characteristics;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	characteristics.clear();

	characteristic["Name"] = "Version";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[0];
	characteristics.append(characteristic);

	characteristic["Name"] = "CompanyCode";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[1];
	characteristics.append(characteristic);

	characteristic["Name"] = "ChannelNumber";
	characteristic["Type"] = "integer";
	characteristic["Value"] = cmd_info.m_data[2];
	characteristics.append(characteristic);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Characteristic"] = characteristics;
	sub_devices.append(sub_device);

	return sub_devices;
}


///////////////////////////////////////////

Json::Value ControllerDevice::getDeviceStatus(int device_id, int sub_id)
{
	Json::Value ret, tmp_ret, tmp_individual_ret;
	Json::Value devices, device, sub_devices, sub_device, statuses, status;
	std::vector<DeviceInformation> device_infos;
	int req_device_id, req_sub_id;
	bool is_found;

	if (device_id == 0xff) {
		device_infos = m_devices;
	}
	else if (sub_id == 0xff) {
		for (unsigned int i = 0; i < m_devices.size(); i++) {
			if (m_devices[i].m_device_id == device_id) {
				device_infos.push_back(m_devices[i]);
			}
		}
	}
	else {
		for (unsigned int i = 0; i < m_devices.size(); i++) {
			if (m_devices[i].m_device_id == device_id) {
				if (((sub_id & 0x0f) == 0x0f) && ((sub_id & 0xf0) == m_devices[i].m_sub_gid)) {
					device_infos.push_back(m_devices[i]);
					break;
				}
				else {
					is_found = false;
					for (unsigned int j = 0; j < m_devices[i].m_sub_ids.size(); j++) {
						if (sub_id == m_devices[i].m_sub_ids[j]) {
							device_infos.push_back(m_devices[i]);
							break;
						}
					}
					if (is_found == true) {
						break;
					}
				}
			}
		}
	}

	devices.clear();
	devices = Json::arrayValue;
	for (unsigned int i = 0; i < device_infos.size(); i++) {
		device.clear();

		is_found = false;
		for (unsigned int j = 0; j < devices.size(); j++) {
			if (hex_str2int(devices[j]["DeviceID"].asString()) == device_infos[i].m_device_id) {
				trace("exist.");
				is_found = true;
			}
		}
		if (is_found == false) {
			device["DeviceID"] = int2hex_str(device_infos[i].m_device_id);
			device["SubDeviceList"] = Json::arrayValue;
			devices.append(device);
		}
	}

	for (unsigned int i = 0; i < device_infos.size(); i++) {
		req_device_id = device_id;
		if (req_device_id == 0xff) {
			req_device_id = device_infos[i].m_device_id;
		}

		req_sub_id = sub_id;
		if (req_sub_id == 0xff) {
			req_sub_id = device_infos[i].m_sub_gid | 0x0f;
		}

		trace("get status: %x, %x", req_device_id, req_sub_id);
		tmp_ret = Json::arrayValue;
		switch (req_device_id) {
		case SYSTEMAIRCON_DEVICE_ID:
			tmp_ret = getDeviceStatus_systemAircon(req_device_id, req_sub_id, device_infos[i]);
			break;
		case MICROWAVEOVEN_DEVICE_ID:
			tmp_ret = getDeviceStatus_microwaveOven(req_device_id, req_sub_id, device_infos[i]);
			break;
		case DISHWASHER_DEVICE_ID:
			tmp_ret = getDeviceStatus_dishWasher(req_device_id, req_sub_id, device_infos[i]);
			break;
		case DRUMWASHER_DEVICE_ID:
			tmp_ret = getDeviceStatus_drumWasher(req_device_id, req_sub_id, device_infos[i]);
			break;
		case LIGHT_DEVICE_ID:
			tmp_ret = getDeviceStatus_light(req_device_id, req_sub_id, device_infos[i]);
			break;
		case GASVALVE_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceStatus_gasValve(req_device_id, req_sub_id, device_infos[i]);
					for (unsigned int k = 0; k < tmp_individual_ret.size(); k++) {
						tmp_ret.append(tmp_individual_ret[k]);
					}
				}
			}
			else {
				tmp_ret = getDeviceStatus_gasValve(req_device_id, req_sub_id, device_infos[i]);
			}
			break;
		case CURTAIN_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceStatus_curtain(req_device_id, req_sub_id, device_infos[i]);
					for (unsigned int k = 0; k < tmp_individual_ret.size(); k++) {
						tmp_ret.append(tmp_individual_ret[k]);
					}
				}
			}
			else {
				tmp_ret = getDeviceStatus_curtain(req_device_id, req_sub_id, device_infos[i]);
			}
			break;
		case REMOTEINSPECTOR_DEVICE_ID:
			tmp_ret = getDeviceStatus_remoteInspector(req_device_id, req_sub_id, device_infos[i]);
			break;
		case DOORLOCK_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceStatus_doorLock(req_device_id, req_sub_id, device_infos[i]);
					for (unsigned int k = 0; k < tmp_individual_ret.size(); k++) {
						tmp_ret.append(tmp_individual_ret[k]);
					}
				}
			}
			else {
				tmp_ret = getDeviceStatus_doorLock(req_device_id, req_sub_id, device_infos[i]);
			}
			break;
		case VANTILATOR_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceStatus_vantilator(req_device_id, req_sub_id, device_infos[i]);
					for (unsigned int k = 0; k < tmp_individual_ret.size(); k++) {
						tmp_ret.append(tmp_individual_ret[k]);
					}
				}
			}
			else {
				tmp_ret = getDeviceStatus_vantilator(req_device_id, req_sub_id, device_infos[i]);
			}
			break;
		case BREAKER_DEVICE_ID:
			tmp_ret = getDeviceStatus_breaker(req_device_id, req_sub_id, device_infos[i]);
			break;
		case PREVENTCRIMEEXT_DEVICE_ID:
			if ((req_sub_id & 0x0f) == 0x0f) {
				for (unsigned int j = 0; j < device_infos[i].m_sub_ids.size(); j++) {
					req_sub_id = device_infos[i].m_sub_ids[j];
					tmp_individual_ret = getDeviceStatus_preventCrimeExt(req_device_id, req_sub_id, device_infos[i]);
					for (unsigned int k = 0; k < tmp_individual_ret.size(); k++) {
						tmp_ret.append(tmp_individual_ret[k]);
					}
				}
			}
			else {
				tmp_ret = getDeviceStatus_preventCrimeExt(req_device_id, req_sub_id, device_infos[i]);
			}
			break;
		case BOILER_DEVICE_ID:
			tmp_ret = getDeviceStatus_boiler(req_device_id, req_sub_id, device_infos[i]);
			break;
		case TEMPERATURECONTROLLER_DEVICE_ID:
			tmp_ret = getDeviceStatus_temperatureController(req_device_id, req_sub_id, device_infos[i]);
			break;
		case ZIGBEE_DEVICE_ID:
			tmp_ret = getDeviceStatus_zigbee(req_device_id, req_sub_id, device_infos[i]);
			break;
		case POWERMETER_DEVICE_ID:
			tmp_ret = getDeviceStatus_powerMeter(req_device_id, req_sub_id, device_infos[i]);
			break;
		case POWERGATE_DEVICE_ID:
			tmp_ret = getDeviceStatus_powerGate(req_device_id, req_sub_id, device_infos[i]);
			break;
		default:
			tracee("there is no such device: %x", device_id);
			statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
			sub_device["SubDeviceID"] = int2hex_str(sub_id);
			sub_device["Status"] = statuses;
			tmp_ret.append(sub_device);
			break;
		}

		trace("tmp_ret: \n%s", tmp_ret.toStyledString().c_str());
		for (unsigned int j = 0; j < devices.size(); j++) {
			if (hex_str2int(devices[j]["DeviceID"].asString()) == device_infos[i].m_device_id) {
				for (unsigned int k = 0; k < tmp_ret.size(); k++) {
					devices[j]["SubDeviceList"].append(tmp_ret[k]);
				}
			}
		}
	}

	if (device_id == 0xff) {
		ret["DeviceList"] = devices;
	}
	else if (sub_id == 0xff) {
		for (unsigned int i = 0; i < devices.size(); i++) {
			if (hex_str2int(devices[i]["DeviceID"].asString()) == device_id) {
				ret["SubDeviceList"] = devices[i]["SubDeviceList"];
				break;
			}
		}
	}
	else {
		if ((sub_id & 0x0f) == 0x0f) {
			for (unsigned int i = 0; i < devices.size(); i++) {
				if (hex_str2int(devices[i]["DeviceID"].asString()) == device_id) {
					ret["SubDeviceList"] = devices[i]["SubDeviceList"];
					break;
				}
			}
		}
		else {
			for (unsigned int i = 0; i < devices.size(); i++) {
				if (hex_str2int(devices[i]["DeviceID"].asString()) == device_id) {
					if (devices[i]["SubDeviceList"].size() > 0) {
						ret["Status"] = devices[i]["SubDeviceList"][0]["Status"];
						break;
					}
				}
			}
		}
	}

	return ret;
}   

Json::Value ControllerDevice::getDeviceStatus_systemAircon(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;
	int dev_num, data_dev_num;
	double temperature;
	int req_sub_id;
	std::vector<int> tmp_sub_ids;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;

	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if(getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0){
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if ((sub_id & 0x0f) == 0x0f) {
		tmp_sub_ids = device_info.m_sub_ids;
	}
	else {
		tmp_sub_ids.push_back(sub_id);
	}

	dev_num = tmp_sub_ids.size();
	data_dev_num = cmd_info.m_data.size() / 5;
	if (data_dev_num < dev_num) {
		dev_num = data_dev_num;
	}

	sub_devices.clear();
	for (int i = 0; i < dev_num; i++) {
		statuses.clear();

		status["Name"] = "Power";
		status["Type"] = "boolean";
		status["Value"] = cmd_info.m_data[i*5 + 1] & 0x10 ? true : false;
		statuses.append(status);

		status["Name"] = "WindDir";
		status["Type"] = "integer";
		status["Value"] = (cmd_info.m_data[i * 5 + 2] >> 4);
		statuses.append(status);

		status["Name"] = "WindVol";
		status["Type"] = "integer";
		status["Value"] = cmd_info.m_data[i * 5 + 2] & 0x0f;
		statuses.append(status);

		temperature = cmd_info.m_data[i * 5 + 3] & 0x7f;
		temperature += cmd_info.m_data[i * 5 + 3] & 0x80 ? 0.5 : 0;
		status["Name"] = "SettingTemperature";
		status["Type"] = "number";
		status["Value"] = temperature;
		statuses.append(status);

		temperature = cmd_info.m_data[i * 5 + 4] & 0x7f;
		temperature += cmd_info.m_data[i * 5 + 4] & 0x80 ? 0.5 : 0;
		status["Name"] = "CurrentTemperature";
		status["Type"] = "number";
		status["Value"] = temperature;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(tmp_sub_ids[i]);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);
	}

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_microwaveOven(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Status"] = statuses;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_dishWasher(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Status"] = statuses;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_drumWasher(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Status"] = statuses;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_light(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	int req_sub_id;
	std::vector<int> tmp_sub_ids;
	SerialCommandInfo cmd_info;

	int dev_num, data_dev_num;
	int tmp_byte;

	req_sub_id = sub_id;

	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if(getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0){
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	statuses.clear();

	if ((sub_id & 0x0f) == 0x0f) {
		tmp_sub_ids = device_info.m_sub_ids;
	}
	else {
		tmp_sub_ids.push_back(sub_id);
	}

	dev_num = tmp_sub_ids.size();
	data_dev_num = cmd_info.m_data.size() - 1;
	if (data_dev_num < dev_num) {
		dev_num = data_dev_num;
	}

	for (int i = 0; i < dev_num; i++) {
		statuses.clear();

		tmp_byte = cmd_info.m_data[i+1];

		status["Name"] = "Power";
		status["Type"] = "boolean";
		status["Value"] = tmp_byte & 0x01 ? true : false;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(tmp_sub_ids[i]);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);
	}

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_gasValve(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;

	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if(getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0){
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	//set status
	sub_devices.clear();
	statuses.clear();

	status["Name"] = "GasLeak";
	status["Type"] = "boolean";
	status["Value"] = cmd_info.m_data[1] & 0x10 ? true : false;
	statuses.append(status);

	status["Name"] = "ExtinguisherBuzzer";
	status["Type"] = "boolean";
	status["Value"] = cmd_info.m_data[1] & 0x08 ? true : false;
	statuses.append(status);

	status["Name"] = "Operating";
	status["Type"] = "boolean";
	status["Value"] = cmd_info.m_data[1] & 0x04 ? true : false;
	statuses.append(status);

	status["Name"] = "Closed";
	status["Type"] = "boolean";
	status["Value"] = cmd_info.m_data[1] & 0x02 ? true : false;
	statuses.append(status);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Status"] = statuses;

	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_curtain(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;

	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if(getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0){
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	statuses.clear();

	status["Name"] = "Status";
	status["Type"] = "integer";
	status["Value"] = cmd_info.m_data[1];
	statuses.append(status);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Status"] = statuses;

	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_remoteInspector(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	int req_sub_id;
	std::vector<int> tmp_sub_ids;
	int dev_num, data_dev_num;
	SerialCommandInfo cmd_info;
	int ival1, ival2;
	double cur_value, acc_value;

	req_sub_id = sub_id;

	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if(getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0){
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if ((sub_id & 0x0f) == 0x0f) {
		tmp_sub_ids = device_info.m_sub_ids;
	}
	else {
		tmp_sub_ids.push_back(sub_id);
	}

	dev_num = tmp_sub_ids.size();
	data_dev_num = (cmd_info.m_data.size() - 1) / 6;
	if (data_dev_num < dev_num) {
		dev_num = data_dev_num;
	}

	sub_devices.clear();
	for (int i = 0; i < dev_num; i++) {
		statuses.clear();

		ival1 = ival2 = 0;

		ival1 += ((cmd_info.m_data[i*6 + 1] & 0xf0) >> 4) * 100000;
		ival1 += (cmd_info.m_data[i*6 + 1] & 0x0f) * 10000;
		ival1 += ((cmd_info.m_data[i*6 + 2] & 0xf0) >> 4) * 1000;
		ival1 += (cmd_info.m_data[i*6 + 2] & 0x0f) * 100;
		ival1 += ((cmd_info.m_data[i*6 + 3] & 0xf0) >> 4) * 10;
		ival1 += (cmd_info.m_data[i*6 + 3] & 0x0f) * 1;

		ival2 += ((cmd_info.m_data[i*6 + 4] & 0xf0) >> 4) * 100000;
		ival2 += (cmd_info.m_data[i*6 + 4] & 0x0f) * 10000;
		ival2 += ((cmd_info.m_data[i*6 + 5] & 0xf0) >> 4) * 1000;
		ival2 += (cmd_info.m_data[i*6 + 5] & 0x0f) * 100;
		ival2 += ((cmd_info.m_data[i*6 + 6] & 0xf0) >> 4) * 10;
		ival2 += (cmd_info.m_data[i*6 + 6] & 0x0f) * 1;

		status.clear();
		if(i == 2){//electricity
			cur_value = ival1;
			acc_value = (double)ival2 / 10;
		}
		else if(i == 4){//heat
			cur_value = (double)ival1 / 1000;
			acc_value = (double)ival2 / 100;
		}
		else{
			cur_value = (double)ival1 / 1000;
			acc_value = (double)ival2 / 10;
		}

		status["Name"] = "CurrentValue";
		status["Type"] = "number";
		status["Value"] = cur_value;
		statuses.append(status);

		status["Name"] = "AccumulatedValue";
		status["Type"] = "number";
		status["Value"] = acc_value;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(tmp_sub_ids[i]);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);
	}

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_doorLock(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;

	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if(getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0){
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	statuses.clear();

	status["Name"] = "Emergency";
	status["Type"] = "integer";
	status["Value"] = cmd_info.m_data[1] & 0x02 ? true : false;
	statuses.append(status);

	status["Name"] = "Opened";
	status["Type"] = "integer";
	status["Value"] = cmd_info.m_data[1] & 0x01 ? true : false;
	statuses.append(status);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Status"] = statuses;

	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_vantilator(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	//get status
	req_sub_id = sub_id;
	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if(getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0){
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	statuses.clear();

	status["Name"] = "Error";
	status["Type"] = "integer";
	status["Value"] = cmd_info.m_data[0];
	statuses.append(status);

	status["Name"] = "Power";
	status["Type"] = "integer";
	status["Value"] = cmd_info.m_data[1];
	statuses.append(status);

	status["Name"] = "AirVolume";
	status["Type"] = "integer";
	status["Value"] = cmd_info.m_data[2];
	statuses.append(status);

	sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
	sub_device["Status"] = statuses;

	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_breaker(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value ret;
	Json::Value sub_devices, sub_device, statuses, status;

	int req_sub_id;
	std::vector<int> tmp_sub_ids;
	SerialCommandInfo cmd_info;

	if ((sub_id & 0x0f) == 0x0f) {
		tmp_sub_ids = device_info.m_sub_ids;
	}
	else {
		tmp_sub_ids.push_back(sub_id);
	}

	sub_devices.clear();
	for (unsigned int i = 0; i < tmp_sub_ids.size(); i++) {
		//get status
		req_sub_id = tmp_sub_ids[i];

		status.clear();
		statuses.clear();
		sub_device.clear();

		if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
			tracee("send get device status failed.");
			statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
			sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
			sub_device["Status"] = statuses;
			sub_devices.append(sub_device);
			continue;
		}

		if (getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0) {
			tracee("get command response failed.");
			statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
			sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
			sub_device["Status"] = statuses;
			sub_devices.append(sub_device);
			continue;
		}

		status["Name"] = "LightRelayClosed";
		status["Type"] = "boolean";
		status["Value"] = cmd_info.m_data[1] & 0x04 ? true : false;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);
	}

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_boiler(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;
	double tmp_double;

	int req_sub_id;
	std::vector<int> tmp_sub_ids;
	SerialCommandInfo cmd_info;

	if ((sub_id & 0x0f) == 0x0f) {
		tmp_sub_ids = device_info.m_sub_ids;
	}
	else {
		tmp_sub_ids.push_back(sub_id);
	}

	sub_devices.clear();
	for (unsigned int i = 0; i < tmp_sub_ids.size(); i++) {
		//get status
		req_sub_id = tmp_sub_ids[i];

		status.clear();
		statuses.clear();
		sub_device.clear();

		if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
			tracee("send get device status failed.");
			statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
			sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
			sub_device["Status"] = statuses;
			sub_devices.append(sub_device);
			continue;
		}

		if (getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0) {
			tracee("get command response failed.");
			statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
			sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
			sub_device["Status"] = statuses;
			sub_devices.append(sub_device);
			continue;
		}

		status["Name"] = "Error";
		status["Type"] = "integer";
		status["Value"] = cmd_info.m_data[0];
		statuses.append(status);

		status["Name"] = "Outgoing";
		status["Type"] = "boolean";
		status["Value"] = cmd_info.m_data[1] & 0x02 ? true : false;
		statuses.append(status);

		status["Name"] = "Heating";
		status["Type"] = "boolean";
		status["Value"] = cmd_info.m_data[1] & 0x01 ? true : false;
		statuses.append(status);

		tmp_double = cmd_info.m_data[2] & 0x7f;
		tmp_double += cmd_info.m_data[2] & 0x80 ? 0.5 : 0;
		status["Name"] = "SettingTemperature";
		status["Type"] = "number";
		status["Value"] = tmp_double;
		statuses.append(status);

		tmp_double = cmd_info.m_data[3] & 0x7f;
		tmp_double += cmd_info.m_data[3] & 0x80 ? 0.5 : 0;
		status["Name"] = "CurrentTemperature";
		status["Type"] = "number";
		status["Value"] = tmp_double;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);
	}

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_preventCrimeExt(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;
	Json::Value tmp_obj;
	std::string tmp_string;
	int tmp_int;

	int req_sub_id;
	SerialCommandInfo cmd_info;

	//get status
	req_sub_id = sub_id;

	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0) {
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	sub_devices.clear();
	statuses.clear();

	status["Name"] = "SensorSet";
	status["Type"] = "booleanarray";
	tmp_obj.clear();
	for (int i = 0; i<PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; i++) {
		tmp_int = cmd_info.m_data[1] & (0x01 << i);
		tmp_obj.append(tmp_int ? true : false);
	}
	status["Value"] = tmp_obj;
	statuses.append(status);

	status["Name"] = "SensorType";
	status["Type"] = "integerarray";
	tmp_obj.clear();
	for (int i = 0; i<PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; i++) {
		if (i>3) {
			tmp_int = cmd_info.m_data[2] & (0x03 << ((i - 4) * 2));
			tmp_int = tmp_int >> ((i - 4) * 2);
		}
		else {
			tmp_int = cmd_info.m_data[3] & (0x03 << (i * 2));
			tmp_int = tmp_int >> (i * 2);
		}
		tmp_obj.append(tmp_int);
	}
	status["Value"] = tmp_obj;
	statuses.append(status);

	status["Name"] = "SensorStatus";
	status["Type"] = "integerarray";
	tmp_obj.clear();
	for (int i = 0; i<PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; i++) {
		if (i>3) {
			tmp_int = cmd_info.m_data[4] & (0x03 << ((i - 4) * 2));
			tmp_int = tmp_int >> ((i - 4) * 2);
		}
		else {
			tmp_int = cmd_info.m_data[5] & (0x03 << (i * 2));
			tmp_int = tmp_int >> (i * 2);
		}
		tmp_obj.append(tmp_int);
	}
	status["Value"] = tmp_obj;
	statuses.append(status);

	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Status"] = statuses;

	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_temperatureController(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;
	double tmp_double;

	int req_sub_id;
	std::vector<int> tmp_sub_ids;
	int dev_num, data_dev_num;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;

	//get status
	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0) {
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if ((sub_id & 0x0f) == 0x0f) {
		tmp_sub_ids = device_info.m_sub_ids;
	}
	else {
		tmp_sub_ids.push_back(sub_id);
	}

	dev_num = tmp_sub_ids.size();
	data_dev_num = (cmd_info.m_data.size() - 5) / 2;
	if (data_dev_num < dev_num) {
		dev_num = data_dev_num;
	}

	sub_devices.clear();
	for (int i = 0; i < dev_num; i++) {
		statuses.clear();

		status["Name"] = "Error";
		status["Type"] = "integer";
		status["Value"] = cmd_info.m_data[0];
		statuses.append(status);

		status["Name"] = "Heating";
		status["Type"] = "boolean";
		status["Value"] = cmd_info.m_data[1] & 0x02 ? true : false;
		statuses.append(status);

		status["Name"] = "Outgoing";
		status["Type"] = "boolean";
		status["Value"] = cmd_info.m_data[2] & 0x02 ? true : false;
		statuses.append(status);

		status["Name"] = "Reservation";
		status["Type"] = "boolean";
		status["Value"] = cmd_info.m_data[3] & 0x01 ? true : false;
		statuses.append(status);

		status["Name"] = "HotwaterExclusive";
		status["Type"] = "boolean";
		status["Value"] = cmd_info.m_data[4] & 0x01 ? true : false;
		statuses.append(status);

		tmp_double = cmd_info.m_data[5 + i*2] & 0x7f;
		tmp_double += cmd_info.m_data[5 + i*2] & 0x80 ? 0.5 : 0;
		status["Name"] = "SettingTemperature";
		status["Type"] = "number";
		status["Value"] = tmp_double;
		statuses.append(status);

		tmp_double = cmd_info.m_data[5 + i*2 + 1] & 0x7f;
		tmp_double += cmd_info.m_data[5 + i*2 + 1] & 0x80 ? 0.5 : 0;
		status["Name"] = "CurrentTemperature";
		status["Type"] = "number";
		status["Value"] = tmp_double;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(tmp_sub_ids[i]);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);
	}

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_zigbee(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Status"] = statuses;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_powerMeter(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;

	statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Status"] = statuses;
	sub_devices.append(sub_device);

	return sub_devices;
}

Json::Value ControllerDevice::getDeviceStatus_powerGate(int device_id, int sub_id, DeviceInformation device_info)
{
	Json::Value sub_devices, sub_device, statuses, status;
	int dev_num, data_dev_num;
	double dvalue, dvalue_point;
	std::string tmp_string;

	int req_sub_id;
	std::vector<int> tmp_sub_ids;
	SerialCommandInfo cmd_info;

	req_sub_id = sub_id;
	//get status
	if (sendGetDeviceStatus(device_id, req_sub_id) < 0) {
		tracee("send get device status failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if (getCommand(device_id, req_sub_id, 0x81, &cmd_info) < 0) {
		tracee("get command response failed.");
		statuses = makeErrorValues(ERROR_CODE_NO_RESPONSE);
		sub_device["SubDeviceID"] = int2hex_str(req_sub_id);
		sub_device["Status"] = statuses;
		sub_devices.append(sub_device);
		return sub_devices;
	}

	if ((sub_id & 0x0f) == 0x0f) {
		tmp_sub_ids = device_info.m_sub_ids;
	}
	else {
		tmp_sub_ids.push_back(sub_id);
	}

	dev_num = tmp_sub_ids.size();
	data_dev_num = (cmd_info.m_data.size() - 1) / 5;
	if (data_dev_num < dev_num) {
		dev_num = data_dev_num;
	}

	sub_devices.clear();
	for (int i = 0; i < dev_num; i++) {
		statuses.clear();

		status["Name"] = "Power";
		status["Type"] = "boolean";
		status["Value"] = cmd_info.m_data[i * 5 + 1] & 0x10 ? true : false;
		statuses.append(status);

		dvalue = 0;
		dvalue += (cmd_info.m_data[i * 5 + 1] & 0x0f) * 1000;
		dvalue += ((cmd_info.m_data[i * 5 + 2] & 0xf0) >> 4) * 100;
		dvalue += (cmd_info.m_data[i * 5 + 2] & 0x0f) * 10;
		dvalue += (cmd_info.m_data[i * 5 + 3] & 0xf0) >> 4;
		dvalue_point = cmd_info.m_data[i * 5 + 3] & 0x0f;
		dvalue += dvalue_point / 10;
		status["Name"] = "PowerMeasurement";
		status["Type"] = "number";
		status["Value"] = dvalue;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(tmp_sub_ids[i]);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);
	}

	return sub_devices;
}

int ControllerDevice::controlDevice(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	int ret;

	std::string cmd;
	Json::Value params, tmp_res_obj;
	Json::Value sub_device, controlls, control, params2, param;

	bool is_found;
	std::vector< DeviceInformation> device_infos;

	if(cmd_obj["CommandType"].type() != Json::stringValue){
		tracee("there is no command: %s", cmd_obj.toStyledString().c_str());
		return -1;
	}
	if(cmd_obj["Parameters"].type() != Json::arrayValue){
		tracee("there is no params: %s", cmd_obj.toStyledString().c_str());
		return -1;
	}

	cmd = cmd_obj["CommandType"].asString();
	params = cmd_obj["Parameters"];

	for (unsigned int i = 0; i < m_devices.size(); i++) {
		if (m_devices[i].m_device_id == device_id) {
			if (((sub_id & 0x0f) == 0x0f) && ((sub_id == 0xff) || ((sub_id & 0xf0) == m_devices[i].m_sub_gid))) {
				device_infos.push_back(m_devices[i]);
				break;
			}
			else {
				is_found = false;
				for (unsigned int j = 0; j < m_devices[i].m_sub_ids.size(); j++) {
					if (sub_id == m_devices[i].m_sub_ids[j]) {
						device_infos.push_back(m_devices[i]);
						break;
					}
				}
				if (is_found == true) {
					break;
				}
			}
		}
	}

	trace("control message: %x, %x: \n%s", device_id, sub_id, cmd_obj.toStyledString().c_str());
	ret = -1;
	res_obj = Json::arrayValue;
	for (unsigned int i = 0; i < device_infos.size(); i++) {
		tmp_res_obj = Json::arrayValue;

		switch (device_id) {
		case SYSTEMAIRCON_DEVICE_ID:
			ret = controlDevice_systemAircon(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
			case MICROWAVEOVEN_DEVICE_ID:
				ret = controlDevice_microwaveOven(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
				break;
			case DISHWASHER_DEVICE_ID:
				ret = controlDevice_dishWasher(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
				break;
			case DRUMWASHER_DEVICE_ID:
				ret = controlDevice_drumWasher(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
				break;
		case LIGHT_DEVICE_ID:
			ret = controlDevice_light(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		case GASVALVE_DEVICE_ID:
			ret = controlDevice_gasValve(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		case CURTAIN_DEVICE_ID:
			ret = controlDevice_curtain(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		case REMOTEINSPECTOR_DEVICE_ID:
			ret = controlDevice_remoteInspector(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		case DOORLOCK_DEVICE_ID:
			ret = controlDevice_doorLock(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		case VANTILATOR_DEVICE_ID:
			ret = controlDevice_vantilator(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		case BREAKER_DEVICE_ID:
			ret = controlDevice_breaker(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		case PREVENTCRIMEEXT_DEVICE_ID:
			ret = controlDevice_preventCrimeExt(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		case BOILER_DEVICE_ID:
			ret = controlDevice_boiler(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		case TEMPERATURECONTROLLER_DEVICE_ID:
			ret = controlDevice_temperatureController(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
			case ZIGBEE_DEVICE_ID:
				ret = controlDevice_zigbee(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
				break;
			case POWERMETER_DEVICE_ID:
				ret = controlDevice_powerMeter(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
				break;
		case POWERGATE_DEVICE_ID:
			ret = controlDevice_powerGate(device_id, sub_id, device_infos[i], cmd, params, tmp_res_obj);
			break;
		default:
			trace("invalid device_id: %x", device_id);
			control["CommandType"] = cmd;
			params2 = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
			control["Parameters"] = params2;
			controlls.append(control);
			sub_device["SubDeviceID"] = int2hex_str(sub_id);
			sub_device["Control"] = controlls;
			tmp_res_obj.append(sub_device);
			ret = 0;
			break;
		}

		if (ret < 0) {
			trace("control failed: %x: %x, %s, %s", device_id, sub_id, cmd.c_str(), params.toStyledString().c_str());
			control["CommandType"] = cmd;
			params2 = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
			control["Parameters"] = params2;
			controlls.append(control);
			sub_device["SubDeviceID"] = int2hex_str(sub_id);
			sub_device["Control"] = controlls;
			tmp_res_obj.append(sub_device);
		}
		else {
			tmp_res_obj = makeControlResponse(device_id, sub_id, device_infos[i], cmd);
		}

		for (unsigned int j = 0; j < tmp_res_obj.size(); j++) {
			res_obj.append(tmp_res_obj[j]);
		}
	}

	return ret; 
}   

Json::Value ControllerDevice::makeControlResponse(int device_id, int sub_id, DeviceInformation device_info, std::string command_type)
{
	Json::Value obj_ret, obj_tmp;
	Json::Value obj_statuses;
	Json::Value obj_control_infos, obj_control_info;
	Json::Value obj_param, obj_params, obj_control, obj_controlls, obj_sub_device;

	std::string control_info_filename, err_str, cmd_type, target;
	std::string target_dev_name;

	std::vector<std::string> cmd_types;

	
	int req_cmd, tmp_cmd;
	std::string str_res_cmd;
	Json::Value obj_control_info_params;

	obj_ret = Json::arrayValue;
	target_dev_name = getDeviceNameString(device_id);

	//read control_info and find device
	control_info_filename = m_data_home_path + std::string("/") + std::string("control_info.json");
	if (parseJson(control_info_filename, &obj_control_infos, &err_str) == false) {
		tracee("open or parse file failed: %s", control_info_filename.c_str());
		obj_control["CommandType"] = command_type;
		obj_params = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
		obj_control["Parameters"] = obj_params;
		obj_controlls.append(obj_control);
		obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
		obj_sub_device["Control"] = obj_controlls;
		obj_ret.append(obj_sub_device);
		return obj_ret;
	}

	if (obj_control_infos.isMember(target_dev_name) == false) {
		tracee("invalid device name or the device has no control info.: %s", target_dev_name.c_str());
		tracee("%s", obj_control_infos.toStyledString().c_str());
		obj_control["CommandType"] = command_type;
		obj_params = makeErrorValues(ERROR_CODE_INTERNAL_ERROR);
		obj_control["Parameters"] = obj_params;
		obj_controlls.append(obj_control);
		obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
		obj_sub_device["Control"] = obj_controlls;
		obj_ret.append(obj_sub_device);
		return obj_ret;
	}
	obj_control_info = obj_control_infos[target_dev_name];

	//get command type
	str_res_cmd.empty();
	cmd_types = obj_control_info.getMemberNames();
	for (unsigned int i = 0; i < cmd_types.size(); i++) {
		//check control info. validation
		if (obj_control_info[cmd_types[i]].isMember("RequestCode") == false) {
			tracee("there is no requestCode field.");
			obj_control["CommandType"] = command_type;
			obj_params = makeErrorValues(ERROR_CODE_INTERNAL_ERROR);
			obj_control["Parameters"] = obj_params;
			obj_controlls.append(obj_control);
			obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
			obj_sub_device["Control"] = obj_controlls;
			obj_ret.append(obj_sub_device);
			return obj_ret;
		}
		else if (obj_control_info[cmd_types[i]]["RequestCode"].type() != Json::stringValue) {
			tracee("invalid requestCode field.");
			obj_control["CommandType"] = command_type;
			obj_params = makeErrorValues(ERROR_CODE_INTERNAL_ERROR);
			obj_control["Parameters"] = obj_params;
			obj_controlls.append(obj_control);
			obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
			obj_sub_device["Control"] = obj_controlls;
			obj_ret.append(obj_sub_device);
			return obj_ret;
		}

		if (obj_control_info[cmd_types[i]].isMember("ResponseCode") == false) {
			tracee("there is no responseCode field.");
			obj_control["CommandType"] = command_type;
			obj_params = makeErrorValues(ERROR_CODE_INTERNAL_ERROR);
			obj_control["Parameters"] = obj_params;
			obj_controlls.append(obj_control);
			obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
			obj_sub_device["Control"] = obj_controlls;
			obj_ret.append(obj_sub_device);
			return obj_ret;
		}
		else if (obj_control_info[cmd_types[i]]["ResponseCode"].type() != Json::stringValue) {
			tracee("invalid responseCode field.");
			obj_control["CommandType"] = command_type;
			obj_params = makeErrorValues(ERROR_CODE_INTERNAL_ERROR);
			obj_control["Parameters"] = obj_params;
			obj_controlls.append(obj_control);
			obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
			obj_sub_device["Control"] = obj_controlls;
			obj_ret.append(obj_sub_device);
			return obj_ret;
		}

		if (obj_control_info[cmd_types[i]].isMember("ResponseParameters") == false) {
			tracee("there is no responseParameters field.");
			obj_control["CommandType"] = command_type;
			obj_params = makeErrorValues(ERROR_CODE_INTERNAL_ERROR);
			obj_control["Parameters"] = obj_params;
			obj_controlls.append(obj_control);
			obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
			obj_sub_device["Control"] = obj_controlls;
			obj_ret.append(obj_sub_device);
			return obj_ret;
		}
		else if (obj_control_info[cmd_types[i]]["ResponseParameters"].type() != Json::arrayValue) {
			tracee("invalid responseParameters field.");
			obj_control["CommandType"] = command_type;
			obj_params = makeErrorValues(ERROR_CODE_INTERNAL_ERROR);
			obj_control["Parameters"] = obj_params;
			obj_controlls.append(obj_control);
			obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
			obj_sub_device["Control"] = obj_controlls;
			obj_ret.append(obj_sub_device);
			return obj_ret;
		}

		//find command_type
		if (cmd_types[i].compare(command_type) == 0) {
			str_res_cmd = obj_control_info[cmd_types[i]]["ResponseCode"].asString();
			obj_control_info_params = obj_control_info[cmd_types[i]]["ResponseParameters"];
			break;
		}
		else {
			tmp_cmd = hex_str2int(obj_control_info[cmd_types[i]]["RequestCode"].asString());
			req_cmd = hex_str2int(command_type);

			if (req_cmd == tmp_cmd) {
				str_res_cmd = obj_control_info[cmd_types[i]]["ResponseCode"].asString();
				obj_control_info_params = obj_control_info[cmd_types[i]]["ResponseParameters"];
				break;
			}
		}
	}

	if (str_res_cmd.empty() == true) {
		obj_control["CommandType"] = command_type;
		obj_params = makeErrorValues(ERROR_CODE_INVALID_COMMAND_TYPE);
		obj_control["Parameters"] = obj_params;
		obj_controlls.append(obj_control);
		obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
		obj_sub_device["Control"] = obj_controlls;
		obj_ret.append(obj_sub_device);
		return obj_ret;
	}

	trace("found parameters: \n%s\n", obj_control_info_params.toStyledString().c_str());

	obj_tmp = getDeviceStatus(device_id, sub_id);
	if (obj_tmp.isMember("DeviceList") == true) {
		if (obj_tmp["DeviceList"].size() == 0) {
			trace("no such device_id: %x", device_id);
			obj_control["CommandType"] = command_type;
			obj_params = makeErrorValues(ERROR_CODE_NO_SUCH_DEVICE);
			obj_control["Parameters"] = obj_params;
			obj_controlls.append(obj_control);
			obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
			obj_sub_device["Control"] = obj_controlls;
			obj_ret.append(obj_sub_device);
			return obj_ret;
		}
		obj_statuses = obj_tmp[0]["SubDeviceList"];
	}
	else if (obj_tmp.isMember("SubDeviceList") == true) {
		obj_statuses = obj_tmp["SubDeviceList"];
	}
	else {
		obj_statuses = Json::arrayValue;
		obj_sub_device["SubDeviceID"] = int2hex_str(sub_id);
		obj_sub_device["Status"] = obj_tmp["Status"];
		obj_statuses.append(obj_sub_device);
	}

	trace("ret statuses: \n%s\n", obj_statuses.toStyledString().c_str());

	for (unsigned int i = 0; i < obj_statuses.size(); i++) {
		obj_sub_device.clear();
		obj_control.clear();
		obj_controlls.clear();
		obj_params.clear();
		obj_param.clear();


		for (unsigned int j = 0; j < obj_statuses[i]["Status"].size(); j++) {
			for (unsigned int k = 0; k < obj_control_info_params.size(); k++) {
				trace("check: %s , %s", obj_control_info_params[k]["Name"].asString().c_str(), obj_statuses[i]["Status"][j]["Name"].asString().c_str());

				if (obj_control_info_params[k]["Name"].asString().compare(obj_statuses[i]["Status"][j]["Name"].asString()) == 0) {
					obj_param["Name"] = obj_statuses[i]["Status"][j]["Name"];
					obj_param["Type"] = obj_statuses[i]["Status"][j]["Type"];
					obj_param["Value"] = obj_statuses[i]["Status"][j]["Value"];
					obj_params.append(obj_param);
				}
			}
		}
		obj_control["CommandType"] = str_res_cmd;
		obj_control["Parameters"] = obj_params;
		obj_controlls.append(obj_control);

		obj_sub_device["SubDeviceID"] = obj_statuses[i]["SubDeviceID"];
		obj_sub_device["Control"] = obj_controlls;
		obj_ret.append(obj_sub_device);
	}
	
	trace("ret control: \n%s\n", obj_ret.toStyledString().c_str());

	return obj_ret;
}

int ControllerDevice::controlDevice_systemAircon(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	SerialCommandInfo cmd_info;
	Json::Value tmp_obj;
	int param_num;
	bool power;
	double double_temperature;
	int int_temperature;

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;
	bytes[4] = 1;

	if (cmd.compare("SetPowerControl") == 0) {
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Power") == 0) {
				power = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x43;
		bytes[5] = power ? 1 : 0;
	}
	else if (cmd.compare("SetTemperatureControl") == 0) {
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("SettingTemperature") == 0) {
				double_temperature = tmp_obj["Value"].asDouble();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x44;

		int_temperature = int(double_temperature);
		if (double_temperature - int_temperature > 0) {
			int_temperature |= 0x80;
		}
		bytes[5] = int_temperature;
	}
	else {
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}
	
	SerialDevice::getChecksum(bytes, 6, &bytes[6], &bytes[7]);
	
	if((sub_id & 0x0f) == 0x0f){
		for(int i=0; i<3; i++){
			if(sendSerialCommand(bytes, 8) < 0){
				tracee("send command failed.");
			}
			usleep(MIN_REQUEST_DELAY * 1000);
		}

	}
	else{
		if(sendSerialCommand(bytes, 8) < 0){
			tracee("send command failed.");
			return -1;
		}

		if(getCommand(device_id, sub_id, bytes[3] | 0x80, &cmd_info) < 0){
			tracee("doesnot receive ack.");
			return -1;
		}
	}

	return 0;
}

int ControllerDevice::controlDevice_microwaveOven(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	return 0;
}

int ControllerDevice::controlDevice_dishWasher(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	return 0;
}

int ControllerDevice::controlDevice_drumWasher(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	return 0;
}

int ControllerDevice::controlDevice_light(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	SerialCommandInfo cmd_info;
	Json::Value tmp_obj;
	bool power;
	int param_num;

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;
	bytes[4] = 1;

	if(cmd.compare("IndividualControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Power") == 0) {
				power = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x41;
		bytes[5] = power ? 1 : 0;
	}
	else if(cmd.compare("GroupControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Power") == 0) {
				power = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x42;
		bytes[5] = power ? 1 : 0;
	}
	else{
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}
	
	SerialDevice::getChecksum(bytes, 6, &bytes[6], &bytes[7]);
	
	if(bytes[3] == 0x41){
		if(sendSerialCommand(bytes, 8) < 0){
			tracee("send command failed.");
			return -1;
		}

		if(getCommand(device_id, sub_id, 0xc1, &cmd_info) < 0){
			tracee("doesnot receive ack.");
			return -1;
		}
	}
	else{
		for(int i=0; i<3; i++){
			if(sendSerialCommand(bytes, 8) < 0){
				tracee("send command failed.");
			}
			usleep(MIN_REQUEST_DELAY * 1000);
		}
	}

	return 0;
}

int ControllerDevice::controlDevice_gasValve(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	SerialCommandInfo cmd_info;
	Json::Value tmp_obj;
	bool closed;
	int param_num;

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;
	bytes[4] = 1;

	if(cmd.compare("IndividualControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Closed") == 0) {
				closed = tmp_obj["Value"].asBool();
				param_num++;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x41;
		bytes[5] = closed ? 0x01 : 0x00;
	}
	else if (cmd.compare("AllControl") == 0) {
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Closed") == 0) {
				closed = tmp_obj["Value"].asBool();
				param_num++;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x42;
		bytes[5] = closed ? 0x01 : 0x00;
	}
	else{
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}
	
	SerialDevice::getChecksum(bytes, 6, &bytes[6], &bytes[7]);
	
	if(bytes[3] == 0x41){
		if(sendSerialCommand(bytes, 8) < 0){
			tracee("send command failed.");
			return -1;
		}

		if(getCommand(device_id, sub_id, 0xc1, &cmd_info) < 0){
			tracee("doesnot receive ack.");
			return -1;
		}
	}
	else{
		for(int i=0; i<3; i++){
			if(sendSerialCommand(bytes, 8) < 0){
				tracee("send command failed.");
			}
			usleep(MIN_REQUEST_DELAY * 1000);
		}
	}

	return 0;
}

int ControllerDevice::controlDevice_curtain(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	SerialCommandInfo cmd_info;
	Json::Value tmp_obj;
	int operation;
	int param_num;

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;
	bytes[4] = 2;

	if (cmd.compare("IndividualControl") == 0) {
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Operation") == 0) {
				operation = tmp_obj["Value"].asInt();
				param_num++;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x41;
		bytes[5] = operation;
	}
	else if (cmd.compare("AllControl") == 0) {
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Operation") == 0) {
				operation = tmp_obj["Value"].asInt();
				param_num++;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x42;
		bytes[5] = operation;
	}
	else{
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}
	
	SerialDevice::getChecksum(bytes, 7, &bytes[7], &bytes[8]);
	
	if(bytes[3] == 0x41){
		if(sendSerialCommand(bytes, 9) < 0){
			tracee("send command failed.");
			return -1;
		}

		if(getCommand(device_id, sub_id, 0xc1, &cmd_info) < 0){
			tracee("doesnot receive ack.");
			return -1;
		}
	}
	else{
		for(int i=0; i<3; i++){
			if(sendSerialCommand(bytes, 9) < 0){
				tracee("send command failed.");
			}
			usleep(MIN_REQUEST_DELAY * 1000);
		}
	}

	return 0;
}

int ControllerDevice::controlDevice_remoteInspector(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	return 0;
}

int ControllerDevice::controlDevice_doorLock(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	SerialCommandInfo cmd_info;
	Json::Value tmp_obj;
	bool param_num, open_flag;

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;
	bytes[4] = 1;

	if (cmd.compare("IndividualControl") == 0) {
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Opened") == 0) {
				open_flag = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x41;
		bytes[5] = open_flag ? 1 : 0;
	}
	else if (cmd.compare("AllControl") == 0) {
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Opened") == 0) {
				open_flag = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x42;
		bytes[5] = open_flag ? 1 : 0;
	}
	else{
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}

	SerialDevice::getChecksum(bytes, 6, &bytes[6], &bytes[7]);
	
	if(bytes[3] == 0x41){
		if(sendSerialCommand(bytes, 8) < 0){
			tracee("send command failed.");
			return -1;
		}

		if(getCommand(device_id, sub_id, 0xc1, &cmd_info) < 0){
			tracee("doesnot receive ack.");
			return -1;
		}
	}
	else{
		for(int i=0; i<3; i++){
			if(sendSerialCommand(bytes, 8) < 0){
				tracee("send command failed.");
			}
			usleep(MIN_REQUEST_DELAY * 1000);
		}
	}

	return 0;
}

int ControllerDevice::controlDevice_vantilator(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	SerialCommandInfo cmd_info;
	bool power;
	int vol;
	Json::Value tmp_obj;
	int param_num;

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;
	bytes[4] = 1;

	if(cmd.compare("PowerControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Power") == 0) {
				power = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x41;
		bytes[5] = power ? 1 : 0;
	}
	else if(cmd.compare("AirVolumeControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("AirVolume") == 0) {
				vol = tmp_obj["Value"].asInt();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x42;
		bytes[5] = vol;
	}
	else{
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}
	
	SerialDevice::getChecksum(bytes, 6, &bytes[6], &bytes[7]);
	
	if(sendSerialCommand(bytes, 8) < 0){
		tracee("send command failed.");
		return -1;
	}

	if(getCommand(device_id, sub_id, bytes[3] | 0x80, &cmd_info) < 0){
		tracee("doesnot receive ack.");
		return -1;
	}

	return 0;
}

int ControllerDevice::controlDevice_breaker(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	int last_idx;
	bool closed;
	SerialCommandInfo cmd_info;
	Json::Value tmp_obj;
	int param_num;

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;

	if(cmd.compare("IndividualRelayControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("LightRelayClosed") == 0) {
				closed = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x41;
		bytes[4] = 1;
		bytes[5] = closed ? 0x01 : 0x00;

		last_idx = 5;
	}
	else if(cmd.compare("AllRelayControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("LightRelayClosed") == 0) {
				closed = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x42;
		bytes[4] = 2;
		bytes[5] = closed ? 0xff : 0x00;
		bytes[6] = 0x00;

		last_idx = 6;
	}
	else{
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}
	
	SerialDevice::getChecksum(bytes, last_idx + 1, &bytes[last_idx + 1], &bytes[last_idx + 2]);
	
	if((sub_id & 0x0f) == 0x0f){
		for(int i=0; i<3; i++){
			if(sendSerialCommand(bytes, last_idx +3) < 0){
				tracee("send command failed.");
			}
			usleep(MIN_REQUEST_DELAY * 1000);
		}
	}
	else{
		if(sendSerialCommand(bytes, last_idx + 3) < 0){
			tracee("send command failed.");
			return -1;
		}

		if(getCommand(device_id, sub_id, bytes[3] | 0x80, &cmd_info) < 0){
			tracee("doesnot receive ack.");
			return -1;
		}
	}

	return 0;
}

int ControllerDevice::controlDevice_boiler(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	SerialCommandInfo cmd_info;
	Json::Value tmp_obj;
	bool heating, outgoing;
	int int_temperature;
	double double_temperature;
	int param_num;

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;
	bytes[4] = 1;

	if(cmd.compare("HeatingControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Heating") == 0) {
				heating = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x43;
		bytes[5] = heating ? 0x01 : 0x00;
	}
	else if(cmd.compare("SetTemperatureControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("SettingTemperature") == 0) {
				double_temperature = tmp_obj["Value"].asDouble();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x44;

		int_temperature = int(double_temperature);
		if (double_temperature - int_temperature > 0) {
			int_temperature |= 0x80;
		}
		bytes[5] = int_temperature;
	}
	else if(cmd.compare("SetOutGoingModeControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Outgoing") == 0) {
				outgoing = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x45;
		bytes[5] = outgoing ? 0x01 : 0x00;
	}
	else{
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}
	
	SerialDevice::getChecksum(bytes, 6, &bytes[6], &bytes[7]);
	
	if((sub_id & 0x0f) == 0x0f){
		for(int i=0; i<3; i++){
			if(sendSerialCommand(bytes, 8) < 0){
				tracee("send command failed.");
			}
			usleep(MIN_REQUEST_DELAY * 1000);
		}
	}
	else{
		if(sendSerialCommand(bytes, 8) < 0){
			tracee("send command failed.");
			return -1;
		}

		if(getCommand(device_id, sub_id, bytes[3] | 0x80, &cmd_info) < 0){
			tracee("doesnot receive ack.");
			return -1;
		}
	}

	return 0;
}

int ControllerDevice::controlDevice_preventCrimeExt(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	SerialCommandInfo cmd_info;
	int last_idx;
	Json::Value tmp_obj, tmp_obj2;
	bool sensor_set[8];
	int sensor_types[8];
	int len;
	int param_num;

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;

	memset(sensor_set, false, sizeof(sensor_set));
	memset(sensor_types, false, sizeof(sensor_types));

	if(cmd.compare("SetSensorSetControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("SensorSet") == 0) {
				tmp_obj2 = tmp_obj["Value"];

				len = tmp_obj2.size();
				if (len > 8) {
					len = 8;
				}
				for (int j = 0; j < len; j++) {
					sensor_set[j] = tmp_obj2[j].asBool();
				}
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x43;
		bytes[4] = 1;

		for(unsigned int i=0; i<8; i++){
			bytes[5] |= sensor_set[i] ? 0x01 << i : 0x00;
		}

		last_idx = 5;
	}
	else if(cmd.compare("SetSensorTypeControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("SensorTypes") == 0) {
				tmp_obj2 = tmp_obj["Value"];

				len = tmp_obj2.size();
				if (len > 8) {
					len = 8;
				}
				for (int j = 0; j < len; j++) {
					sensor_types[j] = tmp_obj2[j].asInt();
				}
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x44;
		bytes[4] = 2;

		for(unsigned int i=0; i<8; i++){
			if(i>3){
				bytes[5] |= (sensor_types[i] & 0x03) << ((i-4)*2);
			}
			else{
				bytes[6] |= (sensor_types[i] & 0x03) << i*2;
			}
		}

		last_idx = 6;
	}
	else{
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}
	
	SerialDevice::getChecksum(bytes, last_idx + 1, &bytes[last_idx + 1], &bytes[last_idx + 2]);
	
	if((sub_id & 0x0f) == 0x0f){
		for(int i=0; i<3; i++){
			if(sendSerialCommand(bytes, last_idx + 3) < 0){
				tracee("send command failed.");
			}
			usleep(MIN_REQUEST_DELAY * 1000);
		}
	}
	else{
		if(sendSerialCommand(bytes, last_idx + 3) < 0){
			tracee("send command failed.");
			return -1;
		}

		if(getCommand(device_id, sub_id, bytes[3] | 0x80, &cmd_info) < 0){
			tracee("doesnot receive ack.");
			return -1;
		}
	}

	return 0;
}

int ControllerDevice::controlDevice_temperatureController(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	return controlDevice_boiler(device_id, sub_id, device_info, cmd, params, res_obj);
}

int ControllerDevice::controlDevice_zigbee(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	return 0;
}

int ControllerDevice::controlDevice_powerMeter(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	return 0;
}

int ControllerDevice::controlDevice_powerGate(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& res_obj)
{
	uint8_t bytes[64];
	SerialCommandInfo cmd_info;
	Json::Value tmp_obj;
	int last_idx;
	int param_num, dev_num;
	bool power;

	dev_num = 0;
	for (unsigned int i = 0; i < m_devices.size(); i++) {
		if (m_devices[i].m_device_id == device_id) {
			dev_num = m_devices[i].m_sub_ids.size();
			break;
		}
	}

	memset(bytes, 0x00, 64);

	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;

	if(cmd.compare("IndividualControl") == 0){
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Power") == 0) {
				power = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x41;
		bytes[4] = 1;
		bytes[5] = power ? 0x01 : 0x00;

		last_idx = 5;
	}
	else if(cmd.compare("AllControl") == 0){
		if (dev_num == 0) {
			tracee("the device has no sub device: %x", device_id);
			return -1;
		}
		param_num = 0;
		for (unsigned int i = 0; i < params.size(); i++) {
			tmp_obj = params[i];
			if (tmp_obj["Name"].asString().compare("Power") == 0) {
				power = tmp_obj["Value"].asBool();
				param_num++;
				break;
			}
		}

		if (param_num != 1) {
			tracee("invalid params: %s", params.toStyledString().c_str());
			return -1;
		}

		bytes[3] = 0x42;
		bytes[4] = dev_num;
		for (int i = 0; i < dev_num; i++) {
			bytes[5 + i] = power ? 0x01 : 0x00;
		}
		last_idx = 4 + dev_num;
	}
	else{
		tracee("invalid command: %s", cmd.c_str());
		return -1;
	}
	
	SerialDevice::getChecksum(bytes, last_idx + 1, &bytes[last_idx + 1], &bytes[last_idx + 2]);
	
	if(bytes[3] == 0x42){
		for(int i=0; i<3; i++){
			if(sendSerialCommand(bytes, last_idx + 3) < 0){
				tracee("send command failed.");
			}
			usleep(MIN_REQUEST_DELAY * 1000);
		}
	}
	else{
		if(sendSerialCommand(bytes, last_idx + 3) < 0){
			tracee("send command failed.");
			return -1;
		}

		if(getCommand(device_id, sub_id, bytes[3] | 0x80, &cmd_info) < 0){
			tracee("doesnot receive ack.");
			return -1;
		}
	}

	return 0;
}

int ControllerDevice::sendGetDeviceStatus(int device_id, int sub_id)
{
	uint8_t bytes[64];

	//get status
	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;
	bytes[3] = 0x01;
	bytes[4] = 0;

	SerialDevice::getChecksum(bytes, 5, &bytes[5], &bytes[6]);

	if (sendSerialCommand(bytes, 7) < 0) {
		tracee("send command failed.");
		return -1;
	}

	return 0;
}

int ControllerDevice::sendGetDeviceCharacteristic(int device_id, int sub_id)
{
	uint8_t bytes[64];

	//send to get characteristic
	bytes[0] = FRAME_START_BYTE;
	bytes[1] = device_id;
	bytes[2] = sub_id;
	bytes[3] = 0x0f;
	bytes[4] = 0;

	SerialDevice::getChecksum(bytes, 5, &bytes[5], &bytes[6]);

	if (sendSerialCommand(bytes, 7) < 0) {
		tracee("send command failed.");
		return -1;
	}

	return 0;
}


int ControllerDevice::sendSerialCommand(uint8_t* bytes, int len)
{
	struct timeval tv;
	static struct timeval pre_tv;
	static long interval;
	static bool is_first = true;
	int dsec, dnsec;
	long need_sleep;

	memcpy(m_last_sent_bytes, bytes, len);
	m_last_sent_bytes_len = len;

	gettimeofday2(&tv);
	if (is_first == true) {
		pre_tv = tv;
		is_first = false;
		return HomeDevice::sendSerialCommand(bytes, len);
	}

	dsec = tv.tv_sec - pre_tv.tv_sec;
	dnsec = tv.tv_usec - pre_tv.tv_usec;
	interval = (dsec * 1000) + (dnsec / 1000);

	pre_tv = tv;

	need_sleep = (MIN_REQUEST_DELAY - interval);
	trace("interval: %ld ms,    neep_sleep: %ld ms", interval, need_sleep);
	if (need_sleep > 0) {
		usleep(need_sleep * 1000);
	}
	return HomeDevice::sendSerialCommand(bytes, len);
}

void ControllerDevice::recvHttpuMessage(HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	uint8_t* body;
	std::string mime;
	int len;
	Json::Value jobj, jobj2, jobj3, jobj4;
	std::string path;
	std::string err_msg;
	struct DiscoveryRequestInfo req_info;

	HomeDevice::recvHttpuMessage(message, sock_info, from_addr);
	message->getPath(&path);
	//trace("in controller device recvHttpuMessage: %s", path.c_str());

	if (path.compare("/advertisement") == 0) {
		//trace("in controller device Advertisement...");
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

	//process discovery
	if (path.compare("/advertisement") == 0) {
		if (m_device_id != CONTROLLER_DEVICE_ID) {
			return;
		}

		jobj2 = jobj["Advertisement"];
		if (jobj2.type() != Json::objectValue) {
			trace("invalid httpu message: \n%s", body);
			return;
		}

		processAdvertisement(jobj2);
	}
}

void ControllerDevice::processDiscoveryResponse(HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	uint8_t* body;
	std::string mime;
	int len;
	Json::Value jobj, dis_res, jobj_dev_list;
	std::string path;
	std::string err_msg;

	//trace("in controller processDiscoveryResponse...");

	if (message->isRequest() == true) {
		return;
	}

	len = ((HttpMessage*)message)->getBody(&body, &mime);

	if (parseJson((char*)body, (char*)(body + len), &jobj, &err_msg) == false) {
		trace("invalid httpu message: \n%s", jobj.toStyledString().c_str());
		return;
	}

	//process discovery response
	//trace("discovery response: \n%s", jobj.toStyledString().c_str());
	dis_res = jobj["DiscoveryResponse"];
	if (dis_res.type() != Json::objectValue) {
		trace("invalid httpu message: \n%s", jobj.toStyledString().c_str());
		return;
	}

	jobj_dev_list = dis_res["DeviceList"];
	if (jobj_dev_list.type() != Json::arrayValue) {
		trace("invalid httpu message: \n%s", jobj.toStyledString().c_str());
		return;
	}

	for (unsigned int i = 0; i < jobj_dev_list.size(); i++) {
		processAdvertisement(jobj_dev_list[i]);
	}

	return;
}

void ControllerDevice::processAdvertisement(Json::Value jobj_dev)
{
	Json::Value jobj, jobj_list;
	int sub_id;
	struct DeviceInformation dev_info;
	bool is_found;

	//trace("process advertisement: \n%s", jobj_dev.toStyledString().c_str());

	//device id
	jobj = jobj_dev["DeviceID"];
	if (jobj.type() != Json::stringValue) {
		trace("invalid httpu message: \n%s", jobj_dev.toStyledString().c_str());
		return;
	}
	dev_info.m_device_id = hex_str2int(jobj.asString());

	//device sub id & number
	jobj_list = jobj_dev["DeviceSubIDs"];
	if (jobj_list.type() != Json::arrayValue) {
		trace("invalid httpu message: \n%s", jobj_dev.toStyledString().c_str());
		return;
	}

	for (unsigned int i = 0; i < jobj_list.size(); i++) {
		sub_id = hex_str2int(jobj_list[i].asString());
		if (sub_id != 0) {
			dev_info.m_sub_ids.push_back(sub_id);
		}
	}
	if (dev_info.m_sub_ids.size() == 0) {
		trace("invalid device number");
		return;
	}
	dev_info.m_sub_gid = dev_info.m_sub_ids[0] & 0xf0;

	//name
	dev_info.m_device_name = HomeDevice::getDeviceNameString(dev_info.m_device_id);

	//base url
	jobj = jobj_dev["BaseURL"];
	if (jobj.type() != Json::stringValue) {
		trace("invalid httpu message: \n%s", jobj_dev.toStyledString().c_str());
		return;
	}
	dev_info.m_base_url = jobj.asString();
	dev_info.m_type = DEVICE_TYPE_ETHERNET;

	//check if already exist
	is_found = false;
	for (unsigned int i = 0; i < m_devices.size(); i++) {
		if ((dev_info.m_device_id == m_devices[i].m_device_id) && (dev_info.m_sub_gid == m_devices[i].m_sub_gid)) {
			is_found = true;
			/*
			trace("update device: %s, 0x%02x, 0x%02x, %d",
				dev_info.m_device_name.c_str(),
				dev_info.m_device_id,
				dev_info.m_sub_gid,
				dev_info.m_sub_ids.size());
			*/
			m_devices[i].m_base_url = dev_info.m_base_url;
			m_devices[i].m_type |= DEVICE_TYPE_ETHERNET;
			break;
		}
	}
	if(is_found == false) {
		trace("add device: %s, 0x%02x, %d",
			dev_info.m_device_name.c_str(),
			dev_info.m_device_id,
			dev_info.m_sub_ids.size());

		m_devices.push_back(dev_info);
	}
}

void ControllerDevice::sendDiscovery()
{
	HttpuMessage message;
	Json::Value dis, jobj;
	std::vector<struct UdpSocketInfo> socket_infos;

	trace("send discovery message...");

	socket_infos = m_discovery_httpu_server->getSocketInfos();

	jobj["Type"] = std::string("All");
	dis["Discovery"] = jobj;

	message.setRequest();
	message.setMethod("POST");
	message.setPath("/discovery");
	message.setBody((uint8_t*)dis.toStyledString().c_str(), dis.toStyledString().length(), std::string("application/json"));

	for (unsigned int i = 0; i < socket_infos.size(); i++) {
#ifdef __ANDROID__
		m_httpu_server->send(socket_infos[i].m_socket, "255.255.255.255", HTTPU_DISCOVERY_PORT, &message);
#else
		m_httpu_server->send(socket_infos[i].m_socket, HTTPU_DISCOVERY_ADDR, HTTPU_DISCOVERY_PORT, &message);
#endif
	}

	return;
}

void ControllerDevice::printDeviceList()
{
	std::string type;
	std::string url;
	std::string tmp_str;

	trace("Device List: %d --------", m_devices.size());
	for (unsigned int i = 0; i < m_devices.size(); i++) {
		if (m_devices[i].m_type == DEVICE_TYPE_SERIAL) {
			type = std::string("serial_only");
			url = std::string("url_none");
		}
		else if (m_devices[i].m_type == DEVICE_TYPE_ETHERNET) {
			type = std::string("ethernet_only");
			url = m_devices[i].m_base_url;
		}
		else {
			type = std::string("serial_ethernet_both");
			url = m_devices[i].m_base_url;
		}

		tmp_str = std::string("");
		for (unsigned int j = 0; j < m_devices[i].m_sub_ids.size(); j++) {
			if (j != 0) {
				tmp_str += ", ";
			}
			tmp_str += int2hex_str(m_devices[i].m_sub_ids[j]);
		}
		printf("	%s, 0x%02x, [%s], %s, %s\n",
			m_devices[i].m_device_name.c_str(),
			m_devices[i].m_device_id,
			tmp_str.c_str(),
			type.c_str(), url.c_str());
	}

	return;
}