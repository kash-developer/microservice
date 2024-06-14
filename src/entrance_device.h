#ifndef _ENTRANCE_DEVICE_H_
#define _ENTRANCE_DEVICE_H_

#include <home_device_lib.h>

struct EntranceStatus {
	int m_sub_id;
	std::string m_last_open;
};

struct EntranceCharacteristic {
	int m_version;
	int m_company_code;
};

class EntranceDevice : public HomeDevice {
private:
	struct EntranceCharacteristic m_characteristic;
	std::vector<struct EntranceStatus> m_statuses;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	void setStatus();
	void setCmdInfo();

public:
	EntranceDevice();
	~EntranceDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();

	void setLastOpen(int sub_id, std::string last_open);
};


#endif

