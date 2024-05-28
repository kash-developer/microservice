
#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

class HttpRequest;

class HttpResponse : public HttpMessage {

private:
	HttpRequest* m_request;

public:
	HttpResponse();
	~HttpResponse();

	void setRequest(HttpRequest* request);

	int sendResponse();
	int recvResponse(SOCKET sock);
};



#endif
