
#ifndef _CURTAIN_DEVICE_H_
#define _CURTAIN_DEVICE_H_

#include <home_device_lib.h>

#define CURTAIN_MAX_OPEN_VALUE			10

#define CURTAIN_OPERATION_OPENING		8
#define CURTAIN_OPERATION_CLOSING		4
#define CURTAIN_OPERATION_CLOSED		0
#define CURTAIN_OPERATION_OPENED		1
#define CURTAIN_OPERATION_STOPPED		2

struct CurtainStatus {
	int m_sub_id;

	int m_status;
	int m_op;
	int m_cur_open;

	time_t m_last_update;
};

struct CurtainCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
	uint8_t m_characteristic;
};

class CurtainDevice : public HomeDevice {
private:
	struct CurtainCharacteristic m_characteristic;
	std::vector<struct CurtainStatus> m_statuses;
	
	bool m_run_flag;

public:

private:
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);
	int processCurtainSerialCommand(SerialCommandInfo* cmd_info);

	int setAngleValue(int sub_id, int value);
	int setOpenValue(int sub_id, int value);

	int doOperation(int sub_id, int op, int angle_value = 0, int open_value = 0);
	void updateValues();

	void setCmdInfo();
	void setStatus();

public:
	CurtainDevice();
	~CurtainDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();
};


#endif



