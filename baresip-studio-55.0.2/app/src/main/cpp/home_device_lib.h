
#ifndef HOME_DEVICE_LIB_H_
#define HOME_DEVICE_LIB_H_

#include <http_server.h>
#include <httpu_server.h>

#include <serial_device.h>
#include <string>
#include <json/json.h>

#include <thread>
#include <vector>
#include <queue>
#include <time.h>
#include <map>
#include <mutex>

#define HOME_DEVICE_VERSION	1
#define DATA_HOME_PATH	"."

#define SMARTHOME_BASE_PATH	std::string("/smarthome/v1")

#define MIN_REQUEST_DELAY		10	//mili second
#define MIN_RESPONSE_DELAY		10	//mili second

#define HTTPU_DISCOVERY_ADDR	"224.0.1.1"
#define HTTPU_DISCOVERY_PORT	65123
#define HTTPU_ADVERTISE_INTERVAL	60
#define HTTPU_DISCOVERY_NUMBER	3
//#define HTTPU_ADVERTISE_INTERVAL	1

#define DEVICE_TYPE_SERIAL		1
#define DEVICE_TYPE_ETHERNET	2
#define DEVICE_TYPE_BOTH		(DEVICE_TYPE_SERIAL | DEVICE_TYPE_ETHERNET)


#define ERROR_CODE_SUCCESS		0
#define ERROR_CODE_NO_SUCH_DEVICE			1
#define ERROR_CODE_INTERNAL_ERROR			2
#define ERROR_CODE_INVALID_MESSAGE			11
#define ERROR_CODE_INVALID_COMMAND_TYPE		12
#define ERROR_CODE_INVALID_PARAMETER		13
#define ERROR_CODE_SEND_FAILED	201
#define ERROR_CODE_NO_RESPONSE	202
#define ERROR_CODE_UNKNOWN	255

class HomeDevice;

struct DiscoveryRequestInfo {
	std::string m_type;
	std::string m_value;
	struct UdpSocketInfo m_from_interface;
	struct sockaddr_in m_from_addr;
};

struct DiscoveryResponseHistory {
	struct DiscoveryRequestInfo m_req_info;
	time_t m_last_sent;
};

struct DeviceInformation {
	int m_device_id;
	std::vector<int> m_sub_ids;
	int m_sub_gid;
	std::string m_device_name;
	std::string m_base_url;
	int m_type;
	bool m_use_old_characteristic;
};

struct OpenApiCommandInfo {
	int m_device_id;
	int m_sub_id;
	Json::Value m_obj;
};

typedef struct _SerialCommandInfo{
	uint8_t m_header;
	uint8_t m_device_id;
	uint8_t m_sub_id;
	uint8_t m_command_type;
	uint8_t m_data_length;
	std::vector<uint8_t> m_data;
	uint8_t m_xor_sum;
	uint8_t m_add_sum;

	std::vector<uint8_t> m_frame;

	struct timeval m_arrived;
	long m_response_delay;
	long m_serial_byte_delay;
} SerialCommandInfo;

typedef int(*ProcessSerialCommandCallback)(void*, HomeDevice*, SerialCommandInfo*);
typedef int(*SendSerialCommandCallback)(void*, HomeDevice*, SerialCommandInfo*, int, uint8_t*, int);

typedef bool(*recvHttpCallback)(void*, HomeDevice*, HttpRequest*, HttpResponse*);
typedef bool(*recvHttpuCallback)(void*, HomeDevice*, HttpuMessage*, struct UdpSocketInfo, struct sockaddr_in);

class HomeDevice {
protected:
	SerialDevice* m_serial_device;
	HttpServer* m_http_server;
	HttpuServer* m_httpu_server;

	int m_http_port;

	bool m_run_flag;

	std::thread* m_get_cmd_tid;
	std::thread* m_send_adv_tid;
	std::thread* m_process_discovery_tid;

	Json::Value m_obj_configuration;
	Json::Value m_obj_characteristic;
	Json::Value m_obj_status;
	Json::Value m_obj_cmd;

