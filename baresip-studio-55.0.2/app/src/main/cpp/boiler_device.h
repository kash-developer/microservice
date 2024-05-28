
#ifndef _BOILER_DEVICE_H_
#define _BOILER_DEVICE_H_

#include <home_device_lib.h>

struct BoilerStatus {
	int m_sub_id;

	int m_error;
	bool m_outgoing;
	bool m_heating;
	double m_setting_temperature;
	double m_cur_temperature;
};

struct BoilerCharacteristic {
	int m_version;
	int m_company_code;
	int m_max_temperature;
	int m_min_temperature;
	bool m_decimal_point_flag;
	bool m_outgoing_mode_flag;
};

class BoilerDevice : public HomeDevice {
private:
	struct BoilerCharacteristic m_characteristic;
	std::vector<struct BoilerStatus> m_statuses;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processBoilerSerialCommand(SerialCommandInfo* cmd_info);

	void setCmdInfo();
	void setStatus();

	int heatingControl(int sub_id, bool heating);
	int setTemperatureControl(int sub_id, int temperature);
	int setOutGoingModeControl(int sub_id, bool outgoing_mode);

public:
	BoilerDevice();
	~BoilerDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();

};


#endif



