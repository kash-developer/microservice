
#include "httpu_server.h"
#include "trace.h"
#include "win_porting.h"

#ifdef WIN32
#else
#include "unistd.h"
#include "netdb.h"
#endif

#ifdef __ANDROID__
std::vector<std::string> HttpuServer::m_my_addrs;
#endif

HttpuServer::HttpuServer()
{
	m_run_flag = false;
	m_tid = NULL;
	m_udp_callback = NULL;
	m_udp_callback_instance = NULL;
	m_httpu_callback = NULL;
	m_httpu_callback_instance = NULL;
	m_port = -1;
}


HttpuServer::~HttpuServer()
{
}

#ifdef __ANDROID__
void HttpuServer::addMyAddress(std::string addr)
{
	m_my_addrs.push_back(addr);
}
void HttpuServer::clearMyAddress()
{
	m_my_addrs.clear();
}
#endif

std::vector<std::string> HttpuServer::getMyAddrs()
{
#ifdef __ANDROID__
	return m_my_addrs;
#else
	std::vector<std::string> my_addrs;
	struct hostent* he;
	struct in_addr** addr_list;
	char myhostname[255];

	gethostname(myhostname, sizeof(myhostname));
	trace("myaddr: %s", myhostname);
	he = gethostbyname(myhostname);
	if (he == NULL) {
		tracee("gethostbyname failed.");
		return my_addrs;
	}

	addr_list = (struct in_addr**)he->h_addr_list;

/*
	for(int i=0; he->h_aliases[i]; i++){
		printf("alias: %s\n", he->h_aliases[i]);
	}
	for(int i=0; he->h_addr_list[i]; i++){
		printf("addr: %s\n", he->h_aliases[i]);
	}	
*/
	for (int i = 0; i < (int)sizeof(addr_list); i++) {
		//printf("pro...%d\n", i);
		if (addr_list[i] == NULL) {
			trace("addr is null: %d", i);
			break;
		}
		else {
			//printf("ppp: %x\n", addr_list[i]);
			//printf("aaa: %s\n",inet_ntoa(*addr_list[i]));
			trace("addr is %s", inet_ntoa(*addr_list[i]));
			my_addrs.push_back(inet_ntoa(*addr_list[i]));
		}
		//break;
	}

	return my_addrs;
#endif
}

std::vector<struct UdpSocketInfo> HttpuServer::createMulticastSocket(const char* mul_addr, const char* local_addr, unsigned short port)
{
	std::vector<struct UdpSocketInfo> sock_infos;
	struct UdpSocketInfo sock_info;

	struct ip_mreq mreq;
	struct sockaddr_in addr;
	SOCKET sock;
	int on_off;
	std::vector<std::string> my_addrs;
	
	bool error_flag;

	my_addrs = getMyAddrs();
	if (my_addrs.size() == 0) {
		tracee("get my addresses failed.");
		return sock_infos;
	}

	error_flag = false;
	for (unsigned int i = 0; i < my_addrs.size(); i++) {
		//create socket
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock == INVALID_SOCKET) {
			trace("socket creation failed.");
			error_flag = true;
			break;
		}

		//set re-use option
		on_off = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			(char*)&on_off, sizeof(on_off)) != 0) {
			trace("setsockopt for message multi-UDP failed");
			closesocket(sock);
			error_flag = true;
			break;
		}


		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&on_off, sizeof(on_off)) != 0) {
			trace("setsockopt for message multi-UDP failed");
			closesocket(sock);
			error_flag = true;
			break;
		}

		//bind
		addr.sin_family = AF_INET;
#ifdef WIN32
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
#else
		//addr.sin_addr.s_addr = inet_addr(mul_addr);
		//addr.sin_addr.s_addr = inet_addr(my_addrs[i].c_str());
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
		addr.sin_port = htons(port);

		if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
			trace("bind ssdp socket failed.");
			closesocket(sock);
			error_flag = true;
			break;
		}

		trace("createMulticastSock: %d, %s, %s:%d", sock, my_addrs[i].c_str(), mul_addr, port);

		//join multicast group
		mreq.imr_multiaddr.s_addr = inet_addr(mul_addr);
		mreq.imr_interface.s_addr = inet_addr(my_addrs[i].c_str());
		trace("join group %s at %s", mul_addr, my_addrs[i].c_str());
		if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq)) < 0) {
			trace("join group failed: %s", mul_addr);
			perror("join group failed: ");
			closesocket(sock);

			error_flag = true;
			break;
		}

		sock_info.m_socket = sock;
		sock_info.m_addr = my_addrs[i];
		sock_infos.push_back(sock_info);
	}

	if (error_flag == true) {
		for (unsigned int i = 0; i < sock_infos.size(); i++) {
			closesocket(sock_infos[i].m_socket);
		}
		sock_infos.clear();
	}

	return sock_infos;
}

