
#include "system_aircon_device.h"
#include "trace.h"
#include "tools.h"

SystemAirconDevice::SystemAirconDevice() : HomeDevice()
{
}

SystemAirconDevice::~SystemAirconDevice()
{
	stop();
}

int SystemAirconDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	struct SystemAirconStatus status;

	trace("in system aircon init.");

	if (HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0) {
		return -1;
	}

	for(unsigned int i=0; i<m_sub_ids.size(); i++){
		status.m_sub_id = m_sub_ids[i];
		status.m_power = false;
		status.m_wind_dir = 0x00;
		status.m_wind_vol = 0x00;
		status.m_setting_temperature = 18;
		status.m_cur_temperature = 18;

		m_statuses.push_back(status);
	}

	m_characteristic.m_decimal_point = false;
	m_characteristic.m_cooling_min_temperature = 18;
	m_characteristic.m_cooling_max_temperature = 30;
	m_characteristic.m_indoor_unit_number = (uint8_t)m_sub_ids.size();

	if(conf_json[m_device_name].type() == Json::objectValue){
		tmp_json_obj = conf_json[m_device_name];

		if (tmp_json_obj["Characteristic"].type() == Json::objectValue) {
			tmp_json_obj = tmp_json_obj["Characteristic"];

			if (tmp_json_obj["Version"].type() == Json::intValue) {
				m_characteristic.m_version = tmp_json_obj["Version"].asInt();
			}
			if (tmp_json_obj["CompanyCode"].type() == Json::intValue) {
				m_characteristic.m_company_code = tmp_json_obj["CompanyCode"].asInt();
			}
			if (tmp_json_obj["DecimalPoint"].type() == Json::booleanValue) {
				m_characteristic.m_decimal_point = tmp_json_obj["DecimalPoint"].asBool();
			}
			if (tmp_json_obj["CoolingMaxTemperature"].type() == Json::intValue) {
				m_characteristic.m_cooling_max_temperature = tmp_json_obj["CoolingMaxTemperature"].asInt();
			}
			if (tmp_json_obj["CoolingMinTemperature"].type() == Json::intValue) {
				m_characteristic.m_cooling_min_temperature = tmp_json_obj["CoolingMinTemperature"].asInt();
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

		obj_char["Name"] = "IndoorUnitNumber";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_indoor_unit_number;
		obj_chars.append(obj_char);

		obj_char["Name"] = "CoolingMaxTemperature";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_cooling_max_temperature;
		obj_chars.append(obj_char);

		obj_char["Name"] = "CoolingMinTemperature";
		obj_char["Type"] = "integer";
		obj_char["Value"] = m_characteristic.m_cooling_min_temperature;
		obj_chars.append(obj_char);

		obj_char["Name"] = "DecimalPoint";
		obj_char["Type"] = "boolean";
		obj_char["Value"] = m_characteristic.m_decimal_point;
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

void SystemAirconDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
}

void SystemAirconDevice::setStatus()
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

		status["Name"] = "WindDir";
		status["Type"] = "integer";
		status["Value"] = m_statuses[i].m_wind_dir;
		statuses.append(status);

		status["Name"] = "WindVol";
		status["Type"] = "integer";
		status["Value"] = m_statuses[i].m_wind_vol;
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

int SystemAirconDevice::run()
{
	return HomeDevice::run();
}

void SystemAirconDevice::stop()
{
	HomeDevice::stop();
}

int SystemAirconDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	std::string cmd;
	Json::Value params;
	int tmp_value;
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

	if((cmd.compare("SetPowerControl") == 0) || (int_cmd == 0x43)) {
		if(setPowerControl(sub_id, params["Power"]["Value"].asBool()) < 0){
			tracee("set power control failed.");
			return -1;
		}
	}
	else if((cmd.compare("SetTemperatureControl") == 0) || (int_cmd == 0x44)) {
		tmp_value = params["SettingTemperature"]["Value"].asInt();
		if(params["SettingTemperature"]["Value"].asDouble() - tmp_value > 0){
			tmp_value |= 0x80;
		}

		if(setTemperatureControl(sub_id, tmp_value) < 0){
			tracee("set temperature control failed.");
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
			if ((cmd.compare("SetPowerControl") == 0) || (int_cmd == 0x43)) {
				ret_param.clear();
				ret_param["Name"] = "Power";
				ret_param["Value"] = m_statuses[i].m_power;
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

int SystemAirconDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processSystemAirconSerialCommand(&cmd_info);
}

int SystemAirconDevice::processSystemAirconSerialCommand(SerialCommandInfo* cmd_info)
{
	int body_len;
	int res_type, idx;
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
		body_len = 0x0b;

		res_body_bytes[0] = m_characteristic.m_version;
		res_body_bytes[1] |= m_characteristic.m_company_code;
		res_body_bytes[2] |= m_characteristic.m_indoor_unit_number;
		res_body_bytes[4] = m_characteristic.m_cooling_max_temperature;
		res_body_bytes[5] = m_characteristic.m_cooling_min_temperature;
	}
	else if(cmd_info->m_command_type == 0x43){
		if(setPowerControl(cmd_info->m_sub_id, cmd_info->m_data[0]) < 0){
			tracee("set power control failed.");
		}
	}
	else if(cmd_info->m_command_type == 0x44){
		if(setTemperatureControl(cmd_info->m_sub_id, cmd_info->m_data[0]) < 0){
			tracee("set temperature control failed.");
		}
	}
	else{
		tracee("invalid control: %x", cmd_info->m_command_type);
		return -1;
	}

	if(cmd_info->m_command_type != 0x0f){
		res_type = cmd_info->m_command_type | 0x80;

		idx = 0;
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if ((cmd_info->m_sub_id == m_statuses[i].m_sub_id) || ((cmd_info->m_sub_id & 0x0f) == 0x0f)) {
				res_body_bytes[i * 5 + 0] = 0;
				res_body_bytes[i * 5 + 1] |= m_statuses[i].m_power ? 0x10 : 0x00;
				res_body_bytes[i * 5 + 2] |= m_statuses[i].m_wind_dir << 4;
				res_body_bytes[i * 5 + 2] |= m_statuses[i].m_wind_vol;
				res_body_bytes[i * 5 + 3] = int(m_statuses[i].m_setting_temperature);
				if (int(m_statuses[i].m_setting_temperature) < m_statuses[i].m_setting_temperature) {
					res_body_bytes[i * 5 + 3] |= 0x80;
				}
				res_body_bytes[i * 5 + 4] = int(m_statuses[i].m_cur_temperature);
				if (int(m_statuses[i].m_cur_temperature) < m_statuses[i].m_cur_temperature) {
					res_body_bytes[i * 5 + 4] |= 0x80;
				}
				idx++;
			}
		}
		body_len = idx * 5;

		//no such sub_id
		if (idx == 0) {
			body_len = 0;
		}
	}

	if(body_len > 0){
		sendSerialResponse(cmd_info, res_type, res_body_bytes, body_len);
	}

	setStatus();

	return 0;
}

int SystemAirconDevice::setPowerControl(int sub_id, bool power)
{
	trace("setPowerControl: %x, %d", sub_id, power);
	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			m_statuses[i].m_power = power;
		}
	}

	return 0;
}

int SystemAirconDevice::setTemperatureControl(int sub_id, int temperature)
{
	double new_temperature;
	double point;

	trace("setTemperatureControl: %x, %x, %d, %d", sub_id, temperature, temperature & 0x7f, temperature & 0x80);

	point = 0;
	new_temperature = temperature & 0x7f;
	if ((temperature & 0x80) != 0) {
		trace("add 0.5");
		new_temperature += 0.5;
	}
	else {
		trace("no add: %x", temperature & 0x80);
	}

	if (new_temperature > m_characteristic.m_cooling_max_temperature) {
		new_temperature = m_characteristic.m_cooling_max_temperature;
	}
	else if (new_temperature < m_characteristic.m_cooling_min_temperature) {
		new_temperature = m_characteristic.m_cooling_min_temperature;
	}

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			m_statuses[i].m_setting_temperature = new_temperature;
		}
	}

	return 0;
}