	int m_debug_level;
	int m_device_id;
	std::vector<int> m_sub_ids;
	int m_sub_gid;
	std::string m_device_name;

	std::string m_ui_home_path;
	std::string m_data_home_path;

	std::vector<struct DeviceInformation> m_devices;
	std::queue<struct DiscoveryRequestInfo> m_discovery_req_q;

	void* m_serial_callback_instance;
	ProcessSerialCommandCallback m_before_process_cmd_callback;
	SendSerialCommandCallback m_before_send_cmd_callback;
	ProcessSerialCommandCallback m_after_process_cmd_callback;

	void* m_http_callback_instance;
	recvHttpCallback m_recv_before_process_http_callback;
	recvHttpCallback m_recv_after_process_http_callback;

	void* m_httpu_callback_instance;
	recvHttpuCallback m_recv_before_process_httpu_callback;
	recvHttpuCallback m_recv_after_process_httpu_callback;

	static std::map<int, std::string> m_error_description;
	static std::mutex m_mutex;
	static bool m_class_init;

public:
	int getDeviceId();

protected:
	int serializeOpenApiResponse_device_group(Json::Value& obj_ret, int sub_id, Json::Value obj);
	int serializeOpenApiResponse_deviceList(Json::Value& obj_ret, int device_id, int sub_id, Json::Value obj);
	std::string serializeOpenApiResponse(std::vector<struct OpenApiCommandInfo> responses, Json::Value obj_req);

	std::vector<struct OpenApiCommandInfo> parseOpenApiCommand_device_group(int device_id, int sub_id, Json::Value obj);
	std::vector<struct OpenApiCommandInfo> parseOpenApiCommand_deviceList(int device_id, int sub_id, Json::Value obj);
	std::vector<struct OpenApiCommandInfo> parseOpenApiCommand(int device_id, int sub_id, Json::Value obj);

	virtual int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	virtual int processSerialCommand(SerialCommandInfo cmd_info);
	virtual void setStatus();
	virtual void setCmdInfo();

	int checkCommandToMe(int device_id, int sub_id);

	Json::Value createErrorResponse(int code, std::string description = "");
	std::string getErrorDescription(int code);

	void runSerialCommandReceiverThread();
	void runSendAdvertisementThread();
	void runProcessDiscoveryThread();

	int sendAdvertisement();

public:
	HomeDevice();
	virtual ~HomeDevice();

	Json::Value generateCmdInfo(int device_id, std::vector<int> sub_ids);
	int extractParametersByCommandInfo(int device_id, Json::Value cmd, Json::Value& sorted_params);

	static int parseSerialCommand(SerialQueueInfo q_info, SerialCommandInfo* info);
	static bool checkChecksum(SerialCommandInfo* info, uint8_t* r_xor_sum = NULL, uint8_t* r_add_sum = NULL);
	static std::string getDeviceNameString(int device_id);

	virtual int init(std::string json_str, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	virtual int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	virtual int finalizeInit();

	virtual Json::Value getStatus(std::string device_id, std::string sub_id);
	virtual Json::Value getCharacteristic(std::string device_id, std::string sub_id);
	
	virtual int run();
	virtual void stop();

	virtual bool recvHttpRequest(HttpRequest* request, HttpResponse* response);
	virtual void recvHttpuMessage(HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);

	bool recvHttp(HttpRequest* request, HttpResponse* response);
	void recvHttpu(HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);

	void setCallback(	void* instance,
						ProcessSerialCommandCallback callback1, 
						SendSerialCommandCallback callback2, 
						ProcessSerialCommandCallback callback3);
	void setCallback(void* instance, recvHttpCallback before_callback, recvHttpCallback after_callback);
	void setCallback(void* instance, recvHttpuCallback before_callback, recvHttpuCallback after_callback);

	int sendSerialCommand(uint8_t* bytes, int len);
	int sendSerialResponse(SerialCommandInfo* req_info, int res_type, uint8_t* body_bytes, int len);

	void setDataHomePath(std::string path);

};


#endif



