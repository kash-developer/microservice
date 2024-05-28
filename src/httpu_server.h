
#ifndef _HTTPU_SERVER_H_
#define _HTTPU_SERVER_H_

#include <httpu_message.h>

#include <mutex>
#include <thread>
#include <vector>
#include <win_porting.h>

struct UdpSocketInfo {
	SOCKET m_socket;
	std::string m_addr;
};

typedef void(*UdpServerCallbackFunc)(void* instance, uint8_t* bytes, int length,
										struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);
typedef void(*HttpuServerCallbackFunc)(void* instance, HttpuMessage* message,
										struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);

class HttpuServer
{
private:
	bool m_run_flag;
	std::thread* m_tid;

	std::string m_addr;
	int m_port;

	//SOCKET m_socket;
	std::vector<struct UdpSocketInfo> m_sockets;
	UdpServerCallbackFunc m_udp_callback;
	HttpuServerCallbackFunc m_httpu_callback;
	void* m_udp_callback_instance;
	void* m_httpu_callback_instance;

private:
#ifdef __ANDROID__
	static std::vector<std::string> m_my_addrs;
#endif

public:
	HttpuServer();
	~HttpuServer();

	static std::vector<std::string> getMyAddrs();
	static std::vector<struct UdpSocketInfo> createUdpSocket();
	static std::vector<struct UdpSocketInfo> createUdpSocket(unsigned short port);
	static std::vector<struct UdpSocketInfo> createUdpSocket(const char* address, unsigned short port);
	static std::vector<struct UdpSocketInfo> createMulticastSocket(const char* mul_addr, const char* local_addr, unsigned short port);

#ifdef __ANDROID__
	static void addMyAddress(std::string addr);
	static void clearMyAddress();
#endif

	int init(const char* address, unsigned short port);
	int run();
	void stop();

	void runServerThread();
	void setCallback(void* instance, UdpServerCallbackFunc func);
	void setCallback(void* instance, HttpuServerCallbackFunc func);
	std::vector<struct UdpSocketInfo> getSocketInfos();

	int send(SOCKET sock, const char* host, unsigned short port, HttpuMessage* message);
	int send(SOCKET sock, const char* host, unsigned short port, const uint8_t* bytes, int len);
};

#endif