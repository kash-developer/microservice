
#include "curtain_device.h"
#include "trace.h"
#include "tools.h"

#ifdef WIN32
#else
#include <unistd.h>
#endif

#include <time.h>

CurtainDevice::CurtainDevice() : HomeDevice()
{
}

CurtainDevice::~CurtainDevice()
{
	stop();
}

int CurtainDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj;
	Json::Value tmp_json_obj2;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	struct CurtainStatus status;

	trace("in curtain init.");

	if (HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0) {
		return -1;
	}

	status.m_status = 
	status.m_cur_open = 0x00;
	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		status.m_sub_id = m_sub_ids[i];
		status.m_status = CURTAIN_OPERATION_CLOSED;
		status.m_cur_open = 0;
		status.m_op = 0;
		status.m_last_update = 0;

		m_statuses.push_back(status);
	}

	m_statuses.push_back(status);

	m_characteristic.m_version = -1;
	m_characteristic.m_company_code = -1;

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
			if (tmp_json_obj["Characteristic"].type() == Json::intValue) {
				m_characteristic.m_characteristic = tmp_json_obj["Characteristic"].asInt();
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

		obj_char["Name"] = "Characteristic";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_characteristic;
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

void CurtainDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
}

void CurtainDevice::setStatus()
{
	Json::Value devices, device, sub_devices, sub_device, statuses, status;

	updateValues();
	HomeDevice::setStatus();

	sub_devices.clear();
	devices.clear();

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		statuses.clear();

		status["Name"] = "Status";
		status["Type"] = "integer";
		status["Value"] = m_statuses[i].m_status;
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

int CurtainDevice::run()
{
	return HomeDevice::run();
}

void CurtainDevice::stop()
{
	HomeDevice::stop();
}

int CurtainDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
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
	updateValues();

	if ((cmd.compare("IndividualControl") == 0) || (int_cmd == 0x41)) {
		if (doOperation(sub_id, params["Operation"]["Value"].asInt()) < 0) {
			trace("operate curtain failed.");
			res_obj = createErrorResponse(ERROR_CODE_INTERNAL_ERROR);
			return -1;
		}
	}
	else if ((cmd.compare("AllControl") == 0) || (int_cmd == 0x42)) {
		if (doOperation(sub_id, params["Operation"]["Value"].asInt()) < 0) {
			trace("operate curtain failed.");
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
			if ((cmd.compare("IndividualControl") == 0) || (cmd.compare("0x41") == 0)) {
				ret_param.clear();
				ret_param["Name"] = "Status";
				ret_param["Value"] = m_statuses[0].m_status;
				ret_param["Type"] = "integer";
				ret_params.append(ret_param);

				control["CommandType"] = "0xc1";
				control["Parameters"] = ret_params;
			}
			else if ((cmd.compare("AllControl") == 0) || (cmd.compare("0x42") == 0)) {
				ret_param.clear();
				ret_param["Name"] = "Status";
				ret_param["Value"] = m_statuses[0].m_status;
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

int CurtainDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	trace("in process command...");
	updateValues();

	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processCurtainSerialCommand(&cmd_info);
}

int CurtainDevice::processCurtainSerialCommand(SerialCommandInfo* cmd_info)
{
	int body_len;
	int res_type;
	uint8_t res_body_bytes[255];

	memset(res_body_bytes, 0, 255);
	res_type = cmd_info->m_command_type | 0x80;
	body_len = 0;

	//request doorlock status info
	if(cmd_info->m_command_type == 0x01){
		res_body_bytes[0] = 0x00;
		if(m_statuses[0].m_status == CURTAIN_OPERATION_CLOSED){
			res_body_bytes[1] = 0x02;
		}
		else if (m_statuses[0].m_status == CURTAIN_OPERATION_OPENED) {
			res_body_bytes[1] = 0x01;
		}
		else if (m_statuses[0].m_status == CURTAIN_OPERATION_CLOSING) {
			res_body_bytes[1] = 0x04;
		}
		else if (m_statuses[0].m_status == CURTAIN_OPERATION_OPENING) {
			res_body_bytes[1] = 0x08;
		}
		else{
			res_body_bytes[1] = 0x00;
		}

		body_len = 3;
	}
	//request doorlock chractoristic info
	else if(cmd_info->m_command_type == 0x0f){
		res_body_bytes[0] = m_characteristic.m_version;
		res_body_bytes[1] = m_characteristic.m_company_code;
		res_body_bytes[2] = m_characteristic.m_characteristic;
		body_len = 3;
	}
	//request curtain control: on/off
	else if(cmd_info->m_command_type == 0x41){
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if (cmd_info->m_sub_id == m_statuses[i].m_sub_id) {
				doOperation(cmd_info->m_sub_id, cmd_info->m_data[0], 0, 0);

				res_body_bytes[0] = 0x00;
				if (m_statuses[0].m_status == CURTAIN_OPERATION_CLOSED) {
					res_body_bytes[1] = 0x02;
				}
				else if (m_statuses[0].m_status == CURTAIN_OPERATION_OPENED) {
					res_body_bytes[1] = 0x01;
				}
				else if (m_statuses[0].m_status == CURTAIN_OPERATION_CLOSING) {
					res_body_bytes[1] = 0x04;
				}
				else if (m_statuses[0].m_status == CURTAIN_OPERATION_OPENING) {
					res_body_bytes[1] = 0x08;
				}
				else {
					res_body_bytes[1] = 0x00;
				}

				body_len = 3;
				break;
			}
		}
	}
	//request all curtaincontrol: on/off
	else if(cmd_info->m_command_type == 0x42){
		doOperation(cmd_info->m_sub_id, cmd_info->m_data[0], 0, 0);

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

int CurtainDevice::doOperation(int sub_id, int cmd, int angle_value, int open_value)
{
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if ((sub_id == m_statuses[i].m_sub_id) || ((sub_id & 0x0f) == 0x0f)) {
			if (cmd == 0) {
				if (m_statuses[i].m_status == CURTAIN_OPERATION_CLOSING) {
					return 0;
				}
				else if (m_statuses[i].m_status == CURTAIN_OPERATION_CLOSED) {
					return 0;
				}
				m_statuses[i].m_status = CURTAIN_OPERATION_CLOSING;
			}
			else if (cmd == 1) {
				if (m_statuses[i].m_status == CURTAIN_OPERATION_OPENING) {
					return 0;
				}
				if (m_statuses[i].m_status == CURTAIN_OPERATION_OPENED) {
					return 0;
				}
				m_statuses[i].m_status = CURTAIN_OPERATION_OPENING;
			}
			else if (cmd == 2) {
				if (m_statuses[i].m_status == CURTAIN_OPERATION_CLOSED) {
					return 0;
				}
				if (m_statuses[i].m_status == CURTAIN_OPERATION_OPENED) {
					return 0;
				}
			}

			m_statuses[i].m_last_update = time(NULL);
			m_statuses[i].m_op = cmd;

			trace("op %x: %d, angle: %d, open: %d", sub_id, cmd, angle_value, open_value);
		}
	}
	updateValues();

	return 0;
}

void CurtainDevice::updateValues()
{
	int t_diff;
	time_t cur;

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		cur = time(NULL);
		//trace("cur: %d", cur);
		t_diff = int(cur - m_statuses[i].m_last_update);
		//trace("last: %d", m_statuses[i].m_last_update);
		//trace("t_diff: %d", t_diff);
		//trace("open: %d", m_statuses[i].m_cur_open);

		m_statuses[i].m_last_update = time(NULL);

		if (m_statuses[i].m_op == 0) {
			m_statuses[i].m_cur_open -= t_diff;

			if (m_statuses[i].m_cur_open <= 0) {
				m_statuses[i].m_op = 3;
				m_statuses[i].m_cur_open = 0;
				m_statuses[i].m_status = CURTAIN_OPERATION_CLOSED;
				trace("closed.");
			}
		}
		else if (m_statuses[i].m_op == 1) {
			m_statuses[i].m_cur_open += t_diff;

			if (m_statuses[i].m_cur_open > CURTAIN_MAX_OPEN_VALUE) {
				m_statuses[i].m_op = 3;
				m_statuses[i].m_cur_open = CURTAIN_MAX_OPEN_VALUE;
				m_statuses[i].m_status = CURTAIN_OPERATION_OPENED;
			}
			trace("opened.");
		}
		else if (m_statuses[i].m_op == 2) {
			m_statuses[i].m_op = 3;
			m_statuses[i].m_status = CURTAIN_OPERATION_STOPPED;
			trace("stopped.");
		}
		else {
			;
		}
		trace("openvalue %x: %d, op: %d", m_statuses[i].m_sub_id, m_statuses[i].m_cur_open, m_statuses[i].m_op);
	}
}

int CurtainDevice::setAngleValue(int sub_id, int value)
{
	return 0;
}

int CurtainDevice::setOpenValue(int sub_id, int value)
{
	return 0;
}


