#include "stdafx.h"
#include "OpenApiTestLight.h"

#include "DeviceController.h"

#include <tools.h>
#include <trace.h>
#include <win_porting.h>

#include <http_request.h>
#include <http_response.h>


OpenApiTestLight::OpenApiTestLight()
{
}


OpenApiTestLight::~OpenApiTestLight()
{
}


int OpenApiTestLight::init(DeviceController* parent, ControllerDevice* controller, HomeDevice* device1, HomeDevice* device2)
{
	struct TestItem test_item;

	trace("light init.");
	if (OpenApiTestBase::init(parent, controller, device1, device2) < 0) {
		tracee("test base init failed.");
		return -1;
	}

	m_light_devices = NULL;

	test_item.m_label = "1";
	test_item.m_name = std::string("1. 라이트 테스트");
	test_item.m_test_type = TEST_TYPE_DEVICE;
	test_item.m_enable = true;
	test_item.m_result = TEST_RESULT_NOT_TESTED;
	test_item.m_description = std::string("not tested");
	addTestItem(test_item);

	return 0;
}

int OpenApiTestLight::deviceTestFunctions(struct TestItem* item)
{
	int ret;
	std::string log;

	ret = TEST_RESULT_NOT_TESTED;

	if (item->m_label.compare("1") == 0) {
		log = std::string("Test Name: ") + item->m_name;
		OpenApiTestBase::writeLog(log);

		ret = test_1(item);
	}
	else {
		ret = OpenApiTestBase::deviceTestFunctions(item);
	}

	return ret;
}


bool OpenApiTestLight::recvBeforeHttpCallback(HomeDevice* device, HttpRequest* request, HttpResponse* response)
{
	return true;
}

bool OpenApiTestLight::recvAfterHttpCallback(HomeDevice* device, HttpRequest* request, HttpResponse* response)
{
	return true;
}

bool OpenApiTestLight::recvBeforeHttpuCallback(HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	return true;
}

bool OpenApiTestLight::recvAfterHttpuCallback(HomeDevice* device, HttpuMessage* message, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	return true;
}

int OpenApiTestLight::test_1(struct TestItem* item)
{
	item->m_description = "success";
	item->m_result = TEST_RESULT_SUCCESS;
	writeLog("test success.");

	return item->m_result;
}