std::vector<struct UdpSocketInfo> HttpuServer::createUdpSocket() {
	return createUdpSocket(NULL, 0);
}

std::vector<struct UdpSocketInfo> HttpuServer::createUdpSocket(unsigned short port) {
	return createUdpSocket(NULL, port);
}

std::vector<struct UdpSocketInfo> HttpuServer::createUdpSocket(const char* address, unsigned short port)
{
	std::vector<struct UdpSocketInfo> sock_infos;
	struct UdpSocketInfo sock_info;

	struct sockaddr_in addr;
	SOCKET sock;
	int on_off;

	std::vector<std::string> my_addrs;

	my_addrs = getMyAddrs();
	if (my_addrs.size() == 0) {
		tracee("get my addresses failed.");
		return sock_infos;
	}

	//create socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		trace("socket creation failed.");
		return sock_infos;
	}

	if (port == 0) {
		sock_info.m_socket = sock;
		sock_info.m_addr = my_addrs[0];
		sock_infos.push_back(sock_info);

		return sock_infos;
	}

	//set re-use option
	on_off = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		(char*)&on_off, sizeof(on_off)) != 0) {
		trace("setsockopt for message multi-UDP failed");
		closesocket(sock);
		return sock_infos;
	}

	if (setsockopt(sock,
		SOL_SOCKET, SO_BROADCAST,
		(char*)&on_off, sizeof(on_off)) != 0) {
		trace("setsockopt for message multi-UDP failed");
		closesocket(sock);
		return sock_infos;
	}

	//bind
	addr.sin_family = AF_INET;
	if (address == NULL) {
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else {
		addr.sin_addr.s_addr = inet_addr(address);
	}
	addr.sin_port = htons(port);

	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		trace("bind ssdp socket failed.");
		closesocket(sock);
		return sock_infos;
	}

	sock_info.m_socket = sock;
	sock_info.m_addr = my_addrs[0];
	sock_infos.push_back(sock_info);

	if (address == NULL) {
		trace("createMulticastSock: %d, INADDR_ANY : %d", sock, port);
	}
	else {
		trace("createMulticastSock: %d, %s : %d", sock, address, port);
	}
	return sock_infos;
}

int HttpuServer::init(const char* address, unsigned short port)
{
	struct sockaddr_in addr;

	if (address == NULL) {
		trace("create unicast socket");
		m_sockets = createUdpSocket(NULL, port);
		m_addr = std::string("-");
	}
	else {
		m_addr = std::string(address);
		addr.sin_addr.s_addr = ntohl(inet_addr(address) & 0x000000f0);
		//trace("addr: %x, %x, %x", inet_addr(address), inet_addr(address) & 0x000000ff, addr.sin_addr.s_addr);
		//if (inet_addr(address) == 0xffffffff) {
		//	m_sockets = createBroadcastSocket(address, NULL, port);
		//}
		if (addr.sin_addr.s_addr == 0xe0000000) {
			trace("create multicast socket");
			m_sockets = createMulticastSocket(address, NULL, port);
		}
		else {
			trace("create unicast socket");
			m_sockets = createUdpSocket(address, port);
		}
	}

	if (m_sockets.size() == 0) {
		tracee("create socket failed.");
		return -1;
	}
	for (unsigned int i = 0; i < m_sockets.size(); i++) {
		if (m_sockets[i].m_socket < 0) {
			tracee("create socket failed.");
			return -1;
		}
	}

	trace("socket(s) created: %d", (int)m_sockets.size());
	m_port = port;

	return 0;
}

int HttpuServer::run()
{
	m_tid = new std::thread(&HttpuServer::runServerThread, this);
	return 0;
}

void HttpuServer::stop()
{
	trace("let's stop httpu server...");
	m_run_flag = false;
	for (unsigned int i = 0; i < m_sockets.size(); i++) {
		closesocket(m_sockets[i].m_socket);
	}
	if (m_tid != NULL) {
		m_tid->join();
		delete m_tid;
		m_tid = NULL;
	}
	trace("httpu server stopped.");

	return;
}

