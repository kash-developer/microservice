
#include "http_message.h"
#include "trace.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef WIN32
#else
#include <unistd.h>
#include <sys/socket.h>
#endif

//create new http message
HttpMessage::HttpMessage()
{
	m_debug_level = TRACE_DEFAULT_DEBUG_LEVEL;
	m_is_request = 0;
	m_is_sent_header = 0;
/*
	strcpy(m_method, "");
	strcpy(m_path, "");
	strcpy(m_description, "");
	strcpy(m_content_type, "");
*/
	m_code = 0;
	//m_header = NULL;
	m_body = NULL;
	m_body_length = 0;
}

//remove http message
HttpMessage::~HttpMessage()
{
	/*
	HttpHeaderList* cur;
	HttpHeaderList* next;

	//remove http headers
	cur = m_header;
	while(cur){
		next = cur->m_next;
		delete cur;
		cur = next;
	}
	*/

	//remove http body
	if(m_body != NULL){
		delete[] m_body;
	}
	return;
}

void HttpMessage::setDebugLevel(int level)
{
	m_debug_level = level;
}

void HttpMessage::setRequest()
{
	m_is_request = 1;
}

bool HttpMessage::isRequest()
{
	if (m_is_request == 1) {
		return true;
	}
	return false;
}

int HttpMessage::recvLine(SOCKET sock, std::string* buffer)
{
	int ret;
	int i;
	char c;
	char line[1024];

	fd_set fdset;
	fd_set efdset;
	struct timeval timeout;

	//get a line
	memset(line, 0, 1024);
	i = 0;
	while(1){
		FD_ZERO(&fdset);
		FD_ZERO(&efdset);

		FD_SET(sock, &fdset);
		FD_SET(sock, &efdset);

		timeout.tv_sec = HTTP_RESPONSE_TIMEOUT * 10;
		timeout.tv_usec = 0;

		ret = select(((int)sock) + 1, &fdset, NULL, &efdset, &timeout);
		if (ret == 0) {
			tracee("waiting response timeout.");
			return -1;
		}
		else if (ret < 0) {
			tracee("select error");
			return -1;
		}

		if (FD_ISSET(sock, &efdset)) {
			tracee("exception occured when select");
			return -1;
		}

		if (FD_ISSET(sock, &fdset)) {
		}
		else {
			tracee("unknown error in select.");
			return -1;
		}

		ret = recv(sock, &c, 1, 0);

		if(ret < 0){
			tracee("recv failed.");
			return -1;
		}
		if(ret == 0){
			tracee("connection closed...");
			break;
		}

		if(c == '\r'){
			continue;
		}
		if(c == '\n'){
			break;
		}
		else{
			line[i] = c;
		}
		i++;

		if(i >= 1024 - 2){
			tracee("the header line is over 1024: %d, %s", i, line);
			return -1;
		}
	}

	if(i > 0){
		*buffer = line;
	}

	return i;
}

