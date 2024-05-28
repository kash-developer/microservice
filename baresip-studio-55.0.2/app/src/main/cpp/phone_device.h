#ifndef _PHONE_DEVICE_H_
#define _PHONE_DEVICE_H_

#include <home_device_lib.h>

struct PhoneStatus {
	int m_sub_id;
	std::string m_last_call;
};

struct PhoneCharacteristic {
	int m_version;
	int m_company_code;
	std::string m_server_addr;
};

class PhoneDevice : public HomeDevice {
private:
	struct PhoneCharacteristic m_characteristic;
	std::vector<struct PhoneStatus> m_statuses;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	void setStatus();
	void setCmdInfo();

public:
	PhoneDevice();
	~PhoneDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();

	void setLastCall(int sub_id, std::string last_call);
};


#endif