void HttpuServer::runServerThread()
{
	uint8_t buffer[UDP_BUFFER_SIZE];

	HttpuMessage* message;

	fd_set fdset;
	struct timeval timeout;
	SOCKET max_fd;

	struct sockaddr_in src_addr;
	socklen_t addrlen;
	int len;
	int ret;

	struct UdpSocketInfo sock_info;

	trace("HttpuServer started: %s : %d", m_addr.c_str(), m_port);
	m_run_flag = true;
	while (true) {
		if (m_run_flag != true) {
			break;
		}

		FD_ZERO(&fdset);
		max_fd = 0;
		for (unsigned int i = 0; i < m_sockets.size(); i++) {
			FD_SET(m_sockets[i].m_socket, &fdset);
			if (max_fd < m_sockets[i].m_socket) {
				max_fd = m_sockets[i].m_socket;
			}
		}

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		ret = select(((int)max_fd) + 1, &fdset, NULL, NULL, &timeout);
		if (ret == 0) {
			continue;
		}

		sock_info.m_socket = -1;
		for (unsigned int i = 0; i < m_sockets.size(); i++) {
			if (FD_ISSET(m_sockets[i].m_socket, &fdset)) {
				sock_info.m_socket = m_sockets[i].m_socket;
				sock_info.m_addr = m_sockets[i].m_addr;

				addrlen = sizeof(src_addr);
				if ((len = recvfrom(sock_info.m_socket, (char*)buffer, UDP_BUFFER_SIZE, 0, (struct sockaddr*)&src_addr, &addrlen)) < 0) {
					perror("recvfrom failed: ");
					trace("recvfrom failed: %d", errno);
					continue;
				}
				if (len == 0) {
					continue;
				}

				//call callback udp
				if (m_udp_callback != NULL) {
					m_udp_callback(m_udp_callback_instance, buffer, len, sock_info, src_addr);
				}

				//call callback httpu message
				if (m_httpu_callback != NULL) {
					message = new HttpuMessage();

					if (message->parseMessage((char*)buffer, len) < 0) {
						trace("recvMessage failed.");
						delete message;
						continue;
					}
					//trace("after recvfrom: %s", inet_ntoa(src_addr.sin_addr));

					m_httpu_callback(m_httpu_callback_instance, message, sock_info, src_addr);
					delete message;
				}
			}
		}

	}
	trace("HttpuServer stopped.");
}

void HttpuServer::setCallback(void* instance, HttpuServerCallbackFunc func)
{
	m_httpu_callback = func;
	m_httpu_callback_instance = instance;
}

void HttpuServer::setCallback(void* instance, UdpServerCallbackFunc func)
{
	m_udp_callback = func;
	m_udp_callback_instance = instance;
}

int HttpuServer::send(SOCKET sock, const char* host, unsigned short port, HttpuMessage* message)
{
	int ret;
	struct sockaddr_in addr;
	uint8_t* bytes;
	int len;
	SOCKET new_sock;
	std::vector<struct UdpSocketInfo> sock_infos;
	bool is_new_sock_created;

	new_sock = sock;
	is_new_sock_created = false;

	if (new_sock == INVALID_SOCKET) {
		sock_infos = createUdpSocket();
		if (sock_infos.size() == 0) {
			tracee("create new udp socket failed.");
			return -1;
		}
		new_sock = sock_infos[0].m_socket;
		trace("new socket created: %d", new_sock);
		is_new_sock_created = true;
	}

	len = message->serialize(&bytes);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(port);

	//trace("sendto from %d: %s, %d", new_sock, host, port);
	ret = sendto(new_sock, (const char*)bytes, len, 0, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0) {
		perror("sendto error: ");
	}

	if (is_new_sock_created == true) {
		closesocket(new_sock);
	}

	return ret;
}

int HttpuServer::send(SOCKET sock, const char* host, unsigned short port, const uint8_t* bytes, int len)
{
	int ret;
	struct sockaddr_in addr;
	SOCKET new_sock;
	std::vector<struct UdpSocketInfo> sock_infos;

	new_sock = sock;
	if (new_sock < 0) {
		sock_infos = createUdpSocket();
		if (sock_infos.size() == 0) {
			tracee("create new udp socket failed.");
			return -1;
		}
		new_sock = sock_infos[0].m_socket;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(port);

	//trace("sendto: %s, %d", host, port);
	ret = sendto(new_sock, (const char*)bytes, len, 0, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0) {
		perror("sendto error: ");
		return -1;
	}

	return ret;
}

std::vector<struct UdpSocketInfo> HttpuServer::getSocketInfos()
{
	return m_sockets;
}



