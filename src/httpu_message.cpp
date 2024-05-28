

#include "httpu_message.h"
#include "trace.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>

#ifdef WIN32
#ifndef _WINSOCKAPI_
#include <WinSock2.h>
#endif
#include <Ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif


//create new http message
HttpuMessage::HttpuMessage() : HttpMessage()
{
}

//remove http message
HttpuMessage::~HttpuMessage()
{
}

int HttpuMessage::getLine(const char* buffer, int buffer_size, int line, char* line_buffer)
{
	int count;
	const char* index;
	int line_buffer_index;
	int is_line_feed;

	count = 0;
	is_line_feed  = 0;
	index = buffer;
	line_buffer_index = 0;

	while(1){
		//check buffer size
		if(index - buffer >= buffer_size){
			break;
		}
		else if(*index == 0){
			trace("something is wrong, end of string: \n%s\n", buffer);
			line_buffer[line_buffer_index] = 0;
			return -1;
		}

		if(*index == '\r'){
			//do nothing
			;
		}
		else if(*index == '\n'){
			if(count == line){
				line_buffer[line_buffer_index] = 0;
				return (int)(strlen(line_buffer));
			}

			if(is_line_feed == 1){
				break;
			}

			is_line_feed = 1;
			count++;
		}
		else{
			if(count == line){
				line_buffer[line_buffer_index] = *index;
				line_buffer_index++;
			}
			is_line_feed = 0;
		}

		index++;
	}

	return (int)(strlen(line_buffer));
}

int HttpuMessage::getBody(const char* buffer, int size, int content_length, uint8_t** body)
{
	const char* index;
	int index2;
	const char* body_start;
	int is_line_feed;

	body_start = buffer;

	//skip header
	is_line_feed  = 0;
	index = buffer;
	index2 = 0;
	while(1){
		//check buffer size
		if(index - buffer >= size){
			break;
		}

		if(*index == '\r'){
			//do nothing
			;
		}
		else if(*index == '\n'){
			if(is_line_feed == 1){
				body_start = index + 1;
				break;
			}
			is_line_feed = 1;
		}
		else{
			is_line_feed = 0;
		}

		index++;
		index2++;
	}

	//check the body size
	if((index2 + content_length) > size){
	//if(size - (body_start - buffer) < content_length){
		trace("the size of the body is smaller than the content-length. %d / %d",
				size - (body_start - buffer), content_length);
		return -1;
	}

	if(content_length < 0){
		content_length = size - (int)(body_start - buffer);
	}

	if(content_length > 0){
		*body = new uint8_t[content_length];
		memset(*body, 0, content_length);
		memcpy(*body, body_start, content_length);
	}

	return content_length;
}

int HttpuMessage::recvMessage(SOCKET sock, struct sockaddr_in* addr)
{
	//struct sockaddr_in addr;
	socklen_t addrlen;
	int message_len;

	char* udp_buffer;

	udp_buffer = new char[UDP_BUFFER_SIZE];

	//recv from udp
	memset(udp_buffer, 0, UDP_BUFFER_SIZE);
	addrlen = sizeof(struct sockaddr);
	trace("before recvfrom");
	if((message_len = recvfrom(sock,  udp_buffer, UDP_BUFFER_SIZE, 0, (struct sockaddr*)addr, &addrlen)) < 0){
		perror("recvfrom failed: ");
		trace("recvfrom failed: %d", errno);
		delete[] udp_buffer;
		return -1;
	}
	if(message_len == 0){
		delete[] udp_buffer;
		return -1;
	}

/*
	trace("received UDP: %d--------------------------", message_len);
	printf("%s\n", udp_buffer);
	trace("---------------------------------------");
//*/

	if (parseMessage(udp_buffer, message_len) < 0) {
		tracee("parse message failed.");
		delete[] udp_buffer;
		return -1;
	}

	delete[] udp_buffer;

	return 0;
}

int HttpuMessage::parseMessage(const char* bytes, int length)
{
	int line_count;
	char line_buffer[1024];
	int content_length;

	char protocol[256];
	char key[256];
	char value[256];
	char* tmp_char;
	int len;
	int is_first_line;

	//receive headers
	line_count = 0;
	content_length = -1;
	is_first_line = 1;

	while (1) {
		//get a line
		len = getLine((char*)bytes, 1024, line_count, line_buffer);

		//check the end of headers
		if (len < 0) {
			trace("getLine filaed.: \n%s\n", bytes);
			return -1;
		}
		if (len == 0) {
			break;
		}

		if (is_first_line == 1) {
			sscanf(line_buffer, "%s", key);
			if (strstr(key, "HTTP") == NULL) {
				if (strstr(key, "http") == NULL) {
					setRequest();
				}
			}
		}
		//parse header
		if (is_first_line && (m_is_request == 0)) {
			is_first_line = 0;

			//get result code
			sscanf(line_buffer, "%s %s %s", protocol, key, value);
			if (strstr(protocol, "HTTP") == NULL) {
				if (strstr(protocol, "http") == NULL) {
					tracee("invalid response header: %s", line_buffer);
					return -1;
				}
			}

			sscanf(line_buffer, "%s %d", protocol, &(m_code));
			tmp_char = strstr(line_buffer, " ");
			if (tmp_char == NULL) {
				tracee("invalid result header: %s", line_buffer);
				return -1;
			}

			tmp_char = strstr(tmp_char + 1, " ");
			if (tmp_char == NULL) {
				tracee("invalid result header: %s", line_buffer);
				return -1;
			}

			//get result description
			//strcpy(m_description, tmp_char+1);
			m_description = std::string(tmp_char + 1);
		}
		else if (is_first_line && m_is_request) {
			is_first_line = 0;

			//get method and path
			sscanf(line_buffer, "%s %s %s", key, value, protocol);
			if (strstr(protocol, "HTTP") == NULL) {
				if (strstr(protocol, "http") == NULL) {
					tracee("invalid response header: %s", line_buffer);
					return -1;
				}
			}

			//sscanf(tmp_line, "%s %s", m_method, m_path);
			sscanf(line_buffer, "%s %s", key, value);
			m_method = key;
			m_path = value;
		}
		else {
			tmp_char = strstr(line_buffer, ":");
			if (tmp_char == NULL) {
				trace("invalid header: %s", line_buffer);
				continue;
			}
			*tmp_char = '\0';
			strcpy(key, line_buffer);
			if (strlen(tmp_char + 1) > 0) {
				strcpy(value, tmp_char + 2);
			}
			else {
				strcpy(value, "");
			}

			addHeader(key, value);

			//get the content length
			if (strcmp(key, "Content-Length") == 0) {
				content_length = atoi(value);
			}
		}

		line_count++;
	}

	if (is_first_line) {
		trace("there is no message.");
		return -1;
	}

	//receive body
	if (content_length > 0) {
		m_body_length = getBody(bytes, length, content_length, &m_body);
	}

	return 0;
}

