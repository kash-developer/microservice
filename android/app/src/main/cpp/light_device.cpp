
#include "light_device.h"
#include "trace.h"
#include "tools.h"

LightDevice::LightDevice() : HomeDevice()
{
}

LightDevice::~LightDevice()
{
	stop();
}

int LightDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj, tmp_json_obj2, tmp_json_obj3;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	struct LightStatus status;

	trace("in lihght init.");

	if (HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0) {
		return -1;
	}

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		status.m_sub_id = m_sub_ids[i];
		status.m_power = false;

		m_statuses.push_back(status);
	}

	m_characteristic.m_onoff_dev_number = (uint8_t)m_sub_ids.size();
	m_characteristic.m_version = -1;
	m_characteristic.m_company_code = -1;

	if (conf_json[m_device_name].type() == Json::objectValue) {
		tmp_json_obj = conf_json[m_device_name];

		m_use_old_characteristic = true;
		if (tmp_json_obj["UseOldCharacteristic"].type() == Json::booleanValue) {
			m_use_old_characteristic = tmp_json_obj["UseOldCharacteristic"].asBool();
			trace("use old characteristic: %d", m_use_old_characteristic);
		}

		if (tmp_json_obj["Characteristics"].type() == Json::objectValue) {
			tmp_json_obj = tmp_json_obj["Characteristics"];
			if (m_use_old_characteristic == true) {
				trace("use old characteristic.");
				m_characteristic.m_version = 0;
				m_characteristic.m_company_code = -1;
			}
			else {
				trace("use new characteristic.");
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
	}

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		obj_chars.clear();

		obj_char["Name"] = "OnOffDeviceNumber";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_onoff_dev_number;
		obj_chars.append(obj_char);

		if (m_use_old_characteristic == true) {
			obj_char["Name"] = "DimmingDeviceNumber";
			obj_char["Type"] = "integer";
			obj_char["Value"] = 0;
			obj_chars.append(obj_char);

			obj_char["Name"] = "DimmingDeviceIndex";
			obj_char["Type"] = "integer";
			obj_char["Value"] = 0;
			obj_chars.append(obj_char);
		}
		else {
			obj_char["Name"] = "Version";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_version;
			obj_chars.append(obj_char);

			obj_char["Name"] = "CompanyCode";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_company_code;
			obj_chars.append(obj_char);
		}

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

void LightDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
	trace("cmd_info: \n%s", m_obj_cmd.toStyledString().c_str());
}

void LightDevice::setStatus()
{
	Json::Value devices, device, sub_devices, sub_device, statuses, status;

	HomeDevice::setStatus();

	statuses.clear();
	sub_devices.clear();
	devices.clear();

	for (unsigned int i = 0; i<m_statuses.size(); i++) {
		statuses.clear();

		status["Name"] = "Power";
		status["Type"] = "boolean";
		status["Value"] = m_statuses[i].m_power;
		statuses.append(status);

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

int LightDevice::run()
{
	return HomeDevice::run();
}

void LightDevice::stop()
{
	HomeDevice::stop();
}

int LightDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	std::string cmd;
	Json::Value params;
	Json::Value ret_param, ret_params, sub_device, control;
	int int_cmd;
	bool power_package_off_flag;

	power_package_off_flag = false;

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

	if((cmd.compare("IndividualControl") == 0) || (int_cmd == 0x41)) {
		setPower(sub_id, params["Power"]["Value"].asInt());
		//setDimmingLevel(sub_id, params["Dimming"]["Value"].asInt());
	}
	else if((cmd.compare("GroupControl") == 0) || (int_cmd == 0x42)) {
		setPower(sub_id, params["Power"]["Value"].asInt());
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
				ret_param["Name"] = "Power";
				ret_param["Value"] = m_statuses[i].m_power;
				ret_param["Type"] = "boolean";
				ret_params.append(ret_param);

				control["CommandType"] = "0xc1";
				control["Parameters"] = ret_params;
			}
			else if ((cmd.compare("GroupControl") == 0) || (int_cmd == 0x42)) {
				ret_param.clear();
				ret_param["Name"] = "Power";
				ret_param["Value"] = m_statuses[i].m_power;
				ret_param["Type"] = "boolean";
				ret_params.append(ret_param);

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

int LightDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processLightSerialCommand(&cmd_info);
}

int LightDevice::processLightSerialCommand(SerialCommandInfo* cmd_info)
{
	int idx, body_len;
	int res_type;
	uint8_t res_body_bytes[255];
	bool power;
	bool power_package_off_flag;

	memset(res_body_bytes, 0, 255);
	res_type = 0;
	body_len = 0;
	power_package_off_flag = false;

	//request light status info
	if(cmd_info->m_command_type == 0x01){
		res_type = 0x81;
		res_body_bytes[0] = 0x00;

		idx = 0;
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if ((cmd_info->m_sub_id == m_statuses[i].m_sub_id) || ((cmd_info->m_sub_id & 0x0f) == 0x0f)){
				if (m_statuses[i].m_power == true) {
					res_body_bytes[1 + idx] |= 0x01;
				}
				idx++;
			}
		}
		body_len = idx + 1;

		//no such sub_id
		if (idx == 0) {
			body_len = 0;
		}
	}
	//request light chractoristic info
	else if(cmd_info->m_command_type == 0x0f){
		res_type = 0x8f;

		if (m_use_old_characteristic == true) {
			res_body_bytes[0] = 0;
			res_body_bytes[1] = m_characteristic.m_onoff_dev_number;
			res_body_bytes[2] = 0;
			res_body_bytes[3] = 0;
			res_body_bytes[4] = 0;

			body_len = 0x05;
		}
		else {
			res_body_bytes[0] = m_characteristic.m_version;
			res_body_bytes[1] = m_characteristic.m_company_code;
			res_body_bytes[2] = m_characteristic.m_onoff_dev_number;

			body_len = 0x0b;
		}
	}
	//request light control: on/off, dimming level
	else if(cmd_info->m_command_type == 0x41){
		power = cmd_info->m_data[0];
		setPower(cmd_info->m_sub_id, power);

		res_type = 0xc1;
		res_body_bytes[0] = 0;
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if (cmd_info->m_sub_id == m_statuses[i].m_sub_id) {
				if (m_statuses[i].m_power == true) {
					res_body_bytes[1] = 0x01;
				}
			}
		}
		body_len = 2;
	}
	//request all of lights control: on/off
	else if (cmd_info->m_command_type == 0x42) {
		power = cmd_info->m_data[0];
		setPower(cmd_info->m_sub_id, power);
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

int LightDevice::setPower(int sub_id, bool power, bool is_package)
{
	trace("setPower %x: %d", sub_id, power);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			m_statuses[i].m_power = power;
		}
	}

	return 0;
}

int LightDevice::setDimmingLevel(int sub_id, int level)
{
	trace("setDimmingLevel is disabled.");

	/*
	trace("setDimmingLevel %x: %d", sub_id, level);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			;
		}
	}
	*/
	return 0;
}



