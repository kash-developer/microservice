
#include "http_server.h"
#include "http_request.h"
#include "http_response.h"
#include "trace.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <thread>

#ifdef WIN32
#include <Windows.h>
//#define sleep Sleep
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#endif


HttpServer::HttpServer()
{
}

HttpServer::~HttpServer()
{
	if(m_socket > 0){
		closesocket(m_socket);
	}
	stop();
}

int HttpServer::init(unsigned short port)
{
	m_socket = 0;
	m_callback = NULL;
	m_run_flag = false;
	m_debug_level = TRACE_DEFAULT_DEBUG_LEVEL;
	m_port = -1;

	m_close_socket_tid = NULL;
	m_closing_sockets = NULL;
	m_closing_sockets_old = NULL;

	m_socket = createServerSocket(port);
	if (m_socket < 0) {
		tracee("createServerSocket failed.");
		return -1;
	}

	m_port = port;

	return 0;
}

void HttpServer::setCallback(void* instance, HttpServerCallbackFunc func)
{
	m_instance = instance;
	m_callback = func;
}

void HttpServer::setDebugLevel(int level)
{
	m_debug_level = level;
}

SOCKET HttpServer::createServerSocket(unsigned short port)
{
	struct sockaddr_in addr;
	int opt;

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET) {
		tracee("socket creation failed.");
		return -1;
	}

	//connect to server
	memset(&addr, 0, sizeof(struct sockaddr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	opt = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));


	if(bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0){
		tracee("bind server sock failed.");
		return -1;
	}

	listen(m_socket, 5);

	return m_socket;
}

int HttpServer::run()
{
	std::thread* tid;

	for(int i=0; i<HTTP_SERVER_THREAD_NUMBER; i++){
		tid = new std::thread(&HttpServer::runServerThread, this);
		if (tid == NULL) {
			tracee("create http thread failed: %d", i);
			break;
		}
		m_tid.push_back(tid);
	}

	m_close_socket_tid = new std::thread(&HttpServer::runClosingSocketThread, this);
	
	return 0;
}

void HttpServer::runServerThread()
{
	SOCKET csock;
	struct sockaddr_in addr;
	socklen_t addrlen;
	fd_set fdset;
	SOCKET max_fd;
	struct timeval timeout;
	int ret;

	tracel(m_debug_level, "HttpServer started: %d", m_port);

	m_run_flag = true;

	while (1) {
		if (m_run_flag != true) {
			break;
		}

		FD_ZERO(&fdset);
		max_fd = m_socket;
		FD_SET(m_socket, &fdset);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		ret = select(((int)max_fd) + 1, &fdset, NULL, NULL, &timeout);
		if (ret == 0) {
			continue;
		}

		if (FD_ISSET(m_socket, &fdset)) {
			//accept http client
			addrlen = sizeof(addr);
			csock = accept(m_socket, (struct sockaddr *) &addr, &addrlen);
			if (csock < 0) {
				perror("accept failed: ");
				tracee("accept failed.");
				continue;
			}

			//process the client
			if (processClient(csock) < 0) {
				tracee("processClient failed.");
				closesocket(csock);
				continue;
			}
		}
	}

	tracel(m_debug_level, "HttpServer Stopped.");

	m_run_flag = false;

	closesocket(m_socket);
	m_socket = 0;

	return;
}

void HttpServer::stop()
{
	std::thread* tid;

	trace("let's stop http server.");
	m_run_flag = false;
	closesocket(m_socket);
	if (m_close_socket_tid != NULL) {
		m_close_socket_tid->join();
		delete m_close_socket_tid;
		m_close_socket_tid = NULL;
	}
	for(unsigned int i=0; i<m_tid.size(); i++){
		tid = m_tid[i];
		tid->join();
		delete tid;
	}
	m_tid.clear();

	trace("http server stopped.");

	return;
}

int HttpServer::processClient(SOCKET csock)
{
	processClientThread(csock);

	return 0;
}

void HttpServer::processClientThread(SOCKET csock)
{
	HttpRequest* request;
	HttpResponse* response;
	bool need_send_response;
	uint8_t* byte_stream;
	struct __closing_sockets* s_info;

	//get request
	request = new HttpRequest();
	if(request->recvRequest(csock) < 0){
		tracee("recv http request failed.");
		delete request;
		closesocket(csock);
		return;
	}
	request->setSocket(csock);

	if(request->serialize(&byte_stream) < 0){
		tracee("serialize received request failed.");
	}
	else{
		tracel(m_debug_level, "received request----------------\n%s", byte_stream);
		delete[] byte_stream;
	}

	response = new HttpResponse();
	response->setDebugLevel(m_debug_level);
	response->setRequest(request);

	//call callbacck
	if(m_callback == NULL){
		response->setResponseCode(404, "Not Found");
		need_send_response = true;
	}
	else{
		need_send_response = m_callback(m_instance, request, response);
	}

	//send response if need
	if(need_send_response){
		response->sendResponse();
	}

	s_info = new struct __closing_sockets;
	s_info->m_socket = csock;
	s_info->m_time = time(NULL);
	s_info->m_next = NULL;

	addClosingSocket(s_info);
	request->setSocket(-1);
	delete request;
	delete response;

	//closesocket(csock);

	return;
}

void HttpServer::addClosingSocket(struct __closing_sockets* s_info)
{
	struct __closing_sockets* tmp_info;

	s_info->m_next = NULL;

	m_closing_socket_mutex.lock();

	s_info->m_next = NULL;

	if(m_closing_sockets == NULL){
		m_closing_sockets = s_info;
	}
	else{
		tmp_info = m_closing_sockets;
		while(tmp_info->m_next){
			tmp_info = tmp_info->m_next;
		}
		tmp_info->m_next = s_info;
	}

	m_closing_socket_mutex.unlock();

	return;
}

void HttpServer::addClosingSocketOld(struct __closing_sockets* s_info)
{
	struct __closing_sockets* tmp_info;

	s_info->m_next = NULL;

	if(m_closing_sockets_old == NULL){
		m_closing_sockets_old = s_info;
	}
	else{
		tmp_info = m_closing_sockets_old;
		while(tmp_info->m_next){
			tmp_info = tmp_info->m_next;
		}
		tmp_info->m_next = s_info;
	}

	return;
}

void HttpServer::runClosingSocketThread()
{
	struct __closing_sockets* tmp_info;
	struct __closing_sockets* tmp_info2;
	time_t cur_time;

	tracel(m_debug_level, "ClosingSocketThread started.");

	while(1){
		if(m_run_flag != true){
			break;
		}

		m_closing_socket_mutex.lock();

		m_closing_sockets_tmp = m_closing_sockets;
		m_closing_sockets = NULL;
		
		m_closing_socket_mutex.unlock();

		cur_time = time(NULL);

		tmp_info = m_closing_sockets_tmp;
		while(tmp_info){
			tmp_info2 = tmp_info;
			tmp_info = tmp_info->m_next;

			if(tmp_info2->m_time + 3 < cur_time){
				addClosingSocketOld(tmp_info2);
			}
			else{
				addClosingSocket(tmp_info2);
			}
		};
		m_closing_sockets_tmp = NULL;

		tmp_info = m_closing_sockets_old;
		while(tmp_info){
			tmp_info2 = tmp_info;
			tmp_info = tmp_info->m_next;

			closesocket(tmp_info2->m_socket);
			delete tmp_info2;
		};
		m_closing_sockets_old = NULL;

		sleep(1);
	}

	return;
}




