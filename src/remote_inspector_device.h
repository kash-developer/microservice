
#ifndef _REMOTEINSPECTOR_DEVICE_H_
#define _REMOTEINSPECTOR_DEVICE_H_

#include <home_device_lib.h>

#define SENSOR_NUMBER	5

struct RemoteInspectorStatus {
	int m_sub_id;

	double m_cur_value;
	double m_acc_value;
};

struct RemoteInspectorCharacteristic {
	int m_version;
	int m_company_code;

	int m_water_current_length;
	int m_water_accumulated_length;
	int m_water_current_integer_length;
	int m_water_accumulated_integer_length;

	int m_gas_current_length;
	int m_gas_accumulated_length;
	int m_gas_current_integer_length;
	int m_gas_accumulated_integer_length;

	int m_electricity_current_length;
	int m_electricity_accumulated_length;
	int m_electricity_current_integer_length;
	int m_electricity_accumulated_integer_length;

	int m_hot_water_current_length;
	int m_hot_water_accumulated_length;
	int m_hot_water_current_integer_length;
	int m_hot_water_accumulated_integer_length;

	int m_heat_current_length;
	int m_heat_accumulated_length;
	int m_heat_current_integer_length;
	int m_heat_accumulated_integer_length;
};

class RemoteInspectorDevice : public HomeDevice {
private:
	std::vector<struct RemoteInspectorStatus> m_statuses;
	RemoteInspectorCharacteristic m_characteristic;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processRemoteInspectorSerialCommand(SerialCommandInfo* cmd_info);

	void setCmdInfo();
	void setStatus();

public:
	RemoteInspectorDevice();
	~RemoteInspectorDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



