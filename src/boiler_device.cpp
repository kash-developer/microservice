
#include "boiler_device.h"
#include "trace.h"
#include "tools.h"

BoilerDevice::BoilerDevice() : HomeDevice()
{
}

BoilerDevice::~BoilerDevice()
{
	stop();
}

int BoilerDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)

{
	Json::Value tmp_json_obj;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	struct BoilerStatus status;

	trace("in boiler init.");

	if(HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0){
		return -1;
	}

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		status.m_sub_id = m_sub_ids[i];
		status.m_error = 0;
		status.m_outgoing = false;
		status.m_heating = false;
		status.m_setting_temperature = 22.5;
		status.m_cur_temperature = 18.5;

		m_statuses.push_back(status);
	}

	m_characteristic.m_version = -1;
	m_characteristic.m_company_code = -1;
	m_characteristic.m_max_temperature = 30;
	m_characteristic.m_min_temperature = 10;
	m_characteristic.m_decimal_point_flag = false;
	m_characteristic.m_outgoing_mode_flag = false;

	if(conf_json[m_device_name].type() == Json::objectValue){
		tmp_json_obj = conf_json[m_device_name];

		if (tmp_json_obj["Characteristics"].type() == Json::objectValue) {
			tmp_json_obj = tmp_json_obj["Characteristics"];

			if (tmp_json_obj["Version"].type() == Json::intValue) {
				m_characteristic.m_version = tmp_json_obj["Version"].asInt();
			}
			if (tmp_json_obj["CompanyCode"].type() == Json::intValue) {
				m_characteristic.m_company_code = tmp_json_obj["CompanyCode"].asInt();
			}
			if (tmp_json_obj["MaxTemperature"].type() == Json::intValue) {
				m_characteristic.m_max_temperature = tmp_json_obj["MaxTemperature"].asInt();
			}
			if (tmp_json_obj["MinTemperature"].type() == Json::intValue) {
				m_characteristic.m_min_temperature = tmp_json_obj["MinTemperature"].asInt();
			}
			if (tmp_json_obj["DecimalPoint"].type() == Json::booleanValue) {
				m_characteristic.m_decimal_point_flag = tmp_json_obj["DecimalPoint"].asBool();
			}
			if (tmp_json_obj["OutgoingMode"].type() == Json::booleanValue) {
				m_characteristic.m_outgoing_mode_flag = tmp_json_obj["OutgoingMode"].asBool();
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

		obj_char["Name"] = "MaxTemperature";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_max_temperature;
		obj_chars.append(obj_char);

		obj_char["Name"] = "MinTemperature";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_min_temperature;
		obj_chars.append(obj_char);

		obj_char["Name"] = "DecimalPoint";
		obj_char["Type"] = "boolean";
		obj_char["Value"] = m_characteristic.m_decimal_point_flag;
		obj_chars.append(obj_char);

		obj_char["Name"] = "OutgoingMode";
		obj_char["Type"] = "boolean";
		obj_char["Value"] = m_characteristic.m_outgoing_mode_flag;
		obj_chars.append(obj_char);
	
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

void BoilerDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
}

void BoilerDevice::setStatus()
{
	Json::Value devices, device, sub_devices, sub_device, statuses, status;

	HomeDevice::setStatus();

	sub_devices.clear();
	devices.clear();

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		statuses.clear();

		status["Name"] = "Error";
		status["Type"] = "integer";
		status["Value"] = m_statuses[i].m_error;
		statuses.append(status);

		status["Name"] = "Outgoing";
		status["Type"] = "boolean";
		status["Value"] = m_statuses[i].m_outgoing;
		statuses.append(status);

		status["Name"] = "Heating";
		status["Type"] = "boolean";
		status["Value"] = m_statuses[i].m_heating;
		statuses.append(status);

		status["Name"] = "SettingTemperature";
		status["Type"] = "number";
		status["Value"] = m_statuses[i].m_setting_temperature;
		statuses.append(status);

		status["Name"] = "CurrentTemperature";
		status["Type"] = "number";
		status["Value"] = m_statuses[i].m_cur_temperature;
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

int BoilerDevice::run()
{
	return HomeDevice::run();
}

void BoilerDevice::stop()
{
	HomeDevice::stop();
}

int BoilerDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	Json::Value::const_iterator it;

	std::string cmd;
	Json::Value params;
	int tmp_value;
	Json::Value ret_param, ret_params, sub_device, control;
	int int_cmd;

	if(HomeDevice::processHttpCommand(device_id, sub_id, cmd_obj, res_obj) < 0){
		tracee("process http request failed.");
		res_obj = createErrorResponse(ERROR_CODE_INTERNAL_ERROR);
		return -1;
	}

	if (cmd_obj["CommandType"].type() != Json::stringValue) {
		res_obj = createErrorResponse(ERROR_CODE_INVALID_COMMAND_TYPE);
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

	if((cmd.compare("HeatingControl") == 0) || (int_cmd == 0x43)) {
		if(heatingControl(sub_id, params["Heating"]["Value"].asBool()) < 0){
			tracee("set heating control failed.");
			res_obj = createErrorResponse(ERROR_CODE_INTERNAL_ERROR);
			return -1;
		}
	}
	else if((cmd.compare("SetTemperatureControl") == 0) || (int_cmd == 0x44)) {
		tmp_value = params["SettingTemperature"]["Value"].asInt();
		if(params["SettingTemperature"]["Value"].asDouble() - tmp_value > 0){
			tmp_value |= 0x80;
		}

		if(setTemperatureControl(sub_id, tmp_value) < 0){
			tracee("set set temperature control failed.");
			res_obj = createErrorResponse(ERROR_CODE_INTERNAL_ERROR);
			return -1;
		}
	}
	else if((cmd.compare("SetOutGoingModeControl") == 0) || (int_cmd == 0x45)) {
		if(setOutGoingModeControl(sub_id, params["Outgoing"]["Value"].asBool()) < 0){
			tracee("set set outgoing control failed.");
			res_obj = createErrorResponse(ERROR_CODE_INTERNAL_ERROR);
			return -1;
		}
	}
	else{
		tracee("invalid control: %s", cmd.c_str());
		res_obj = createErrorResponse(ERROR_CODE_INVALID_COMMAND_TYPE);
		return -1;
	}

	setStatus();

	res_obj = Json::arrayValue;
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			control.clear();
			ret_param.clear();
			ret_params.clear();
			if ((cmd.compare("HeatingControl") == 0) || (int_cmd == 0x43)) {
				ret_param.clear();
				ret_param["Name"] = "Heating";
				ret_param["Value"] = m_statuses[i].m_heating;
				ret_param["Type"] = "boolean";
				ret_params.append(ret_param);

				control["CommandType"] = "0xc3";
				control["Parameters"] = ret_params;
			}
			else if ((cmd.compare("SetTemperatureControl") == 0) || (int_cmd == 0x44)) {
				ret_param.clear();
				ret_param["Name"] = "SettingTemperature";
				ret_param["Value"] = m_statuses[i].m_setting_temperature;
				ret_param["Type"] = "number";
				ret_params.append(ret_param);

				control["CommandType"] = "0xc4";
				control["Parameters"] = ret_params;
			}
			else if ((cmd.compare("SetOutGoingModeControl") == 0) || (int_cmd == 0x45)) {
				ret_param.clear();
				ret_param["Name"] = "Outgoing";
				ret_param["Value"] = m_statuses[i].m_outgoing;
				ret_param["Type"] = "boolean";
				ret_params.append(ret_param);

				control["CommandType"] = "0xc5";
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

int BoilerDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processBoilerSerialCommand(&cmd_info);
}


int BoilerDevice::processBoilerSerialCommand(SerialCommandInfo* cmd_info)
{
	int body_len;
	int res_type;
	uint8_t res_body_bytes[255];

	memset(res_body_bytes, 0, 255);
	res_type = 0;
	body_len = 0;

	//request status info
	if(cmd_info->m_command_type == 0x01){
	}
	//chractoristic info
	else if(cmd_info->m_command_type == 0x0f){
		res_type = 0x8f;
		body_len = 11;

		res_body_bytes[0] = m_characteristic.m_version;
		res_body_bytes[1] = m_characteristic.m_company_code;
		res_body_bytes[2] = 0x00;
		res_body_bytes[3] = m_characteristic.m_max_temperature;
		res_body_bytes[4] = m_characteristic.m_min_temperature;
		res_body_bytes[5] = m_characteristic.m_decimal_point_flag?0x10:0x00;
		res_body_bytes[5] |= m_characteristic.m_outgoing_mode_flag?0x02:0x00;
		res_body_bytes[6] = 0x00;
		res_body_bytes[7] = 0x00;
		res_body_bytes[8] = 0x00;
		res_body_bytes[9] = 0x00;
		res_body_bytes[10] = 0x00;
	}
	else if(cmd_info->m_command_type == 0x43){
		heatingControl(cmd_info->m_sub_id, cmd_info->m_data[0]);
	}
	else if(cmd_info->m_command_type == 0x44){
		setTemperatureControl(cmd_info->m_sub_id, cmd_info->m_data[0]);
	}
	else if(cmd_info->m_command_type == 0x45){
		setOutGoingModeControl(cmd_info->m_sub_id, cmd_info->m_data[0]);
	}
	else{
		tracee("invalid control: %x", cmd_info->m_command_type);
		return -1;
	}

	if((cmd_info->m_command_type != 0x0f) && ((cmd_info->m_sub_id & 0x0f) != 0x0f)){
		res_type = cmd_info->m_command_type + 0x80;

		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if (cmd_info->m_sub_id == m_statuses[i].m_sub_id) {
				res_body_bytes[0] = m_statuses[i].m_error;
				res_body_bytes[1] |= m_statuses[i].m_outgoing ? 0x02 : 0x00;
				res_body_bytes[1] |= m_statuses[i].m_heating ? 0x01 : 0x00;
				res_body_bytes[2] = int(m_statuses[i].m_setting_temperature);
				if (m_statuses[i].m_setting_temperature - int(m_statuses[i].m_setting_temperature) > 0) {
					res_body_bytes[2] |= 0x80;
				}
				res_body_bytes[3] = int(m_statuses[i].m_cur_temperature);
				if (m_statuses[i].m_cur_temperature - int(m_statuses[i].m_cur_temperature) > 0) {
					res_body_bytes[3] |= 0x80;
				}
				res_body_bytes[4] = 0x00;
				body_len = 5;

				break;
			}
		}
	}

	if(body_len > 0){
		sendSerialResponse(cmd_info, res_type, res_body_bytes, body_len);
	}

	setStatus();

	return 0;
}

int BoilerDevice::heatingControl(int sub_id, bool heating)
{
	int ret = -1;

	trace("heatingControl: %x, %d", sub_id, heating);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			m_statuses[i].m_heating = heating;
			ret = 0;
		}
	}

	return ret;
}

int BoilerDevice::setTemperatureControl(int sub_id, int temperature)
{
	int ret = -1;
	int new_temperature;
	double point;

	trace("setTemperatureControl: %d", temperature);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			point = 0;
			new_temperature = temperature & 0x7f;
			if (new_temperature > m_characteristic.m_max_temperature) {
				new_temperature = m_characteristic.m_max_temperature;
			}
			else if (new_temperature < m_characteristic.m_min_temperature) {
				new_temperature = m_characteristic.m_min_temperature;
			}
			else {
				if (temperature & 0x80) {
					point = 0.5;
				}
			}

			m_statuses[i].m_setting_temperature = new_temperature;
			m_statuses[i].m_setting_temperature += point;
			ret = 0;
		}
	}

	return ret;
}

int BoilerDevice::setOutGoingModeControl(int sub_id, bool outgoing_mode)
{
	int ret = -1;

	trace("setOutGoingModeControl: %d", outgoing_mode);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			if (m_characteristic.m_outgoing_mode_flag == false) {
				tracee("the boiler doesnot support outgoing mode.");
				return -1;
			}

			m_statuses[i].m_outgoing = outgoing_mode;
			ret = 0;
		}
	}

	return ret;
}




