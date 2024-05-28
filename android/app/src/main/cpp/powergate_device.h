
#ifndef _POWERGATE_DEVICE_H_
#define _POWERGATE_DEVICE_H_

#include <home_device_lib.h>

struct PowerGateStatus {
	int m_sub_id;

	bool m_power;
	double m_power_measurement;
};

struct PowerGateCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
	uint8_t m_channel_number;
};

class PowerGateDevice : public HomeDevice {
private:
	struct PowerGateCharacteristic m_characteristic;
	std::vector<struct PowerGateStatus> m_statuses;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processPowerGateSerialCommand(SerialCommandInfo* cmd_info);

	void setCmdInfo();
	void setStatus();

	int individualControl(int sub_id, bool value);
	int allControl(int sub_id, bool value);

public:
	PowerGateDevice();
	~PowerGateDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



