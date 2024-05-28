
#include "vantilator_device.h"
#include "trace.h"
#include "tools.h"

VantilatorDevice::VantilatorDevice() : HomeDevice()
{
}

VantilatorDevice::~VantilatorDevice()
{
	stop();
}

int VantilatorDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj, tmp_json_obj2;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	struct VantilatorStatus status;

	trace("in vantilator init.");

	if (HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0) {
		return -1;
	}

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		status.m_sub_id = m_sub_ids[i];
		status.m_power = false;
		status.m_air_volume = 0x01;

		m_statuses.push_back(status);
	}

	m_characteristic.m_max_air_volume = VANTILATOR_DEFAULT_MAX_AIR_VOLUME;

	trace("conf: %s", conf_json.toStyledString().c_str());
	if(conf_json[m_device_name].type() == Json::objectValue){
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
			if (tmp_json_obj["MaxAirVolume"].type() == Json::intValue) {
				m_characteristic.m_max_air_volume = tmp_json_obj["MaxAirVolume"].asInt();
			}
		}
	}

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		obj_chars.clear();

		obj_char["Name"] = "Version";
		obj_char["Type"] = "iintegernt";
		obj_char["Value"] = m_characteristic.m_version;
		obj_chars.append(obj_char);

		obj_char["Name"] = "CompanyCode";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_company_code;
		obj_chars.append(obj_char);

		obj_char["Name"] = "MaxAirVolume";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_max_air_volume;
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


void VantilatorDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
}

void VantilatorDevice::setStatus()
{
	Json::Value devices, device, sub_devices, sub_device, statuses, status;

	HomeDevice::setStatus();

	sub_devices.clear();
	devices.clear();

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		statuses.clear();

		status["Name"] = "Error";
		status["Type"] = "integer";
		status["Value"] = 0;
		statuses.append(status);

		status["Name"] = "Power";
		status["Type"] = "boolean";
		status["Value"] = m_statuses[0].m_power;
		statuses.append(status);

		status["Name"] = "AirVolume";
		status["Type"] = "integer";
		status["Value"] = m_statuses[0].m_air_volume;
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

int VantilatorDevice::run()
{
	return HomeDevice::run();
}

void VantilatorDevice::stop()
{
	HomeDevice::stop();
}

int VantilatorDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
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

	if((cmd.compare("PowerControl") == 0) || (int_cmd == 0x41)) {
		if(setPower(sub_id, params["Power"]["Value"].asBool()) < 0){
			tracee("set power failed.");
			return -1;
		}
	}
	else if((cmd.compare("AirVolumeControl") == 0) || (int_cmd == 0x42)) {
		if(setAirVolume(sub_id, params["AirVolume"]["Value"].asInt()) < 0){
			trace("set air volume failed: %d", params[0]["Value"].asInt());
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
			if ((cmd.compare("PowerControl") == 0) || (cmd.compare("0x41") == 0)) {
				ret_param.clear();
				ret_param["Name"] = "Power";
				ret_param["Value"] = m_statuses[0].m_power;
				ret_param["Type"] = "boolean";
				ret_params.append(ret_param);

				control["CommandType"] = "0xc1";
				control["Parameters"] = ret_params;
			}
			else if ((cmd.compare("AirVolumeControl") == 0) || (cmd.compare("0x42") == 0)) {
				ret_param.clear();
				ret_param["Name"] = "AirVolume";
				ret_param["Value"] = m_statuses[0].m_air_volume;
				ret_param["Type"] = "integer";
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

int VantilatorDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processVantilatorSerialCommand(&cmd_info);
}

int VantilatorDevice::processVantilatorSerialCommand(SerialCommandInfo* cmd_info)
{
	int body_len;
	int res_type;
	uint8_t res_body_bytes[255];

	memset(res_body_bytes, 0, 255);
	res_type = 0;
	body_len = 0;

	//request vantilator status info
	if(cmd_info->m_command_type == 0x01){
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			res_type = 0x81;
			res_body_bytes[0] = 0x00;
			res_body_bytes[1] = m_statuses[i].m_power ? 0x01 : 0x00;
			res_body_bytes[2] = m_statuses[i].m_air_volume;
			res_body_bytes[3] = 0;
			res_body_bytes[4] = 0;
			body_len = 5;

			break;
		}
	}
	//request vantilator chractoristic info
	else if(cmd_info->m_command_type == 0x0f){
		res_type = 0x8f;
		res_body_bytes[0] = m_characteristic.m_version;
		res_body_bytes[1] = m_characteristic.m_company_code;
		res_body_bytes[2] = m_characteristic.m_max_air_volume;
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
	else if((cmd_info->m_command_type == 0x41) || 
				(cmd_info->m_command_type == 0x42)) { 
		res_type = cmd_info->m_command_type + 0x80;

		if(cmd_info->m_command_type == 0x41){
			if(cmd_info->m_data[0] ==  0x01){
				setPower(cmd_info->m_sub_id, true);
			}
			else{
				setPower(cmd_info->m_sub_id, false);
			}
		}
		else if(cmd_info->m_command_type == 0x42){
			setAirVolume(cmd_info->m_sub_id, cmd_info->m_data[0]);
		}

		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			res_body_bytes[0] = 0x00;
			res_body_bytes[1] = m_statuses[0].m_power ? 0x01 : 0x00;
			res_body_bytes[2] = m_statuses[0].m_air_volume;
			res_body_bytes[3] = 0;
			res_body_bytes[4] = 0;
			body_len = 5;
		
			break;
		}
	}

	if(body_len > 0){
		sendSerialResponse(cmd_info, res_type, res_body_bytes, body_len);
	}

	setStatus();

	return 0;
}

int VantilatorDevice::setPower(int sub_id, bool power)
{
	trace("set power %x: %d", sub_id, power);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			m_statuses[i].m_power = power;
		}
	}

	return 0;
}

int VantilatorDevice::setAirVolume(int sub_id, int air_volume)
{
	trace("set air volume %x: %d", sub_id, air_volume);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			m_statuses[i].m_air_volume = air_volume;
		}
	}

	return 0;
}






