
#ifndef _CONTROLLER_DEVICE_H_
#define _CONTROLLER_DEVICE_H_

#include <home_device_lib.h>
#include <httpu_server.h>

#include <mutex>

#define COMMAND_RESPONSE_TIMEOUT	3
#define DISCOVERY_RESPONSE_HTTPU_PORT	65222

struct ControllerStatus {
	int m_sub_id;
};

struct ControllerCharacteristic {
	uint8_t m_version;
	uint8_t m_company_code;
};


class ControllerDevice : public HomeDevice {
private:
	struct ControllerCharacteristic m_characteristic;
	std::vector<struct ControllerStatus> m_statuses;

	std::vector<SerialCommandInfo> m_serial_cmds;
	std::mutex m_serial_cmd_mutex;

	bool m_store_message;

	uint8_t m_last_sent_bytes[255];
	int m_last_sent_bytes_len;
	HttpuServer* m_discovery_httpu_server;

public:

private:
	Json::Value getDeviceList();
	int processHttpCommand(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);
	int processSerialCommand(SerialCommandInfo cmd_info);

	void setStatus();
	void setCmdInfo();

	void sendDiscovery();
	void processAdvertisement(Json::Value jobj_dev);

	Json::Value makeErrorValues(int error_code);

	Json::Value getDeviceCharacteristic_systemAircon(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_microwaveOven(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_dishWasher(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_drumWasher(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_light(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_gasValve(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_curtain(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_remoteInspector(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_doorLock(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_vantilator(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_breaker(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_boiler(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_preventCrimeExt(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_temperatureController(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_zigbee(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_powerMeter(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceCharacteristic_powerGate(int device_id, int sub_id, DeviceInformation device_info);

	Json::Value getDeviceStatus_systemAircon(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_microwaveOven(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_dishWasher(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_drumWasher(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_light(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_gasValve(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_curtain(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_remoteInspector(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_doorLock(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_vantilator(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_breaker(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_boiler(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_preventCrimeExt(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_temperatureController(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_zigbee(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_powerMeter(int device_id, int sub_id, DeviceInformation device_info);
	Json::Value getDeviceStatus_powerGate(int device_id, int sub_id, DeviceInformation device_info);

	Json::Value makeControlResponse(int device_id, int sub_id, DeviceInformation device_info, std::string command_type);

	int controlDevice_systemAircon(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_microwaveOven(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_dishWasher(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_drumWasher(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_light(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_gasValve(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_curtain(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_remoteInspector(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_doorLock(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_vantilator(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_breaker(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_boiler(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_preventCrimeExt(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_temperatureController(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_zigbee(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_powerMeter(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);
	int controlDevice_powerGate(int device_id, int sub_id, DeviceInformation device_info, std::string cmd, Json::Value params, Json::Value& json_obj_res);

public:
	ControllerDevice();
	~ControllerDevice();

	int init(Json::Value& conf_json, std::vector<int> sub_ids = std::vector<int>(), int http_port = -1, bool use_serial = true);
	int run();
	void stop();

	void recvHttpuMessage(HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);
	void processDiscoveryResponse(HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);

	Json::Value getDeviceCharacteristic(int device_id, int sub_id);
	Json::Value getDeviceStatus(int device_id, int sub_id);
	int controlDevice(int device_id, int sub_id, Json::Value cmd_obj, Json::Value& res_obj);

	bool recvHttpRequest(HttpRequest* request, HttpResponse* response);

	int sendGetDeviceCharacteristic(int device_id, int sub_id);
	int sendGetDeviceStatus(int device_id, int sub_id);
	int getCommand(int device_id, int sub_id, int cmd_type, SerialCommandInfo* cmd_info);
	int sendSerialCommand(uint8_t* bytes, int len);

	void printDeviceList();
};


#endif



