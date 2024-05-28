
#ifndef _VALTILATOR_DEVICE_H_
#define _VALTILATOR_DEVICE_H_

#include <home_device_lib.h>

#define VANTILATOR_MODE_BYPASS		0X01
#define VANTILATOR_MODE_SLEEP		0X02
#define VANTILATOR_MODE_HEAT		0X03
#define VANTILATOR_MODE_AUTO		0X04
#define VANTILATOR_MODE_ECO			0X05

#define VANTILATOR_DEFAULT_MAX_AIR_VOLUME	0X04

struct VantilatorStatus {
	int m_sub_id;
	bool m_power;
	int m_air_volume;
};

struct VantilatorCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
	uint8_t m_max_air_volume;
};

class VantilatorDevice : public HomeDevice {
private:
	struct VantilatorCharacteristic m_characteristic;
	std::vector<struct VantilatorStatus> m_statuses;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processVantilatorSerialCommand(SerialCommandInfo* cmd_info);

	int setPower(int sub_id, bool power);
	int setAirVolume(int sub_id, int air_volume);

	void setCmdInfo();
	void setStatus();

public:
	VantilatorDevice();
	~VantilatorDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



