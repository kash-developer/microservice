
#include "doorlock_device.h"
#include "trace.h"
#include "tools.h"

DoorLockDevice::DoorLockDevice() : HomeDevice()
{
}

DoorLockDevice::~DoorLockDevice()
{
	stop();
}

int DoorLockDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj;
	Json::Value tmp_json_obj2;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	struct DoorLockStatus status;

	trace("in doorlock init.");

	if (HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0) {
		return -1;
	}

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		status.m_sub_id = m_sub_ids[i];
		status.m_emergency = false;
		status.m_opened = false;
		m_statuses.push_back(status);
	}

	m_characteristic.m_version = -1;
	m_characteristic.m_company_code = -1;
	m_characteristic.m_forced_set_emergency_off = false;

	if(conf_json[m_device_name].type() == Json::objectValue){
		tmp_json_obj = conf_json[m_device_name];

		if (tmp_json_obj["Characteristic"].type() == Json::objectValue) {
			tmp_json_obj = tmp_json_obj["Characteristic"];

			if (tmp_json_obj["Version"].type() == Json::intValue) {
				m_characteristic.m_version = tmp_json_obj["Version"].asInt();
				trace("set version: %d, %d", m_characteristic.m_version, tmp_json_obj["Version"].asInt());
			}
			if (tmp_json_obj["CompanyCode"].type() == Json::intValue) {
				m_characteristic.m_company_code = tmp_json_obj["CompanyCode"].asInt();
				trace("set company code: %d, %d", m_characteristic.m_company_code, tmp_json_obj["CompanyCode"].asInt());
			}
			if (tmp_json_obj["ForcedSetEmergencyOff"].type() == Json::booleanValue) {
				m_characteristic.m_forced_set_emergency_off = tmp_json_obj["ForcedSetEmergencyOff"].asBool();
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

		obj_char["Name"] = "ForcedSetEmergencyOff";
		obj_char["Type"] = "boolean";
		obj_char["Value"] = m_characteristic.m_forced_set_emergency_off;
		obj_chars.append(obj_char);

		sub_device["SubDeviceID"] = int2hex_str(m_sub_ids[i]);
		sub_device["Characteristic"] = obj_chars;
		sub_devices.append(sub_device);

		device["DeviceID"] = int2hex_str(m_device_id);
		device["SubDeviceList"] = sub_devices;
		devices.append(device);
		m_obj_characteristic["DeviceList"] = devices;
	}
	trace("characteristic: \n%s", m_obj_characteristic.toStyledString().c_str());

	setCmdInfo();
	setStatus();
	finalizeInit();

	return 0;
}

void DoorLockDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
}

void DoorLockDevice::setStatus()
{
	Json::Value devices, device, sub_devices, sub_device, statuses, status;

	HomeDevice::setStatus();

	sub_devices.clear();
	devices.clear();

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		status["Name"] = "Emergency";
		status["Type"] = "boolean";
		status["Value"] = m_statuses[i].m_emergency;
		statuses.append(status);

		status["Name"] = "Opened";
		status["Type"] = "boolean";
		status["Value"] = m_statuses[i].m_opened;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(m_statuses[i].m_sub_id);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);

		device["DeviceID"] = int2hex_str(m_device_id);
		device["SubDeviceList"] = sub_devices;
		devices.append(device);
		m_obj_status["DeviceList"] = devices;
	}
	trace("set status: \n%s", m_obj_status.toStyledString().c_str());
}

int DoorLockDevice::run()
{
	return HomeDevice::run();
}

void DoorLockDevice::stop()
{
	HomeDevice::stop();
}

int DoorLockDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	std::string cmd;
	Json::Value params;
	int int_cmd;
	Json::Value ret_param, ret_params, sub_device, control;

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

	if ((cmd.compare("IndividualControl") == 0) || (int_cmd == 0x41)) {
		if (setClosed(sub_id, params["Opened"]["Value"].asBool()) < 0) {
			trace("set lock failed.");
			res_obj = createErrorResponse(ERROR_CODE_INTERNAL_ERROR);
			return -1;
		}
	}
	else if ((cmd.compare("AllControl") == 0) || (int_cmd == 0x42)) {
		if (setClosed(sub_id, params["Opened"]["Value"].asBool()) < 0) {
			trace("set lock failed.");
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
			if ((cmd.compare("IndividualControl") == 0) || (int_cmd == 0x41)) {
				ret_param.clear();
				ret_param["Name"] = "Opened";
				ret_param["Value"] = m_statuses[i].m_opened;
				ret_param["Type"] = "boolean";
				ret_params.append(ret_param);

				ret_param.clear();
				ret_param["Name"] = "Emergency";
				ret_param["Value"] = m_statuses[i].m_emergency;
				ret_param["Type"] = "boolean";
				ret_params.append(ret_param);

				res_obj["CommandType"] = "0xc1";
				res_obj["Parameters"] = ret_params;
			}
			else if ((cmd.compare("AllControl") == 0) || (int_cmd == 0x42)) {
				ret_param.clear();
				ret_param["Name"] = "Opened";
				ret_param["Value"] = params[0]["Value"];
				ret_param["Type"] = "boolean";
				ret_params.append(ret_param);

				res_obj["CommandType"] = "0xc2";
				res_obj["Parameters"] = ret_params;
			}
			else {
				res_obj["CommandType"] = int2hex_str(int_cmd + 0x80);
				res_obj["Parameters"] = Json::Value(Json::arrayValue);
				res_obj["error"] = "Invalid Command";
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

int DoorLockDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processDoorlockSerialCommand(&cmd_info);
}

int DoorLockDevice::processDoorlockSerialCommand(SerialCommandInfo* cmd_info)
{
	int body_len;
	int res_type;
	uint8_t res_body_bytes[255];

	memset(res_body_bytes, 0, 255);
	res_type = 0;
	body_len = 0;

	//request doorlock status info
	if(cmd_info->m_command_type == 0x01){
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if (cmd_info->m_sub_id == m_statuses[i].m_sub_id) {
				res_type = 0x81;
				res_body_bytes[0] = 0x00;
				res_body_bytes[1] |= (uint8_t)m_statuses[i].m_emergency << 1;
				res_body_bytes[1] |= (uint8_t)m_statuses[i].m_opened;
				body_len = 2;

				break;
			}
		}
	}
	//request doorlock chractoristic info
	else if(cmd_info->m_command_type == 0x0f){
		res_type = 0x8f;
		res_body_bytes[0] = m_characteristic.m_version;
		res_body_bytes[1] = m_characteristic.m_company_code;
		res_body_bytes[2] = (uint8_t)m_characteristic.m_forced_set_emergency_off;
		res_body_bytes[3] = 0;
		res_body_bytes[4] = 0;
		res_body_bytes[5] = 0;
		res_body_bytes[6] = 0;
		res_body_bytes[7] = 0;
		res_body_bytes[8] = 0;
		res_body_bytes[9] = 0;
		res_body_bytes[10] = 0;

		body_len = 11;
	}
	//request doorlock control: on/off
	else if(cmd_info->m_command_type == 0x41){
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if (cmd_info->m_sub_id == m_statuses[i].m_sub_id) {
				setClosed(cmd_info->m_sub_id, cmd_info->m_data[0]);

				res_type = 0xc1;
				res_body_bytes[0] = 0x00;
				res_body_bytes[1] = m_statuses[0].m_emergency << 1;
				res_body_bytes[1] |= (m_statuses[0].m_opened ? 1 : 0);
			}
		}

		body_len = 2;
	}
	//request all doorlocks control: on/off
	else if(cmd_info->m_command_type == 0x42){
		res_type = 0xc1;
		setClosed(cmd_info->m_sub_id, cmd_info->m_data[0]);
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

int DoorLockDevice::setEmergency(int sub_id, bool emergency)
{
	trace("set emergency %x: %d", sub_id, emergency);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			if (m_characteristic.m_forced_set_emergency_off == false) {
				tracee("forced set emergency off is false.");
				return -1;
			}
			trace("set emergency: %d", emergency);
			m_statuses[0].m_emergency = emergency;
		}
	}
	return 0;
}

int DoorLockDevice::setClosed(int sub_id, bool open_flag)
{
	trace("set opended %x: %d", sub_id, open_flag);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			m_statuses[0].m_opened = open_flag;
		}
	}
	return 0;
}



