
#ifndef _HTTPU_MESSAGE_H_
#define _HTTPU_MESSAGE_H_

#include <http_message.h>


#ifdef WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#define UDP_BUFFER_SIZE	65535

class HttpuMessage : public HttpMessage {

private:
	int getLine(const char* buffer,
				int buffer_size,
				int line, 
				char* line_buffer);

	int getBody(	const char* buffer,
					int buffer_size,
					int content_length,
					uint8_t** m_body);
public:
	HttpuMessage();
	~HttpuMessage();

	int recvMessage(SOCKET sock, struct sockaddr_in* addr);
	int parseMessage(const char* bytes, int length);
};


#endif

