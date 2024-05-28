#pragma once
#include "OpenApiTestBase.h"
class OpenApiTestLight : public OpenApiTestBase
{
private:
	std::vector<struct TestItem*> m_device_test_items;
	Json::Value* m_light_devices;

public:
	OpenApiTestLight();
	~OpenApiTestLight();

	int init(DeviceController* parent, ControllerDevice* controller, HomeDevice* device1, HomeDevice* device2);

	int deviceTestFunctions(struct TestItem* item);
	bool recvBeforeHttpCallback(HomeDevice* device, HttpRequest* request, HttpResponse* response);
	bool recvAfterHttpCallback(HomeDevice* device, HttpRequest* request, HttpResponse* response);
	bool recvBeforeHttpuCallback(HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);
	bool recvAfterHttpuCallback(HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr);

	int test_1(struct TestItem* item);
};
