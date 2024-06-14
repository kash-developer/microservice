
#include "http_request.h"
#include "http_response.h"
#include "trace.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#endif


HttpRequest::HttpRequest() : HttpMessage()
{
	/*
	strcpy(m_url, "");
	strcpy(m_host, "");
	*/
	m_port = 0;
	m_socket = -1;

	setRequest();
}

HttpRequest::~HttpRequest()
{
	if(m_socket != -1){
		closesocket(m_socket);
	}
}

void HttpRequest::setSocket(SOCKET sock)
{
	m_socket = sock;
}

SOCKET HttpRequest::getSocket()
{
	return m_socket;
}

int HttpRequest::setUrl(std::string url)
{
	std::string protocol;
	std::string host;
	std::string path;
	
	struct hostent* he;
	struct in_addr** addr_list;

	m_url = url;

	if(HttpMessage::parseUrl(m_url, &protocol, &host, &m_port, &path) < 0){
		tracee("invalid url: %s", m_url.c_str());
		return -1;
	}

	setPath(path);

	he = gethostbyname(host.c_str());
	if(he == NULL){
		tracee("gethostbyname failed.");
		m_host = host;
	}
	else{
		addr_list = (struct in_addr**)he->h_addr_list;
		if(addr_list[0] == NULL){
			tracee("addr_list[0] is NULL.");
			m_host = host;
		}
		else{
			//strcpy(m_host, inet_ntoa(*addr_list[0]));
			m_host = inet_ntoa(*addr_list[0]);
		}
	}

	//sprintf(host, "%s:%d", m_host, m_port);
	host = m_host + std::string(":") + std::to_string(m_port);
	addHeader("Host", host);

	return 0;
}

void HttpRequest::getUrl(std::string* url)
{
	*url = m_url;
}

HttpResponse* HttpRequest::sendRequest()
{
	int len;
	int ret;
	struct sockaddr_in addr;
	SOCKET sock;

	uint8_t* byte_stream;
	std::string str_content_length;


	HttpResponse* response;

	//create byte stream
	len = serialize(&byte_stream);
	if(byte_stream == NULL){
		tracee("http_serialize failed.");
		return NULL;
	}

	//check if content length is exist
	if(getHeader("Content-Length", &str_content_length) < 0){
		//sprintf(str_content_length, "%d",len);
		str_content_length = std::to_string(len);
		addHeader("Content-Length", str_content_length);
	}

	for(int i=0; i<1; i++){
		//create socket
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if(sock < 0){
			tracee("socket creation failed.");
			delete[] byte_stream;
			return NULL;
		}

		//connect to server
		memset(&addr, 0, sizeof(struct sockaddr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(m_host.c_str());
		addr.sin_port = htons(m_port);

		if((ret = connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr))) < 0){
			tracee("connection failed: %s : %d", m_host.c_str(), m_port);
			perror("connection failed: ");
			//delete[] byte_stream;
			closesocket(sock);
			sleep(1);
			continue;
			//return NULL;
		}
		else{
			break;
		}
	}

	if(ret < 0){
		tracee("create connection is failed three times.");
		return NULL;
	}

	tracel(m_debug_level, "connection success: %d",ret);
	tracel(m_debug_level, "send request: %d...............\n%s\n", len, byte_stream);

	//send request
	if((len = send(sock, (const char*)byte_stream, len, 0)) < 0){
		tracee("send failed.");
		delete[] byte_stream;
		closesocket(sock);
		return NULL;
	} 

	tracel(m_debug_level, "end of sending request: %d\n", len);

	delete[] byte_stream;
	byte_stream = NULL;
	len = -1;

	response = new HttpResponse();

	if(response->recvResponse(sock) < 0){
		tracee("recv response failed.");
		delete response;
		response = NULL;
	}
	else{
		tracel(m_debug_level, "end of receiving response.");
		len = response->serialize(&byte_stream);
		tracel(m_debug_level, "response len: %d", len);
		if(byte_stream == NULL){
			tracee("http_serialize failed.");
		}
		else{
			tracel(m_debug_level, "response message: \n%s", byte_stream);
			delete[] byte_stream;
		}
	}

	closesocket(sock);

	return response;
}

int HttpRequest::recvRequest(SOCKET sock)
{
	return recvMessage(sock);
}



