
#ifndef _HTTP_MESSAGE_H_
#define _HTTP_MESSAGE_H_

#include <win_porting.h>
#include <sys/types.h>
#include <stdint.h>
#include <string>
#include <vector>

//#define MAX_HTTP_URL_LENGTH	1024
//#define MAX_HTTP_HEADER_LENGTH	1024

#define HTTP_RESPONSE_TIMEOUT	10

#ifdef WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#endif

typedef struct _HttpHeaderList {
	std::string m_key;
	std::string m_value;
	//struct _HttpHeaderList* m_next;
} HttpHeaderList;

class HttpMessage {
protected:
	int m_debug_level;
	int m_is_request;
	int m_is_sent_header;

	std::string m_method;

	std::string m_path;
	int m_code;
	std::string m_description;

	std::string m_content_type;

	//HttpHeaderList* m_header;
	std::vector<HttpHeaderList> m_header;

	uint8_t* m_body;
	long m_body_length;

private:
	int recvLine(SOCKET sock, std::string* buffer);
	long recvBody(SOCKET sock, long content_length, uint8_t** buffer);

public:
	static int parseUrl(std::string url, std::string* protocol, std::string* host, unsigned short* port, std::string* path);
	static int getPath_index(int index, std::string path, std::string* value);
	//static int getPath_index(int index, std::string path, std::string value, char** org_pointer);

public:
	HttpMessage();
	~HttpMessage();

	void setDebugLevel(int level);

	void setRequest();
	bool isRequest();

	void setMethod(std::string method);
	void getMethod(std::string* method);

	void setPath(std::string path);
	void getPath(std::string* path);

	int getResponseCode(int* code, std::string* description);
	int setResponseCode(int code, std::string description);

	int addHeader(std::string key, std::string value);
	int removeHeader(std::string key);
	int getHeader(int index, std::string* key, std::string* value);
	int getHeader(std::string key, std::string* value);
	int getHeaderLine();

	long setBody(uint8_t* body, long length, std::string mime_type);
	long getBody(uint8_t** body, std::string* mime_type);
	long getBodyLength();

	int recvMessage(SOCKET sock);

	int serialize(uint8_t** byte_stream);
};


#endif

