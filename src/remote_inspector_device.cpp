
#include "remote_inspector_device.h"
#include "trace.h"
#include "tools.h"

#ifdef WIN32
#else
#include <unistd.h>
#endif

RemoteInspectorDevice::RemoteInspectorDevice() : HomeDevice()
{
}

RemoteInspectorDevice::~RemoteInspectorDevice()
{
	stop();
}

int RemoteInspectorDevice::init(Json::Value& conf_json, std::vector<int> sub_ids, int http_port, bool use_serial)
{
	Json::Value tmp_json_obj;
	Json::Value tmp_json_obj2;
	Json::Value devices, device, sub_devices, sub_device, obj_chars, obj_char;
	struct RemoteInspectorStatus status;

	trace("in remote inspector init.");

	if (HomeDevice::init(conf_json, sub_ids, http_port, use_serial) < 0) {
		return -1;
	}

	srand((unsigned int)time(NULL));
	for(unsigned int i=0; i<m_sub_ids.size(); i++){
		status.m_sub_id = m_sub_ids[i];

		status.m_cur_value = rand() % 10000;
		status.m_acc_value = status.m_cur_value * 3;

		m_statuses.push_back(status);
	}

	m_characteristic.m_version = -1;
	m_characteristic.m_company_code = -1;

	m_characteristic.m_water_current_length = 8;
	m_characteristic.m_water_accumulated_length = 8;
	m_characteristic.m_water_current_integer_length = 8;
	m_characteristic.m_water_accumulated_integer_length = 8;

	m_characteristic.m_gas_current_length = 8;
	m_characteristic.m_gas_accumulated_length = 8;
	m_characteristic.m_gas_current_integer_length = 8;
	m_characteristic.m_gas_accumulated_integer_length = 8;

	m_characteristic.m_electricity_current_length = 8;
	m_characteristic.m_electricity_accumulated_length = 8;
	m_characteristic.m_electricity_current_integer_length = 8;
	m_characteristic.m_electricity_accumulated_integer_length = 8;

	m_characteristic.m_hot_water_current_length = 8;
	m_characteristic.m_hot_water_accumulated_length = 8;
	m_characteristic.m_hot_water_current_integer_length = 8;
	m_characteristic.m_hot_water_accumulated_integer_length = 8;

	m_characteristic.m_heat_current_length = 8;
	m_characteristic.m_heat_accumulated_length = 8;
	m_characteristic.m_heat_current_integer_length = 8;
	m_characteristic.m_heat_accumulated_integer_length = 8;

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

			if (tmp_json_obj["WaterCurrentLength"].type() == Json::intValue) {
				m_characteristic.m_water_current_length = tmp_json_obj["WaterCurrentLength"].asInt();
			}
			if (tmp_json_obj["WaterAccumulatedLength"].type() == Json::intValue) {
				m_characteristic.m_water_accumulated_length = tmp_json_obj["WaterAccumulatedLength"].asInt();
			}
			if (tmp_json_obj["WaterCurrentIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_water_current_integer_length = tmp_json_obj["WaterCurrentIntegerLength"].asInt();
			}
			if (tmp_json_obj["WaterAccumulatedIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_water_accumulated_integer_length = tmp_json_obj["WaterAccumulatedIntegerLength"].asInt();
			}

			if (tmp_json_obj["GasCurrentLength"].type() == Json::intValue) {
				m_characteristic.m_gas_current_length = tmp_json_obj["GasCurrentLength"].asInt();
			}
			if (tmp_json_obj["GasAccumulatedLength"].type() == Json::intValue) {
				m_characteristic.m_gas_accumulated_length = tmp_json_obj["GasAccumulatedLength"].asInt();
			}
			if (tmp_json_obj["GasCurrentIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_gas_current_integer_length = tmp_json_obj["GasCurrentIntegerLength"].asInt();
			}
			if (tmp_json_obj["GasAccumulatedIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_gas_accumulated_integer_length = tmp_json_obj["GasAccumulatedIntegerLength"].asInt();
			}

			if (tmp_json_obj["ElectricityCurrentLength"].type() == Json::intValue) {
				m_characteristic.m_electricity_current_length = tmp_json_obj["ElectricityCurrentLength"].asInt();
			}
			if (tmp_json_obj["ElectricityAccumulatedLength"].type() == Json::intValue) {
				m_characteristic.m_electricity_accumulated_length = tmp_json_obj["ElectricityAccumulatedLength"].asInt();
			}
			if (tmp_json_obj["ElectricityCurrentIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_electricity_current_integer_length = tmp_json_obj["ElectricityCurrentIntegerLength"].asInt();
			}
			if (tmp_json_obj["ElectricityAccumulatedIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_electricity_accumulated_integer_length = tmp_json_obj["ElectricityAccumulatedIntegerLength"].asInt();
			}

			if (tmp_json_obj["HotWaterCurrentLength"].type() == Json::intValue) {
				m_characteristic.m_hot_water_current_length = tmp_json_obj["HotWaterCurrentLength"].asInt();
			}
			if (tmp_json_obj["HotWaterAccumulatedLength"].type() == Json::intValue) {
				m_characteristic.m_hot_water_accumulated_length = tmp_json_obj["HotWaterAccumulatedLength"].asInt();
			}
			if (tmp_json_obj["HotWaterCurrentIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_hot_water_current_integer_length = tmp_json_obj["HotWaterCurrentIntegerLength"].asInt();
			}
			if (tmp_json_obj["HotWaterAccumulatedIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_hot_water_accumulated_integer_length = tmp_json_obj["HotWaterAccumulatedIntegerLength"].asInt();
			}

			if (tmp_json_obj["HeatCurrentLength"].type() == Json::intValue) {
				m_characteristic.m_heat_current_length = tmp_json_obj["HeatCurrentLength"].asInt();
			}
			if (tmp_json_obj["HeatAccumulatedLength"].type() == Json::intValue) {
				m_characteristic.m_heat_accumulated_length = tmp_json_obj["HeatAccumulatedLength"].asInt();
			}
			if (tmp_json_obj["HeatCurrentIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_heat_current_integer_length = tmp_json_obj["HeatCurrentIntegerLength"].asInt();
			}
			if (tmp_json_obj["HeatAccumulatedIntegerLength"].type() == Json::intValue) {
				m_characteristic.m_heat_accumulated_integer_length = tmp_json_obj["HeatAccumulatedIntegerLength"].asInt();
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

		if (i == 0) {
			obj_char["Name"] = "WaterCurrentLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_water_current_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "WaterAccumulatedLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_water_accumulated_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "WaterCurrentIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_water_current_integer_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "WaterAccumulatedIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_water_accumulated_integer_length;
			obj_chars.append(obj_char);
		}
		else if (i == 1) {
			obj_char["Name"] = "GasCurrentLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_gas_current_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "GasAccumulatedLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_gas_accumulated_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "GasCurrentIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_gas_current_integer_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "GasAccumulatedIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_gas_accumulated_integer_length;
			obj_chars.append(obj_char);
		}
		else if (i == 2) {
			obj_char["Name"] = "ElectricityCurrentLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_electricity_current_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "ElectricityAccumulatedLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_electricity_accumulated_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "ElectricityCurrentIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_electricity_current_integer_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "ElectricityAccumulatedIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_electricity_accumulated_integer_length;
			obj_chars.append(obj_char);
		}
		else if (i == 3) {
			obj_char["Name"] = "HotWaterCurrentLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_hot_water_current_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "HotWaterAccumulatedLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_hot_water_accumulated_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "HotWaterCurrentIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_hot_water_current_integer_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "HotWaterAccumulatedIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_hot_water_accumulated_integer_length;
			obj_chars.append(obj_char);
		}
		else if (i == 4) {
			obj_char["Name"] = "HeatCurrentLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_heat_current_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "HeatAccumulatedLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_heat_accumulated_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "HeatCurrentIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_heat_current_integer_length;
			obj_chars.append(obj_char);

			obj_char["Name"] = "HeatAccumulatedIntegerLength";
			obj_char["Type"] = "integer";
			obj_char["Value"] = m_characteristic.m_heat_accumulated_integer_length;
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

void RemoteInspectorDevice::setCmdInfo()
{
	m_obj_cmd = generateCmdInfo(m_device_id, m_sub_ids);
}

void RemoteInspectorDevice::setStatus()
{
	Json::Value devices, device, sub_devices, sub_device, statuses, status;

	HomeDevice::setStatus();
	std::string cur_name, acc_name;
	//double cur_value, acc_value;

	statuses.clear();
	sub_devices.clear();
	devices.clear();

	for (unsigned int i = 0; i<m_statuses.size(); i++) {
		statuses.clear();

		if (i == 0) {
			status["Name"] = "WaterCurrent";
		}
		else if (i == 0) {
			status["Name"] = "GasCurrent";
		}
		else if (i == 0) {
			status["Name"] = "ElectricityCurrent";
		}
		else if (i == 0) {
			status["Name"] = "HotWaterCurrent";
		}
		else if (i == 0) {
			status["Name"] = "HeatCurrent";
		}
		status["Type"] = "number";
		status["Value"] = m_statuses[i].m_cur_value;
		statuses.append(status);

		if (i == 0) {
			status["Name"] = "WaterAccumulated";
		}
		else if (i == 0) {
			status["Name"] = "GasAccumulated";
		}
		else if (i == 0) {
			status["Name"] = "ElectricityAccumulated";
		}
		else if (i == 0) {
			status["Name"] = "HotWaterAccumulated";
		}
		else if (i == 0) {
			status["Name"] = "HeatAccumulated";
		}
		status["Type"] = "number";
		status["Value"] = m_statuses[i].m_acc_value;
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

int RemoteInspectorDevice::run()
{
	return HomeDevice::run();
}

void RemoteInspectorDevice::stop()
{
	HomeDevice::stop();
}

int RemoteInspectorDevice::processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj)
{
	std::string cmd;
	Json::Value params;
	Json::Value sub_device, control;
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

	setStatus();

	control["CommandType"] = int2hex_str(int_cmd + 0x80);
	control["Parameters"] = Json::Value(Json::arrayValue);
	control["error"] = "Invalid SubDeviceID";

	sub_device["SubDeviceID"] = int2hex_str(sub_id);
	sub_device["Control"] = Json::arrayValue;
	sub_device["Control"].append(control);

	res_obj.append(sub_device);

	return 0;
}

int RemoteInspectorDevice::processSerialCommand(SerialCommandInfo cmd_info)
{
	if (HomeDevice::processSerialCommand(cmd_info) < 0) {
		tracee("process serial failed.");
		return -1;
	}

	if ((cmd_info.m_command_type & 0x80) != 0) {
		//tracee("the command is from a device: %x", cmd_info.m_command_type);
		return -1;
	}

	return processRemoteInspectorSerialCommand(&cmd_info);
}

int RemoteInspectorDevice::processRemoteInspectorSerialCommand(SerialCommandInfo* cmd_info)
{
	int body_len;
	int res_type, idx;
	uint8_t res_body_bytes[255];

	int int_part, num_part;

	memset(res_body_bytes, 0, 255);
	res_type = 0;
	body_len = 0;

	//request doorlock status info
	if(cmd_info->m_command_type == 0x01){
		res_type = 0x81;
		res_body_bytes[0] = 0;

		idx = 0;
		for (unsigned int i = 0; i < m_statuses.size(); i++) {
			if ((cmd_info->m_sub_id == m_statuses[i].m_sub_id) || ((cmd_info->m_sub_id & 0x0f) == 0x0f)) {
				int_part = (int)m_statuses[i].m_cur_value;
				switch (i) {
				case 2:	// electricity: int(5), num(1)
					num_part = (int)((m_statuses[i].m_cur_value - int_part) * 10);

					res_body_bytes[6 * i + 1] |= ((int)(int_part / 10000) % 10) << 4;
					res_body_bytes[6 * i + 1] |= ((int)(int_part / 1000) % 10);
					res_body_bytes[6 * i + 2] |= ((int)(int_part / 100) % 10) << 4;
					res_body_bytes[6 * i + 2] |= ((int)(int_part / 10) % 10);
					res_body_bytes[6 * i + 3] |= ((int)(int_part / 1) % 10) << 4;
					res_body_bytes[6 * i + 3] |= num_part;
					break;
				default: //others: int(3), num(3)
					num_part = (int)((m_statuses[i].m_cur_value - int_part) * 1000);

					res_body_bytes[6 * i + 1] |= ((int)(int_part / 100) % 10) << 4;
					res_body_bytes[6 * i + 1] |= ((int)(int_part / 10) % 10);
					res_body_bytes[6 * i + 2] |= ((int)(int_part / 1) % 10) << 4;
					res_body_bytes[6 * i + 2] |= (int)((num_part / 100) % 10);
					res_body_bytes[6 * i + 3] |= (int)((num_part / 10) % 10) << 4;
					res_body_bytes[6 * i + 3] |= (int)((num_part / 1) % 10);
					break;
				}

				int_part = (int)m_statuses[i].m_acc_value;

				switch (i) {
				case 4:	// heat: int(4), num(2)
					num_part = (int)((m_statuses[i].m_acc_value - int_part) * 100);

					res_body_bytes[6 * i + 4] |= ((int)(int_part / 1000) % 10) << 4;
					res_body_bytes[6 * i + 4] |= ((int)(int_part / 100) % 10);
					res_body_bytes[6 * i + 5] |= ((int)(int_part / 10) % 10) << 4;
					res_body_bytes[6 * i + 5] |= ((int)(int_part / 1) % 10);
					res_body_bytes[6 * i + 6] |= (int)((num_part / 10) % 10) << 4;
					res_body_bytes[6 * i + 6] |= (int)((num_part / 1) % 10);
					break;
				default: //others: int(5), num(1)
					num_part = (int)((m_statuses[i].m_cur_value - int_part) * 10);

					res_body_bytes[6 * i + 4] |= ((int)(int_part / 10000) % 10) << 4;
					res_body_bytes[6 * i + 4] |= ((int)(int_part / 1000) % 10);
					res_body_bytes[6 * i + 5] |= ((int)(int_part / 100) % 10) << 4;
					res_body_bytes[6 * i + 5] |= ((int)(int_part / 10) % 10);
					res_body_bytes[6 * i + 6] |= ((int)(int_part / 1) % 10) << 4;
					res_body_bytes[6 * i + 6] |= (int)((num_part / 1) % 10);
					break;
				}

				idx++;
			}
		}

		body_len = 6 * idx + 1;

		//no such sub_id
		if (idx == 0) {
			body_len = 0;
		}
	}
	//request doorlock chractoristic info
	else if(cmd_info->m_command_type == 0x0f){
		res_type = 0x8f;
		body_len = 12;

		res_body_bytes[0] = m_characteristic.m_version;
		res_body_bytes[1] = m_characteristic.m_company_code;

		res_body_bytes[2] = m_characteristic.m_water_current_length << 4;
		res_body_bytes[2] = m_characteristic.m_water_accumulated_length;
		res_body_bytes[3] = m_characteristic.m_water_current_integer_length << 4;
		res_body_bytes[3] = m_characteristic.m_water_accumulated_integer_length;

		res_body_bytes[4] = m_characteristic.m_gas_current_length << 4;
		res_body_bytes[4] = m_characteristic.m_gas_accumulated_length;
		res_body_bytes[5] = m_characteristic.m_gas_accumulated_length << 4;
		res_body_bytes[5] = m_characteristic.m_gas_accumulated_integer_length;

		res_body_bytes[6] = m_characteristic.m_electricity_current_length << 4;
		res_body_bytes[6] = m_characteristic.m_electricity_accumulated_length;
		res_body_bytes[7] = m_characteristic.m_electricity_current_integer_length << 4;
		res_body_bytes[7] = m_characteristic.m_electricity_accumulated_integer_length;

		res_body_bytes[8] = m_characteristic.m_hot_water_current_length << 4;
		res_body_bytes[8] = m_characteristic.m_hot_water_accumulated_length;
		res_body_bytes[9] = m_characteristic.m_hot_water_current_integer_length << 4;
		res_body_bytes[9] = m_characteristic.m_hot_water_accumulated_integer_length;

		res_body_bytes[10] = m_characteristic.m_heat_current_length << 4;
		res_body_bytes[10] = m_characteristic.m_heat_accumulated_length;
		res_body_bytes[11] = m_characteristic.m_heat_current_integer_length << 4;
		res_body_bytes[11] = m_characteristic.m_heat_accumulated_integer_length;
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




