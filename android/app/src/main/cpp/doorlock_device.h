
#ifndef _DOORLOCK_DEVICE_H_
#define _DOORLOCK_DEVICE_H_

#include <home_device_lib.h>

struct DoorLockStatus {
	int m_sub_id;

	bool m_emergency;
	bool m_opened;
};

struct DoorLockCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
	bool m_forced_set_emergency_off;
};

class DoorLockDevice : public HomeDevice {
private:
	struct DoorLockCharacteristic m_characteristic;
	std::vector<struct DoorLockStatus> m_statuses;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processDoorlockSerialCommand(SerialCommandInfo* cmd_info);

	int setClosed(int sub_id, bool open_flag);
	int setEmergency(int sub_id, bool emer_flag);

	void setCmdInfo();
	void setStatus();

public:
	DoorLockDevice();
	~DoorLockDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



