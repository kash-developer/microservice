
#ifndef _PREVENT_CRIME_EXT_DEVICE_H_
#define _PREVENT_CRIME_EXT_DEVICE_H_

#include <home_device_lib.h>

#define PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER	8

struct PreventCrimeExtStatus {
	int m_sub_id;

	bool m_set[PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER];
	int m_type[PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER];
	int m_status[PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER];
};

struct PreventCrimeExtCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
	bool m_capable_set_type[PREVENT_CRIME_EXT_MAX_SENSOR_NUMBER];
};

class PreventCrimeExtDevice : public HomeDevice {
private:
	struct PreventCrimeExtCharacteristic m_characteristic;
	std::vector<struct PreventCrimeExtStatus> m_statuses;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processPreventCrimeExtSerialCommand(SerialCommandInfo* cmd_info);

	void setCmdInfo();
	void setStatus();

	int setSensorSetControl(int sub_id, uint8_t sets);
	int setSensorTypeControl(int sub_id, uint8_t* types);

public:
	PreventCrimeExtDevice();
	~PreventCrimeExtDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



