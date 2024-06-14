
#ifndef _SERIAL_DEVICE_H_
#define _SERIAL_DEVICE_H_

#include <httpu_server.h>
#include <win_porting.h>

#include <thread>
#include <queue>
#include <mutex>
#ifdef WIN32
#include <Ws2tcpip.h>
#include <Windows.h>
#endif

//for TEST UDP
//#define SERIAL_MULTICAST_ADDR	"225.255.255.255"
#define SERIAL_MULTICAST_ADDR	"224.0.1.2"
//#define SERIAL_MULTICAST_ADDR	"127.0.0.1"
#define SERIAL_MULTICAST_PORT	17777

#define FRAME_HEADER_LENGTH		5
#define FRAME_TAIL_LENGTH		2
#define FRAME_DATALENGTH_IDX	4

#define FRAME_START_BYTE		0XF7

#define CONTROLLER_DEVICE_ID			0xE8
#define FORWARDER_DEVICE_ID				0XE9
#define PHONE_DEVICE_ID					0XEA
#define ENTRANCE_DEVICE_ID				0xEB

#define SYSTEMAIRCON_DEVICE_ID			0X02
#define MICROWAVEOVEN_DEVICE_ID			0X04
#define DISHWASHER_DEVICE_ID			0X09
#define DRUMWASHER_DEVICE_ID			0X0A
#define LIGHT_DEVICE_ID					0X0E
#define GASVALVE_DEVICE_ID				0X12
#define CURTAIN_DEVICE_ID				0X13
#define REMOTEINSPECTOR_DEVICE_ID		0X30
#define DOORLOCK_DEVICE_ID				0X31
#define VANTILATOR_DEVICE_ID			0X32
#define BREAKER_DEVICE_ID				0X33
#define PREVENTCRIMEEXT_DEVICE_ID		0X34
#define BOILER_DEVICE_ID				0X35
#define TEMPERATURECONTROLLER_DEVICE_ID	0X36
#define ZIGBEE_DEVICE_ID				0X37
#define POWERMETER_DEVICE_ID			0X38
#define POWERGATE_DEVICE_ID				0X39

struct SerialQueueInfo {
	uint8_t m_addr[4096];
	int m_len;
	struct timeval m_arrived;
	long m_serial_byte_delay;
};

class SerialDevice {
private:
	int m_use_serial;
	int m_multicast_port;
	int m_sending_port;
	HANDLE m_sfd;

	std::string m_com_port;
	int m_com_speed;

	//int m_msfd;
	//int m_mrfd;
	HttpuServer* m_httpu_server;
	std::vector<struct UdpSocketInfo> m_sockets;

	std::thread* m_serial_read_thread;
	std::queue<struct SerialQueueInfo*> m_queue;
	std::mutex m_queue_mutex;

	bool m_run_flag;
	bool m_byte_delay_test_flag;

public:

private:
	int setInterfaceAttrib(HANDLE fd, int speed);
	void runReadThread_serial();

public:
	SerialDevice();
	~SerialDevice();

	void runReadThread_multicast(uint8_t* bytes, int length, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);


	static std::string printBytes(const uint8_t* bytes, int len);
	static void getChecksum(const uint8_t* bytes, int len, uint8_t* xor_checksum, uint8_t* add_checksum);

	int init();
	int init(int port);
	int init(const std::string port, int speed);
	int run();
	void stop();
	void setByteDelayTest(bool flag);
	
	int get(SerialQueueInfo* q_info);
	int send(const uint8_t* bytes, int len);
	int send_multicast(const uint8_t* bytes, int len);
	int send_serial(const uint8_t* bytes, int len);
};

#endif
