
#include "powergate_device.h"
#include "trace.h"
#include "tools.h"

#ifdef WIN32
#else
#include <unistd.h>
#endif

#include <time.h>
#include <cmath>

PowerGateDevice::PowerGateDevice() : HomeDevice()
{
}

PowerGateDevice::~PowerGateDevice()
{
	stop();
}

int PowerGateDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj, tmp_json_obj2, tmp_json_obj3;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	struct PowerGateStatus status;
	double tmp_dvalue;

	trace("in power gate init.");

	if (HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0) {
		return -1;
	}

	srand((unsigned int)time(NULL));
	for(unsigned int i=0; i<m_sub_ids.size(); i++){
		status.m_sub_id = m_sub_ids[i];

		status.m_power = true;
		tmp_dvalue = rand() % 10000;
		status.m_power_measurement = tmp_dvalue / 10;
		trace("set power measurement: %f", status.m_power_measurement);

		m_statuses.push_back(status);
	}

	m_characteristic.m_version = -1;
	m_characteristic.m_company_code = -1;
	m_characteristic.m_channel_number = (uint8_t)m_sub_ids.size();

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

		obj_char["Name"] = "ChannelNumber";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_channel_number;
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

void PowerGateDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
}

void PowerGateDevice::setStatus()
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

		status["Name"] = "PowerMeasurement";
		status["Type"] = "number";
		status["Value"] = m_statuses[i].m_power_measurement;
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

int PowerGateDevice::run()
{
	return HomeDevice::run();
}

void PowerGateDevice::stop()
{
	HomeDevice::stop();
}

int PowerGateDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
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

	if((cmd.compare("IndividualControl") == 0) || (int_cmd == 0x41)) {
		if(individualControl(sub_id, params["Power"]["Value"].asBool()) < 0){
			tracee("individual control failed: %d", sub_id);
			return -1;
		}
	}
	else if((cmd.compare("AllControl") == 0) || (int_cmd == 0x42)) {
		if(allControl(sub_id, params["Power"]["Value"].asBool()) < 0){
			tracee("all control failed.");
			return -1;
		}
	}
	else{
		tracee("invalid control: %s", cmd.c_str());
		return -1;
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
			else if ((cmd.compare("AllControl") == 0) || (int_cmd == 0x42)) {
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

int PowerGateDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processPowerGateSerialCommand(&cmd_info);
}

int PowerGateDevice::processPowerGateSerialCommand(SerialCommandInfo* cmd_info)
{
	int body_len;
	int res_type, idx;
	uint8_t res_body_bytes[255];
	double power_mea;

	memset(res_body_bytes, 0, 255);
	res_type = 0;
	body_len = 0;

	trace("received cmd: %x\n", cmd_info->m_command_type);
	//request status info
	if(cmd_info->m_command_type == 0x01){
		res_type = 0x81;
		res_body_bytes[0] = 0x00;

		idx = 0;
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if ((cmd_info->m_sub_id == m_statuses[i].m_sub_id) || ((cmd_info->m_sub_id & 0x0f) == 0x0f)) {
				res_body_bytes[5 * i + 1] |= m_statuses[i].m_power ? 0x10 : 0x00;

				power_mea = m_statuses[i].m_power_measurement;
				res_body_bytes[5 * i + 1] |= (int)(power_mea / 1000) % 10;
				res_body_bytes[5 * i + 2] |= ((int)(power_mea / 100) % 10) << 4;
				res_body_bytes[5 * i + 2] |= (int)(power_mea / 10) % 10;
				res_body_bytes[5 * i + 3] |= ((int)(power_mea / 1) % 10) << 4;
				res_body_bytes[5 * i + 3] |= (int)((power_mea - (int)power_mea) * 10 + 0.5) % 10;

				idx++;
			}
		}
		body_len = 5 * idx + 1;

		//no such sub_id
		if (idx == 0) {
			body_len = 0;
		}
	}
	else if(cmd_info->m_command_type == 0x0f){
		res_type = 0x8f;
		body_len = 11;

		res_body_bytes[0] = m_characteristic.m_version;
		res_body_bytes[1] = m_characteristic.m_company_code;
		res_body_bytes[2] = m_characteristic.m_channel_number;
		res_body_bytes[3] = 0x00;
		res_body_bytes[4] = 0x00;
		res_body_bytes[5] = 0x00;
		res_body_bytes[6] = 0x00;
		res_body_bytes[7] = 0x00;
		res_body_bytes[8] = 0x00;
		res_body_bytes[9] = 0x00;
		res_body_bytes[10] = 0x00;
	}
	else if(cmd_info->m_command_type == 0x41){
		res_type = 0xc1;
		if(individualControl(cmd_info->m_sub_id, cmd_info->m_data[0]) < 0){
			tracee("individual control failed: %x, %x", cmd_info->m_sub_id, cmd_info->m_data[0]);
		}

		res_type = 0xc1;
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if (cmd_info->m_sub_id == m_statuses[i].m_sub_id) {
				if (m_statuses[i].m_power == true) {
					res_body_bytes[1] = 0x01;
				}
			}
		}
		body_len = 2;
	}
	else if(cmd_info->m_command_type == 0x42){
		if(allControl(cmd_info->m_sub_id, cmd_info->m_data[0]) < 0){
			tracee("all control failed: %x", cmd_info->m_data[0]);
		}
		body_len = 0;
	}
	else{
		tracee("invalid control: %x", cmd_info->m_command_type);
		return -1;
	}

	if(body_len > 0){
		sendSerialResponse(cmd_info, res_type, res_body_bytes, body_len);
	}

	setStatus();

	return 0;
}

int PowerGateDevice::individualControl(int sub_id, bool value)
{
	trace("individualControl: %x, %d", sub_id, value);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			m_statuses[i].m_power = value;
		}
	}

	return 0;
}

int PowerGateDevice::allControl(int sub_id, bool value)
{
	trace("allControl: %x, %x", sub_id, value);

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			m_statuses[i].m_power = value;
		}
	}

	return 0;
}

