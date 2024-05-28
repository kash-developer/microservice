
#include "forwarder_lib.h"
#include <tools.h>
#include <trace.h>

#include <json/json.h>

#include <stdio.h>
#include <thread>

#ifdef WIN32
#else
#include <unistd.h>
#endif

int Forwarder::init(Json::Value& conf_json)
{

	Json::Value json_obj;

	std::string serial_port;
	int serial_speed;

	traceSetLevel(99);

	m_run_flag = false;

	if(conf_json["Serial"].type() == Json::objectValue){
		json_obj = conf_json["Serial"];
		if(json_obj["Speed"].type() == Json::intValue){
			serial_speed = json_obj["Speed"].asInt();
		}
		else{
			tracee("there is no serial speed.");
			return -1;
		}

		if(json_obj["Port"].type() == Json::stringValue){
			serial_port = json_obj["Port"].asString();
		}
		else{
			tracee("there is no serial port.");
			return -1;
		}
	}
	else{
		tracee("there is no serial.");
		return -1;
	}

	if(m_ssd.init(serial_port, serial_speed) < 0){
		tracee("serial device (serial) init failed: %s, %d", serial_port.c_str(), serial_speed);
		return -1;
	}
	if(m_msd.init(SERIAL_MULTICAST_PORT) < 0){
		tracee("serial device (multicast) init failed.");
		return -1;
	}
	
    if(m_ssd.run() < 0){
        tracee("run serial device (serial) failed.");
        return -1;
    }
    if(m_msd.run() < 0){
        tracee("run serial device (multicast) failed.");
        return -1;
    }

	return 0;
}

int Forwarder::run()
{
	m_run_flag = true;

	m_m2s_tid = new std::thread(&Forwarder::m2sThread, this);
	m_s2m_tid = new std::thread(&Forwarder::s2mThread, this);

	return 0;
}

void Forwarder::stop()
{
	if(m_run_flag == false){
		return;
	}

	m_run_flag = false;
	m_msd.stop();
	m_ssd.stop();
	
	
}

void Forwarder::m2sThread()
{
	struct SerialQueueInfo q_info;

	while(true){
		if(m_run_flag != true){
			break;
		}

		if (m_msd.get(&q_info) < 0){
			usleep(10000);
			continue;
		}
		trace("send to serial.");
		m_ssd.send(q_info.m_addr, q_info.m_len);
		//delete[] q_info.m_addr;
	}
}

void Forwarder::s2mThread()
{
	struct SerialQueueInfo q_info;

	while(true){
		if (m_run_flag != true){
			break;
		}

		if (m_ssd.get(&q_info) < 0) {
			usleep(10000);
			continue;
		}
		trace("send to multicast.");
		m_msd.send(q_info.m_addr, q_info.m_len);
		//delete[] q_info.m_addr;
	}
}

#ifdef __ANDROID__
int Forwarder::receivedSerial(uint8_t* data, int len)
{
	//return m_ssd.receivedSerial(data, len);
	return m_ssd.send_multicast(data, len);
}
#endif



