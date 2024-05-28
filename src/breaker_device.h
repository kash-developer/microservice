
#ifndef _BREAKER_DEVICE_H_
#define _BREAKER_DEVICE_H_

#include <home_device_lib.h>

struct BreakerStatus {
	int m_sub_id;
	bool m_light_relay_closed;
};

struct BreakerCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
};

class BreakerDevice : public HomeDevice {
private:
	struct BreakerCharacteristic m_characteristic;
	std::vector<struct BreakerStatus> m_statuses;

	std::vector<int> m_elevators;

	bool m_run_flag;
	time_t m_ele_control_update;


public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processBreakerSerialCommand(SerialCommandInfo* cmd_info);

	int individualRelayControl(int did, int light_relay_closed);
	int allRelayControl(int light_relay_closed);

	void setCmdInfo();
	void setStatus();

public:
	BreakerDevice();
	~BreakerDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



