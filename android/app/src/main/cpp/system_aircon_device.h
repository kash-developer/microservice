
#ifndef _SYSTEMARCON_DEVICE_H_
#define _SYSTEMARCON_DEVICE_H_

#include <home_device_lib.h>

struct SystemAirconStatus {
	int m_sub_id;

	bool m_power;
	int m_wind_dir;
	int m_wind_vol;
	double m_setting_temperature;
	double m_cur_temperature;
};

struct SystemAirconCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
	uint8_t m_onoff_dev_number;

	bool m_decimal_point;
	uint8_t m_cooling_max_temperature;
	uint8_t m_cooling_min_temperature;
	uint8_t m_indoor_unit_number;
};

class SystemAirconDevice : public HomeDevice {
private:
	struct SystemAirconCharacteristic m_characteristic;
	std::vector<struct SystemAirconStatus> m_statuses;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processSystemAirconSerialCommand(SerialCommandInfo* cmd_info);

	void setCmdInfo();
	void setStatus();

	int setPowerControl(int subdid, bool power);
	int setTemperatureControl(int sub_id, int temperature);
	//int setModeControl(int sub_did, int mode);
	//int setWindDirControl(int sub_did, int wind_dir);
	//int setWindVolControl(int sub_did, int wind_vol);

public:
	SystemAirconDevice();
	~SystemAirconDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