long HttpMessage::recvBody(SOCKET sock, long content_length, uint8_t** buffer)
{
	int len, req_recv;
	long remain;
	long total_received;
	char tmp_buf[1024];
	uint8_t* buffer2;

	int ret;
	fd_set fdset;
	fd_set efdset;
	struct timeval timeout;

	//receive body
	total_received = 0;

	if(content_length < 0){
		remain = -1;
	}
	else{
		remain = content_length;

		*buffer = new uint8_t[content_length + 1];
		memset(*buffer, 0, content_length + 1);
	}

	while(1){
		if(remain == 0){
			break;
		}

		if(remain < 0){
			req_recv = 1024;
		}
		else if(remain < 1024){
			req_recv = remain;
		}
		else{
			req_recv = 1024;
		}

		FD_ZERO(&fdset);
		FD_ZERO(&efdset);

		FD_SET(sock, &fdset);
		FD_SET(sock, &efdset);

		timeout.tv_sec = HTTP_RESPONSE_TIMEOUT;
		timeout.tv_usec = 0;

		ret = select(((int)sock) + 1, &fdset, NULL, &efdset, &timeout);
		if (ret == 0) {
			tracee("waiting response timeout.");
			return -1;
		}
		else if (ret < 0) {
			tracee("select error");
		}

		if (FD_ISSET(sock, &efdset)) {
			tracee("exception occured when select");
			return -1;
		}

		if (FD_ISSET(sock, &fdset)) {
		}
		else {
			tracee("unknown error in select.");
			return -1;
		}

		if((len = recv(sock, tmp_buf, req_recv, 0)) < 0){
			tracee("recv failed.");
			return total_received;
		}
		if(len == 0){
			tracee("connection closed: %d", total_received);
			return total_received;
		}

		//store the received byte-stream
		if(remain > 0){
			if(remain >= len){
				memcpy(*buffer + total_received, tmp_buf, len);
			}
			else{
				memcpy(*buffer + total_received, tmp_buf, remain);
			}
		}
		else{
			buffer2 = *buffer;
			*buffer = new uint8_t[total_received + len + 1];
			memset(*buffer, 0, total_received + len + 1);
			memcpy(*buffer, buffer2, total_received);

			(*buffer)[total_received + len] = '\0';
			memcpy(*buffer + total_received, tmp_buf, len);
			delete[] buffer2;
		}
		
		total_received += len;
		if(remain > 0){
			remain -= len;

			if(remain < 0){
				remain = 0;
			}
		}
	}

	return total_received;
}

int HttpMessage::recvMessage(SOCKET sock)
{
	//char line[MAX_HTTP_HEADER_LENGTH];
	std::string line;
	int content_length;

	int is_first_line;
	char* tmp_line;
	char protocol[1024];
	char key[1024];
	char value[1024];
	char* tmp_char;
	int len;

	content_length = -1;
	is_first_line = 1;

	//receive headers
	while(1){
		//get a line
		len = recvLine(sock, &line);

		//check the end of headers
		if(len < 0){
			tracee("http_recv_lien filaed.");
			return -1;
		}
		if(len == 0){
			break;
		}
		tmp_line = new char[line.length() + 1];
		strcpy(tmp_line, line.c_str());

		//parse header
		if(is_first_line && (m_is_request == 0)){
			is_first_line = 0;

			//get result code
			sscanf(tmp_line, "%s %s %s", protocol, key, value);
			if(strstr(protocol, "HTTP") == NULL){
				if(strstr(protocol, "http") == NULL){
					tracee("invalid response header: %s", tmp_line);
					delete[] tmp_line;
					return -1;
				}
			}

			sscanf(tmp_line, "%s %d", protocol, &(m_code));
			tmp_char = strstr(tmp_line, " ");
			if(tmp_char == NULL){
				tracee("invalid result header: %s", tmp_line);
				delete[] tmp_line;
				return -1;
			}

			tmp_char = strstr(tmp_char+1, " ");
			if(tmp_char == NULL){
				tracee("invalid result header: %s", tmp_line);
				delete[] tmp_line;
				return -1;
			}

			//get result description
			//strcpy(m_description, tmp_char+1);
			m_description = std::string(tmp_char + 1);
		}
		else if(is_first_line && m_is_request){
			is_first_line = 0;

			//get method and path
			sscanf(tmp_line, "%s %s %s", key, value, protocol);
			if(strstr(protocol, "HTTP") == NULL){
				if(strstr(protocol, "http") == NULL){
					tracee("invalid response header: %s", tmp_line);
					delete[] tmp_line;
					return -1;
				}
			}
			
			//sscanf(tmp_line, "%s %s", m_method, m_path);
			sscanf(tmp_line, "%s %s", key, value);
			m_method = key;
			m_path = value;
		}
		else{
			tmp_char = strstr(tmp_line, ":");
			if(tmp_char == NULL){
				tracee("invalid header: %s", tmp_line);
				delete[] tmp_line;
				continue;
			}
			*tmp_char = '\0';
			strcpy(key, tmp_line);
			strcpy(value, tmp_char+2);

			addHeader(key, value);

			//get the content length
			if(strcmp(key, "Content-Length") == 0){
				content_length = atoi(value);
			}
			if(strcmp(key, "Content-Type") == 0){
				//strcpy(m_content_type, value);
				m_content_type = value;
			}
		}
		delete[] tmp_line;
	}

	if(is_first_line){
		tracee("there is no message.");
		return -1;
	}

	//receive body
	if(content_length > 0){
		m_body_length = recvBody(sock, content_length, &m_body);
	}

	return 0;
}

