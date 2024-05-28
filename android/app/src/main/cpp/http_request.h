
#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <http_message.h>
#include <string>

class HttpResponse;

class HttpRequest : public HttpMessage {

private:
	std::string m_url;
	std::string m_host;
	unsigned short m_port;

	SOCKET m_socket;

public:
	HttpRequest();
	~HttpRequest();

	void setSocket(SOCKET sock);
	SOCKET getSocket();

	int setUrl(std::string url);
	void getUrl(std::string* url);

	HttpResponse* sendRequest();
	int recvRequest(SOCKET sock);
};


#endif
