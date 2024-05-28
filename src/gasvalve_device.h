
#ifndef _GASVALVE_DEVICE_H_
#define _GASVALVE_DEVICE_H_

#include <home_device_lib.h>

struct GasValveStatus {
	int m_sub_id;
	bool m_closed;
	bool m_operating;
};

struct GasValveCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
};

class GasValveDevice : public HomeDevice {
private:
	struct GasValveCharacteristic m_characteristic;
	std::vector<struct GasValveStatus> m_statuses;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processGasvalveSerialCommand(SerialCommandInfo* cmd_info);

	int setClosed(int sub_id, bool closed_flag);
	int setExtinguisherBuzzer(int sub_id, bool buzzer_flag);
	int setOperating(int sub_id, bool operating_flag);
	int setGasLeak(int sub_id, bool leak_flag);

	void setCmdInfo();
	void setStatus();

public:
	GasValveDevice();
	~GasValveDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