void HttpMessage::setMethod(std::string method)
{
	m_method = method;
	return;
}

void HttpMessage::getMethod(std::string* method)
{
	*method = m_method;
	return;
}

void HttpMessage::setPath(std::string path)
{
	m_path = path;
	return;
}

void HttpMessage::getPath(std::string* path)
{
	*path = m_path;
	return;
}


int HttpMessage::setResponseCode(int code, std::string description)
{
	m_code = code;
	m_description = description;
	return m_code;
}

int HttpMessage::getResponseCode(int* code, std::string* description)
{
	if(code != NULL){
		*code = m_code;
	}

	if(description != NULL){
		*description = m_description;
	}

	return m_code;
}

int HttpMessage::addHeader(std::string key, std::string value)
{
	//char tmp_value[MAX_HTTP_URL_LENGTH];
	std::string tmp_value;
	HttpHeaderList new_header;

	//check if already existing key
	if(getHeader(key, &tmp_value) == 0){
		tracee("alerady added header: %s", key.c_str());
		return -1;
	}

	//create new header
	new_header.m_key = key;
	new_header.m_value = value;

	m_header.push_back(new_header);

	return 0;
}

int HttpMessage::removeHeader(std::string key)
{
	for (unsigned int i = 0; i<m_header.size(); i++) {
		if (m_header[i].m_key.compare(key) == 0) {
			m_header.erase(m_header.begin() + i);
			break;
		}
	}

	return 0;
}

int HttpMessage::getHeaderLine()
{
	int count;

	count = m_header.size();

	return count;
}

int HttpMessage::getHeader(int index, std::string* key, std::string* value)
{
	if(index >= (int)m_header.size()){
		tracee("requested header index: %d / %d", index, m_header.size() - 1);
		return -1;
	}

	*key = m_header[index].m_key;
	*value = m_header[index].m_value;

	return 0;
}

int HttpMessage::getHeader(std::string key, std::string* value)
{
	for(unsigned int i=0; i<m_header.size(); i++){
		if(m_header[i].m_key.compare(key) == 0){
			*value = m_header[i].m_value;
			return 0;
		}
	}

	return -1;
}

long HttpMessage::setBody(uint8_t* body, long length, std::string mime_type)
{
	char buf[64];

	if(m_body != NULL){
		delete[] m_body;
		m_body = NULL;
	}

	m_body_length = length;

	if(length <= 0){
		m_body = NULL;
		return 0;
	}

	removeHeader("Content-Type");
	removeHeader("Content-Length");

	m_content_type = mime_type;
	addHeader("Content-Type", mime_type);

	sprintf(buf, "%ld", length);
	addHeader("Content-Length", buf);

	m_body = new uint8_t[length];
	memcpy(m_body, body, length);

	return length;
}

long HttpMessage::getBody(uint8_t** body, std::string* mime_type)
{
	*body = m_body;
	//strcpy(mime_type, m_content_type);
	*mime_type = m_content_type;

	return m_body_length;
}

long HttpMessage::getBodyLength()
{
	return m_body_length;
}

