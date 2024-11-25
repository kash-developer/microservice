
#ifndef _LIGHT_DEVICE_H_
#define _LIGHT_DEVICE_H_

#include <home_device_lib.h>

struct LightStatus {
	int m_sub_id;
	bool m_power;
};

struct LightCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
	uint8_t m_onoff_dev_number;
};

class LightDevice : public HomeDevice {
private:
	struct LightCharacteristic m_characteristic;
	std::vector<struct LightStatus> m_statuses;
	bool m_use_old_characteristic;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processLightSerialCommand(SerialCommandInfo* cmd_info);

	int setPower(int sub_id, bool power, bool is_package = false);
	int setDimmingLevel(int sub_id, int level);

	void setStatus();
	void setCmdInfo();

public:
	LightDevice();
	~LightDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();

};


#endif



