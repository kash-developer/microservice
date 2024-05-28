#pragma once

#include <string>

#define TEST_RESULT_NOT_TESTED		0
#define TEST_RESULT_SUCCESS			1
#define TEST_RESULT_SKIPPED			2
#define TEST_RESULT_FAIL			-1

#define TEST_EVENT_COMPLETE	-1

#define TEST_TYPE_CONTROLLER	0
#define TEST_TYPE_DEVICE		1
#define TEST_TYPE_STABILITY		2

struct TestItem {
	int m_id;
	std::string m_label;
	std::string m_name;
	int m_test_type;
	bool m_enable;
	int m_result;
	std::string m_description;
};

typedef int(*DeviceTestFunction)(void* instance, struct TestItem*);