int HttpMessage::parseUrl(	std::string url,
							std::string* protocol,
							std::string* host,
							unsigned short* port,
							std::string* path)
{
	char* found_string;
	char* start_string;
	char copied_url[1024];
	char host_port[1024];

	//copy the url
	memset(copied_url, 0, 1024);
	strcpy(copied_url, url.c_str());

	//copy protocol
	start_string = copied_url;
	found_string = strstr(start_string, "://");
	//strcpy(protocol, "http");
	*protocol = "http";
	if(found_string != NULL){
		*found_string = '\0';
		//strcpy(protocol, start_string);
		*protocol = start_string;
		*found_string = ':';

		start_string = found_string + strlen("://");
	}
	
	//copy host & port
	found_string = strstr(start_string, "/");
	if(found_string == NULL){
		strcpy(host_port, start_string);
		start_string += strlen(host_port);
	}
	else{
		*found_string = '\0';
		strcpy(host_port, start_string);
		*found_string = '/';
		start_string = found_string;
	}

	//seperate hostname & port
	found_string = strstr(host_port, ":");
	if(found_string == NULL){
		//strcpy(host, host_port);
		*host = host_port;
		*port = 80;
	}
	else{
		*found_string = '\0';
		//strcpy(host, host_port);
		*host = host_port;
		*port = atoi(found_string + 1);
	}
	
	//copy path
	if(strlen(start_string) == 0){
		//strcpy(path, "/");
		*path = "/";
	}
	else{
		//strcpy(path, start_string);
		*path = start_string;
	}


	return 0;
}

int HttpMessage::serialize(uint8_t** byte_stream)
{
	int len;
	std::string tmp_string;
	char first_line[1024];
	char line[1024];

	len = 0;

	*byte_stream = NULL;

	if (getHeader("Content-Length", &tmp_string) < 0) {
		sprintf(line, "%ld", m_body_length);
		addHeader("Content-Length", line);
	}

	if(m_is_sent_header == 0){
		if(m_is_request){
			//calculate the length of the first line
			sprintf(first_line, "%s %s HTTP/1.1\r\n", m_method.c_str(), m_path.c_str());
			len += (int)strlen(first_line);
		}
		else{
			//calculate the length of the first line
			sprintf(first_line, "HTTP/1.1 %d %s\r\n", m_code, m_description.c_str());
			len += (int)strlen(first_line);
		}

		//calculate the length of headers
		for(unsigned int i=0; i<m_header.size(); i++){
			len += (int)strlen(m_header[i].m_key.c_str());
			len += (int)strlen(": ");
			len += (int)strlen(m_header[i].m_value.c_str());
			len += (int)strlen("\r\n");
		}
		len += (int)strlen("\r\n");
	}
	//calculate the length of body
	len += m_body_length;

	*byte_stream = new uint8_t[len+1];
	memset(*byte_stream, 0, len+1);

	len = 0;

	if(m_is_sent_header == 0){
		//copy first line
		strcat((char*)*byte_stream, first_line);
		len += (int)strlen(first_line);

		//copy remained headers
		for(unsigned int i=0; i<m_header.size(); i++){
			sprintf(line, "%s: %s\r\n", m_header[i].m_key.c_str(), m_header[i].m_value.c_str());
			strcat((char*)*byte_stream, line);

			len += (int)strlen(line);
		}
		strcat((char*)*byte_stream, "\r\n");
		len += (int)strlen("\r\n");
	}

	//copy the body
	memcpy(*byte_stream + len, m_body, m_body_length);
	len += m_body_length;

	return len;
}

int HttpMessage::getPath_index(int index, std::string path, std::string* value)
{
	char tmp_path[512];
	char* token;
	int count;

	strcpy(tmp_path, path.c_str());
	count = 0;

	//first search
	token = strtok(tmp_path, "/");
	if(token == NULL){
		return -1;
	}

	//check index and next search
	while(1){
		if(count == index){
			//strcpy(value, token);
			*value = token;
			break;
		}

		token = strtok(NULL, "/");
		if(token == NULL){
			return -1;
		}

		count++;
	}

	return 0;
}






