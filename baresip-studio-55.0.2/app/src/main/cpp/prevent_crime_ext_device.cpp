
#include "prevent_crime_ext_device.h"
#include "trace.h"
#include "tools.h"

PreventCrimeExtDevice::PreventCrimeExtDevice() : HomeDevice()
{
}

PreventCrimeExtDevice::~PreventCrimeExtDevice()
{
	stop();
}

int PreventCrimeExtDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj, tmp_json_obj2, tmp_json_obj3;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char, obj_capable_set_types;
	struct PreventCrimeExtStatus status;

	trace("in prevent crime ext. controller init.");

	if (HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0) {
		return -1;
	}

	for (unsigned int i = 0; i < m_sub_ids.size(); i++) {
		status.m_sub_id = m_sub_ids[i];

		for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
			status.m_set[j] = 0x00;
			status.m_type[j] = 0x00;
			status.m_status[j] = 0x00;
		}
		m_statuses.push_back(status);

		//only one device can be exist.
		break;
	}

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
			if (tmp_json_obj["CapableSetType"].type() == Json::arrayValue) {
				tmp_json_obj2 = tmp_json_obj["CapableSetType"];
				trace("capable sets: %s", tmp_json_obj2.toStyledString().c_str());
				for (unsigned int i = 0; i < tmp_json_obj2.size(); i++) {
					if (PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER <= (int)i) {
						break;
					}
					m_characteristic.m_capable_set_type[i] = tmp_json_obj2[i].asBool();
				}
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

		obj_char["Name"] = "CapableSetType";
		obj_char["Type"] = "booleanarray";
		obj_capable_set_types.clear();
		for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
			obj_capable_set_types.append(m_characteristic.m_capable_set_type[j]);
		}
		obj_char["Value"] = obj_capable_set_types;
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

void PreventCrimeExtDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
}

void PreventCrimeExtDevice::setStatus()
{
	Json::Value devices, device, sub_devices, sub_device, statuses, status, obj_values;
	std::string tmp_string;

	HomeDevice::setStatus();

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		status["Name"] = "SensorSet";
		status["Type"] = "booleanarray";
		obj_values.clear();
		for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
			obj_values.append(m_statuses[i].m_set[j]);
		}
		status["Value"] = obj_values;
		statuses.append(status);

		status["Name"] = "SensorType";
		status["Type"] = "integerarray";
		obj_values.clear();
		for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
			obj_values.append(m_statuses[i].m_type[j]);
		}
		status["Value"] = obj_values;
		statuses.append(status);

		status["Name"] = "SensorStatus";
		status["Type"] = "integerarray";
		obj_values.clear();
		for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
			obj_values.append(m_statuses[i].m_status[j]);
		}
		status["Value"] = obj_values;
		statuses.append(status);

		sub_device["SubDeviceID"] = int2hex_str(m_sub_ids[i]);
		sub_device["Status"] = statuses;

		sub_devices.append(sub_device);
	}

	device["DeviceID"] = int2hex_str(m_device_id);
	device["SubDeviceList"] = sub_devices;
	devices.append(device);
	m_obj_status["DeviceList"] = devices;

	trace("set status: \n%s", m_obj_status.toStyledString().c_str());
}

int PreventCrimeExtDevice::run()
{
	return HomeDevice::run();
}

void PreventCrimeExtDevice::stop()
{
	HomeDevice::stop();
}

int PreventCrimeExtDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	std::string cmd;
	Json::Value params;
	uint8_t param_values[2];
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

	if((cmd.compare("SetSensorSetControl") == 0) || (int_cmd == 0x43)) {
		param_values[0] = 0;
		for(unsigned int i=0; i<params["SensorSet"]["Value"].size(); i++){
			param_values[0] |= params["SensorSet"]["Value"][i].asBool()?0x01<<i:0x00;
		}

		if(setSensorSetControl(sub_id, param_values[0]) < 0){
			tracee("set sensor satus failed.");
			return -1;
		}
	}
	else if((cmd.compare("SetSensorTypeControl") == 0) || (int_cmd == 0x44)) {
		param_values[0] = param_values[1] = 0;
		for(unsigned int i=0; i<params["SensorTypes"]["Value"].size(); i++){
			if(i>3){
				param_values[0] |= (params["SensorTypes"]["Value"][i].asInt() & 0x03) << ((i-4)*2);
			}
			else{
				param_values[1] |= (params["SensorTypes"]["Value"][i].asInt() & 0x03) << i*2;
			}
		}

		if(setSensorTypeControl(sub_id, param_values) < 0){
			tracee("set sensor satus failed.");
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
			if ((cmd.compare("SetSensorSetControl") == 0) || (int_cmd == 0x43)) {
				ret_param.clear();
				ret_param["Name"] = "SensorSet";
				ret_param["Value"] = Json::Value(Json::arrayValue);
				ret_param["Type"] = "booleanarray";

				for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
					ret_param["Value"].append(m_statuses[j].m_set);
				}
				ret_params.append(ret_param);

				ret_param.clear();
				ret_param["Name"] = "SensorTypes";
				ret_param["Value"] = Json::Value(Json::arrayValue);
				ret_param["Type"] = "integerarray";

				for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
					ret_param["Value"].append(m_statuses[j].m_type);
				}
				ret_params.append(ret_param);

				control["CommandType"] = "0xc3";
				control["Parameters"] = ret_params;
			}
			else if ((cmd.compare("SetSensorTypeControl") == 0) || (int_cmd == 0x44)) {
				ret_param.clear();
				ret_param["Name"] = "SensorSet";
				ret_param["Value"] = Json::Value(Json::arrayValue);
				ret_param["Type"] = "booleanarray";

				for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
					ret_param["Value"].append(m_statuses[j].m_set);
				}
				ret_params.append(ret_param);

				ret_param.clear();
				ret_param["Name"] = "SensorTypes";
				ret_param["Value"] = Json::Value(Json::arrayValue);
				ret_param["Type"] = "integerarray";

				for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
					ret_param["Value"].append(m_statuses[j].m_type);
				}
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

int PreventCrimeExtDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processPreventCrimeExtSerialCommand(&cmd_info);
}

int PreventCrimeExtDevice::processPreventCrimeExtSerialCommand(SerialCommandInfo* cmd_info)
{
	int body_len;
	int res_type;
	uint8_t res_body_bytes[255];
	uint8_t param_values[2];

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
		res_body_bytes[2] = 0;
		for(int i=0; i<PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; i++){
			res_body_bytes[2] |= m_characteristic.m_capable_set_type[i] ? 0x01 << i : 0x00;
		}
		res_body_bytes[3] = 0x00;
		res_body_bytes[4] = 0x00;
		res_body_bytes[5] = 0x00;
		res_body_bytes[6] = 0x00;
		res_body_bytes[7] = 0x00;
		res_body_bytes[8] = 0x00;
		res_body_bytes[9] = 0x00;
		res_body_bytes[10] = 0x00;
	}
	else if(cmd_info->m_command_type == 0x43){
		if(setSensorSetControl(cmd_info->m_sub_id, cmd_info->m_data[0]) < 0){
			tracee("set sensor status control failed.");
		}
	}
	else if(cmd_info->m_command_type == 0x44){
		param_values[0] = cmd_info->m_data[0];
		param_values[1] = cmd_info->m_data[1];
		if(setSensorTypeControl(cmd_info->m_sub_id, param_values) < 0){
			tracee("set sensor type control failed.");
		}
	}
	else{
		tracee("invalid control: %x", cmd_info->m_command_type);
		return -1;
	}

	if(cmd_info->m_command_type != 0x0f){
		res_type = cmd_info->m_command_type + 0x80;
		body_len = 6;

		res_body_bytes[0] = 0;
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if (cmd_info->m_sub_id == m_statuses[i].m_sub_id) {
				for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
					res_body_bytes[1] |= m_statuses[i].m_set[j] ? 0x01 << j : 0x00;

					if (j > 3) {
						res_body_bytes[2] |= m_statuses[i].m_type[j] << ((j - 4) * 2);
						res_body_bytes[4] |= m_statuses[i].m_status[j] << ((j - 4) * 2);
					}
					else {
						res_body_bytes[3] |= m_statuses[i].m_type[j] << (j * 2);
						res_body_bytes[5] |= m_statuses[i].m_status[j] << (j * 2);
					}
				}
			}
		}
	}

	if(body_len > 0){
		sendSerialResponse(cmd_info, res_type, res_body_bytes, body_len);
	}

	setStatus();

	return 0;
}

int PreventCrimeExtDevice::setSensorSetControl(int sub_id, uint8_t sets)
{
	trace("setSensorSetControl: %x", sets);

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
				m_statuses[i].m_set[j] = sets & (0x01 << j) ? true : false;
				trace("set sensor set %x: %d: %x (%x)", sub_id, j, m_statuses[i].m_set[j], sets & (0x01 << j));
			}
		}
	}

	return 0;
}

int PreventCrimeExtDevice::setSensorTypeControl(int sub_id, uint8_t* types)
{
	int value;

	trace("setSensorTypeControl: %x, %x", types[0], types[1]);

	for (unsigned int i = 0; i < m_statuses.size(); i++) {
		if (((sub_id & 0x0f) == 0x0f) || (sub_id == m_statuses[i].m_sub_id)) {
			for (int j = 0; j < PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER; j++) {
				if (j > 3) {
					value = types[0] & (0x03 << ((j - 4) * 2));
					value = value >> ((j - 4) * 2);
				}
				else {
					value = types[1] & (0x03 << j * 2);
					value = value >> (j * 2);
				}

				if ((value != 0x00) && (value != 0x01) && (value != 0x2)) {
					tracee("invalid type value: %x", value);
					return -1;
				}
				m_statuses[i].m_type[j] = value;
				trace("set sensor type %x: %d: %x", sub_id, j, value);
			}
		}
	}

	return 0;
}




