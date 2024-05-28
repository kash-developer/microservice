#ifndef _FORWARDER_LIB_H_
#define _FORWARDER_LIB_H_

#include <json/json.h>
#include <serial_device.h>
#include <thread>

class Forwarder {
private:
	SerialDevice m_msd;
	SerialDevice m_ssd;

	bool m_run_flag;

    std::thread* m_m2s_tid;
    std::thread* m_s2m_tid;


private:
	void m2sThread();
	void s2mThread();

public:
	int init(Json::Value& conf_json);
	int run();
	void stop();

#ifdef __ANDROID__
	int receivedSerial(uint8_t* data, int len);
#endif
};

#endif
