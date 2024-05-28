
#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <http_message.h>
#include <mutex>
#include <thread>
#include <vector>

#define HTTP_SERVER_THREAD_NUMBER	3

class HttpRequest;
class HttpResponse;

struct __closing_sockets {
	SOCKET m_socket;
	time_t m_time;
	struct __closing_sockets* m_next;
};

typedef bool (*HttpServerCallbackFunc)(void* instance, HttpRequest* request, HttpResponse* response);

class HttpServer {

private:
	SOCKET m_socket;
	HttpServerCallbackFunc m_callback;
	void* m_instance;

	int m_debug_level;
	bool m_run_flag;
	int m_port;

	std::vector<std::thread*> m_tid;
	std::thread* m_close_socket_tid;

	struct __closing_sockets* m_closing_sockets;
	struct __closing_sockets* m_closing_sockets_old;
	struct __closing_sockets* m_closing_sockets_tmp;
	std::mutex m_closing_socket_mutex;

private:
	SOCKET createServerSocket(unsigned short port);
	int processClient(SOCKET csock);

	void addClosingSocket(struct __closing_sockets*);
	void addClosingSocketOld(struct __closing_sockets*);

public:
	HttpServer();
	~HttpServer();

	int init(unsigned short port);

	//apis for http server
	int run();
	void stop();
	void setCallback(void* instance, HttpServerCallbackFunc func);

	void setDebugLevel(int level);

	//internal func.
	void runServerThread();
	void runClosingSocketThread();
	void processClientThread(SOCKET csock);
};

struct HttpClientInfo{
	HttpServer* m_instance;
	SOCKET m_socket;
};

extern "C" void* run_http_server_thread(void* args);
extern "C" void* process_client_thread(void* args);


#endif
