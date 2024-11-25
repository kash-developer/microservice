
#include "breaker_device.h"
#include "trace.h"
#include "tools.h"

#ifdef WIN32
#else
#include <unistd.h>
#endif

#include <time.h>

BreakerDevice::BreakerDevice() : HomeDevice()
{
}

BreakerDevice::~BreakerDevice()
{
	stop();
}

int BreakerDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj;
	Json::Value tmp_json_obj2;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	struct BreakerStatus status;

	trace("in curtain init");

	m_ele_control_update = 0;

	if (HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0) {
		return -1;
	}

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		status.m_sub_id = m_sub_ids[i];
		status.m_light_relay_closed = false;
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
	m_obj_characteristic["DeviceList"] = devices;

	trace("characteristic: \n%s", m_obj_characteristic.toStyledString().c_str());

	setCmdInfo();
	setStatus();
	finalizeInit();

	return 0;
}

void BreakerDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
}

void BreakerDevice::setStatus()
{
	Json::Value devices, device, sub_devices, sub_device, statuses, status;

	HomeDevice::setStatus();

	sub_devices.clear();
	devices.clear();
	
	trace("size: %d, %d", m_sub_ids.size(), m_statuses.size());
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		status.clear();
		statuses.clear();
		sub_device.clear();

		status["Name"] = "LightRelayClosed";
		status["Type"] = "boolean";
		status["Value"] = m_statuses[i].m_light_relay_closed;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(m_statuses[i].m_sub_id);
		sub_device["Status"] = statuses;

		trace("add: %x", m_statuses[i].m_sub_id);
		sub_devices.append(sub_device);
	}

	device["DeviceID"] = int2hex_str(m_device_id);
	device["SubDeviceList"] = sub_devices;
	devices.append(device);
	m_obj_status["DeviceList"] = devices;

	trace("set status: \n%s", m_obj_status.toStyledString().c_str());
}

int BreakerDevice::run()
{
	return HomeDevice::run();
}

void BreakerDevice::stop()
{
	HomeDevice::stop();
}

int BreakerDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	std::string cmd;
	Json::Value params;
	int value;
	Json::Value ret_param, ret_params, sub_device, control;
	int int_cmd;

	if (HomeDevice::processHttpCommand(device_id, sub_id, cmd_obj, res_obj) < 0) {
		tracee("process http request failed.");
		res_obj = createErrorResponse(ERROR_CODE_INTERNAL_ERROR);
		return -1;
	}

	if (cmd_obj["CommandType"].type() != Json::stringValue) {
		res_obj = createErrorResponse(ERROR_CODE_INVALID_PARAMETER);
		tracee("there is no commandType field.");
		return -1;
	}
	cmd = cmd_obj["CommandType"].asString();
	int_cmd = hex_str2int(cmd);

	trace("received: \n%s", cmd_obj.toStyledString().c_str());
	if (extractParametersByCommandInfo(device_id, cmd_obj, params) < 0) {
		trace("extractParametersByCommandInfo failed.");
		res_obj = createErrorResponse(ERROR_CODE_INVALID_PARAMETER);
		return -1;
	}

	if (params["LightRelayClosed"]["Value"].asBool() == true) {
		value = 0x0f;
	}
	else {
		value = 0x00;
	}

	if(((cmd.compare("IndividualRelayControl") == 0) || (cmd.compare("AllRelayControl") == 0)) || ((int_cmd == 0x41) || (int_cmd == 0x42))) {
		if(individualRelayControl(sub_id, value) < 0){
			trace("individual relay control failed.");
			res_obj = createErrorResponse(ERROR_CODE_INTERNAL_ERROR);
			return -1;
		}
	}	

	setStatus();

	res_obj = Json::arrayValue;
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			control.clear();
			ret_param.clear();
			ret_params.clear();
			if ((cmd.compare("IndividualRelayControl") == 0) || (int_cmd == 0x41)) {
				ret_param.clear();
				ret_param["Name"] = "LightRelayClosed";
				ret_param["Value"] = m_statuses[i].m_light_relay_closed;
				ret_param["Type"] = "boolean";
				ret_params.append(ret_param);

				control["CommandType"] = "0xc1";
				control["Parameters"] = ret_params;
			}
			else if ((cmd.compare("AllRelayControl") == 0) || (int_cmd == 0x42)) {
				ret_param.clear();
				for (unsigned int j = 0; j < 8; j++) {
					ret_param["Name"] = std::to_string(j) + "_LightRelayClosed";
					if (j < m_statuses.size()) {
						ret_param["Value"] = m_statuses[j].m_light_relay_closed;
					}
					else {
						ret_param["Value"] = false;
					}
					ret_param["Type"] = "boolean";
					ret_params.append(ret_param);
				}
				control["CommandType"] = "0xc2";
				control["Parameters"] = ret_params;
			}
			else {
				control["CommandType"] = int2hex_str(int_cmd + 0x80);
				control["Parameters"] = Json::Value(Json::arrayValue);
				control["error"] = "Invalid Command";
			}
			sub_device["SubDeviceID"] = int2hex_str(m_statuses[i].m_sub_id);
			sub_device["Control"] = Json::arrayValue;
			sub_device["Control"].append(control);

			res_obj.append(sub_device);
		}
	}

	if (res_obj.size() == 0) {
		control["CommandType"] = int2hex_str(int_cmd + 0x80);
		control["Parameters"] = Json::Value(Json::arrayValue);
		control["error"] = "Invalid SubDeviceID";

		sub_device["SubDeviceID"] = int2hex_str(sub_id);
		sub_device["Control"] = Json::arrayValue;
		sub_device["Control"].append(control);

		res_obj.append(sub_device);
	}

	return 0;
}

int BreakerDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processBreakerSerialCommand(&cmd_info);
}

int BreakerDevice::processBreakerSerialCommand(SerialCommandInfo* cmd_info)
{
	int body_len;
	int res_type;
	uint8_t res_body_bytes[255];

	memset(res_body_bytes, 0, 255);
	res_type = cmd_info->m_command_type | 0x80;
	body_len = 0;

	//request breaker status info
	if(cmd_info->m_command_type == 0x01){
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if (cmd_info->m_sub_id == m_statuses[i].m_sub_id) {
				res_body_bytes[0] = 0x00;
				res_body_bytes[1] |= m_statuses[i].m_light_relay_closed ? 0x04 : 0x00;
				res_body_bytes[2] = 0x00;
				body_len = 3;
				break;
			}
		}
	}
	//request breaker chractoristic info
	else if(cmd_info->m_command_type == 0x0f){
		res_body_bytes[0] = m_characteristic.m_version;
		res_body_bytes[1] = m_characteristic.m_company_code;
		res_body_bytes[2] = 0x00;
		res_body_bytes[3] = 0x00;
		res_body_bytes[4] = 0x00;
		res_body_bytes[5] = 0x00;
		res_body_bytes[6] = 0x00;
		res_body_bytes[7] = 0x00;
		res_body_bytes[8] = 0x00;
		res_body_bytes[9] = 0x00;
		res_body_bytes[10] = 0x00;

		body_len = 11;
	}
	//request individual relay control
	else if(cmd_info->m_command_type == 0x41){
		if(individualRelayControl(cmd_info->m_sub_id, cmd_info->m_data[0]) < 0){
			tracee("individual relay control failed.");
		}

		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if (cmd_info->m_sub_id == m_statuses[i].m_sub_id) {
				res_body_bytes[0] = 0x00;
				res_body_bytes[1] |= m_statuses[i].m_light_relay_closed ? 0x04 : 0x00;
				res_body_bytes[2] = 0x00;
				body_len = 3;
				break;
			}
		}
	}
	//request all relay control
	else if(cmd_info->m_command_type == 0x42){
		if(allRelayControl(cmd_info->m_data[0]) < 0){
			tracee("all relay control failed.");
		}
		body_len = 0;
	}
	else{
		tracee("invalid command type.");
		return -1;
	}

	if(body_len > 0){
		sendSerialResponse(cmd_info, res_type, res_body_bytes, body_len);
	}

	setStatus();

	return 0;
}

int BreakerDevice::individualRelayControl(int sub_id, int light_relay_closed)
{
	int ret = -1;

	trace("individualRelayControl: %x, %d", sub_id, light_relay_closed);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			if (light_relay_closed == 0) {
				m_statuses[i].m_light_relay_closed = false;
			}
			else {
				m_statuses[i].m_light_relay_closed = true;
			}
			ret = 0;
		}
	}
	return ret;
}

int BreakerDevice::allRelayControl(int light_relay_closed)
{
	int value;

	trace("allRelayControl: %d", light_relay_closed);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		value = light_relay_closed & (0x01 << i);
		if (value == 0) {
			m_statuses[i].m_light_relay_closed = false;
		}
		else {
			m_statuses[i].m_light_relay_closed = true;
		}
	}
	return 0;
}








