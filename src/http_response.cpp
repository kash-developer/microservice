
#include "http_message.h"
#include "http_request.h"
#include "http_response.h"
#include "trace.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifdef WIN32
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

HttpResponse::HttpResponse() : HttpMessage()
{
	m_request= NULL;

	addHeader("Connection", "close");
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::setRequest(HttpRequest* request)
{
	m_request = request;
}

int HttpResponse::sendResponse()
{
	int len;
	uint8_t* byte_stream;
	SOCKET sock;

	if(m_request == NULL){
		tracee("there is no corresponding request.");
		return -1;
	}

	sock = m_request->getSocket();
	if(sock <= 0){
		tracee("there is no corresponding socket.");
		return -1;
	}

	//create byte stream
	len = serialize(&byte_stream);
	if(byte_stream == NULL){
		tracee("http_serialize failed.");
		return -1;
	}

	//tracel(m_debug_level, "send response...........\n%s\n", byte_stream);
	//HttpHeaderList* header;
	tracel(m_debug_level, "send response........----...");
	tracel(m_debug_level, "HTTP/1.1 %d %s", m_code, m_description.c_str());
	/*
	header = m_header;
	while(header){
		tracel(m_debug_level, "%s: %s", header->m_key, header->m_value);
		header = header->m_next;
	}
	*/
	for(unsigned int i=0; i<m_header.size(); i++){
		tracel(m_debug_level, "%s: %s", m_header[i].m_key.c_str(), m_header[i].m_value.c_str());
	}
	tracel(m_debug_level, "---------------------------");



	m_is_sent_header = 1;

	//send request
	len = send(sock, (const char*)byte_stream, len, 0);

	if(len < 0){
		tracee("send failed.");
		delete[] byte_stream;
		closesocket(sock);
		return -1;
	} 

	tracel(m_debug_level, "end of sending response: %d\n", len);

	delete[] byte_stream;

	return len;
}

int HttpResponse::recvResponse(SOCKET sock)
{
	return recvMessage(sock);
}



