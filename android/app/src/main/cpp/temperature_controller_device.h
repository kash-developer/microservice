
#ifndef _TEMPERATURE_CONTROLLER_DEVICE_H_
#define _TEMPERATURE_CONTROLLER_DEVICE_H_

#include <home_device_lib.h>

struct TemperatureControllerStatus {
	int m_sub_id;

	bool m_heating;
	bool m_outgoing;
	bool m_reservation;
	bool m_hotwater_exclusive;
	double m_setting_temperature;
	double m_cur_temperature;
};

struct TemperatureControllerCharacteristic {
	int m_error;
	int m_version;
	int m_company_code;
	int m_max_temperature;
	int m_min_temperature;
	bool m_decimal_point_flag;
	bool m_outgoing_mode_flag;
	int m_controller_number;
};

class TemperatureControllerDevice : public HomeDevice {
private:
	struct TemperatureControllerCharacteristic m_characteristic;
	std::vector<struct TemperatureControllerStatus> m_statuses;
	bool m_use_old_characteristic;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processTemperatureControllerSerialCommand(SerialCommandInfo* cmd_info);

	void setCmdInfo();
	void setStatus();

	int heatingControl(int sub_id, bool heating);
	int setTemperatureControl(int sub_id, int temperature);
	int setOutGoingModeControl(int sub_id, bool outgoing_mode);

public:
	TemperatureControllerDevice();
	~TemperatureControllerDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



