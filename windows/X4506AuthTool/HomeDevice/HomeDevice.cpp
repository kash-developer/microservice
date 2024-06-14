// HomeDevice.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <serial_device.h>
#include <home_device_lib.h>
#include <trace.h>
#include <tools.h>

#include <controller_device.h>

#include <forwarder_lib.h>
#include <light_device.h>
#include <doorlock_device.h>
#include <vantilator_device.h>
#include <gasvalve_device.h>
#include <curtain_device.h>
#include <boiler_device.h>
#include <temperature_controller_device.h>
#include <breaker_device.h>
#include <prevent_crime_ext_device.h>
#include <system_aircon_device.h>
#include <powergate_device.h>
#include <remote_inspector_device.h>
#include <phone_device.h>
#include <entrance_device.h>

#include <json/json.h>

#include <stdio.h>
#include <string>
#include <sys/stat.h>

HomeDevice* createHomeDevice(std::string filename);

struct DeviceConf {
	int m_device_id;
	std::string m_name;
	std::string m_conf;
};

struct DeviceConf g_device_confs[] = {
	{ FORWARDER_DEVICE_ID, "", "forwarder.json" },
	{ CONTROLLER_DEVICE_ID, "", "controller.json" },
	{ SYSTEMAIRCON_DEVICE_ID, "", "system_aircon.json" },
	{ LIGHT_DEVICE_ID, "", "light.json" },
	{ GASVALVE_DEVICE_ID, "", "gasvalve.json" },
	{ CURTAIN_DEVICE_ID, "", "curtain.json" },
	{ REMOTEINSPECTOR_DEVICE_ID, "", "remote_inspector.json" },
	{ DOORLOCK_DEVICE_ID, "", "doorlock.json" },
	{ VANTILATOR_DEVICE_ID, "", "vantilator.json" },
	{ BREAKER_DEVICE_ID, "", "breaker.json" },
	{ PREVENTCRIMEEXT_DEVICE_ID , "", "prevent_crime_ext.json"},
	{ BOILER_DEVICE_ID, "", "boiler.json" },
	{ TEMPERATURECONTROLLER_DEVICE_ID, "", "temperature_controller.json" },
	{ POWERGATE_DEVICE_ID, "", "powergate.json" },
	{ PHONE_DEVICE_ID, "", "phone.json" },
	{ ENTRANCE_DEVICE_ID, "", "entrance.json" }
};

int main(int argc, char* argv[])
{
	std::vector<HomeDevice*> devices;
	Forwarder* forwarder;

	HomeDevice* hd;
	ControllerDevice* controller;
	std::string device_name;
	std::string filename;
	std::string base_dir;
	std::string err;
	char cmd;
	Json::Value conf_json;
	int registered_device_number;

	std::vector<std::string> arg_vs;
	int arg_c;

	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		printf("WSAStartup error\r\n");
		return -1;
	}

	traceSetLevel(99);
	forwarder = NULL;

	registered_device_number = sizeof(g_device_confs) / sizeof(struct DeviceConf);
	for (int i = 0; i<registered_device_number; i++) {
		g_device_confs[i].m_name = HomeDevice::getDeviceNameString(g_device_confs[i].m_device_id);
		trace("set name: %s", g_device_confs[i].m_name.c_str());
	}

	if (argc < 3) {
		printf("usage: ./home_device <basedir> <device1 [device2 ...]>\n");
		printf("	device(n): device name\n");
		printf("	device names:\n");
		for (int i = 0; i<registered_device_number; i++) {
			printf("		%s\n", g_device_confs[i].m_name.c_str());
		}

		//return -1;
		arg_vs.push_back("HomeDevice.exe");
		arg_vs.push_back("../../../data");
		arg_vs.push_back("Phone");
		/*
		arg_vs.push_back("SystemAircon");
		arg_vs.push_back("Light");
		arg_vs.push_back("GasValve");
		arg_vs.push_back("Curtain");
		arg_vs.push_back("RemoteInspector");
		arg_vs.push_back("DoorLock");
		arg_vs.push_back("Vantilator");
		arg_vs.push_back("Breaker");
		arg_vs.push_back("PreventCrimeExt");
		arg_vs.push_back("Boiler");
		arg_vs.push_back("TemperatureController");
		arg_vs.push_back("PowerGate");
		//*/

		base_dir = arg_vs[1];
		arg_c = arg_vs.size();
	}
	else {
		arg_c = argc;
		for (int i = 0; i < arg_c; i++) {
			std::string arg = argv[i];
			arg_vs.push_back(arg);
		}
		base_dir = arg_vs[1];
	}

	controller = NULL;

	for (int i = 2; i<arg_c; i++) {
		device_name = arg_vs[i];
		filename.clear();

		for (int j = 0; j<registered_device_number; j++) {
			if (device_name.compare(g_device_confs[j].m_name) == 0) {
				filename = g_device_confs[j].m_conf;
				filename = base_dir + "/" + filename;
				break;
			}
		}

		if (filename.empty() == true) {
			tracee("invalid type. skip: %s", device_name.c_str());
			continue;
		}

		if (device_name.compare("Forwarder") == 0) {
			if (parseJson(filename, &conf_json, &err) == false) {
				tracee("parse conf file failed: %s", argv[1]);
				tracee("%s", err.c_str());
				return -1;
			}

			forwarder = new Forwarder();
			if (forwarder->init(conf_json) < 0) {
				tracee("forwarder init failed.");
				return -1;
			}

			if (forwarder->run() < 0) {
				tracee("forwarder init failed.");
				return -1;
			}
		}
		else {
			hd = createHomeDevice(filename);
			if (hd == NULL) {
				tracee("create device failed: %s", filename.c_str());
				continue;
			}

			hd->setDataHomePath(base_dir);
			if (hd->run() < 0) {
				tracee("run home device failed.");
				return -1;
			}

			if(device_name.compare("Controller") == 0) {
				controller = (ControllerDevice*)hd;
			}
			devices.push_back(hd);
		}
	}

	while (true) {
		printf("enter command: ");
		scanf_s("%c", &cmd, sizeof(cmd));
		printf("command: %c\n", cmd);
		if (cmd == 'x') {
			printf("let's exit.\n");
			for (unsigned int i = 0; i<devices.size(); i++) {
				hd = devices[i];
				hd->stop();
				delete hd;
			}
			break;
		}
		else if (cmd == 'p') {
			printf("print device list.\n");
			if (controller != NULL) {
				controller->printDeviceList();
			}
		}
	}

	if (forwarder != NULL) {
		forwarder->stop();
		delete forwarder;
	}
	for (unsigned int i = 0; i < devices.size(); i++) {
		devices[i]->stop();
		delete devices[i];
	}
	devices.clear();

	return 0;
}

HomeDevice* createHomeDevice(std::string filename)
{
	HomeDevice* hd;
	Json::Value json_obj;
	Json::Value tmp_json_obj;
	std::string err;
	int device_id;

	if (parseJson(filename, &json_obj, &err) == false) {
		tracee("parse conf file failed: %s", filename.c_str());
		tracee("%s", err.c_str());
		return NULL;
	}

	device_id = 0;
	if (json_obj["Device"].type() == Json::objectValue) {
		tmp_json_obj = json_obj["Device"];
		if (tmp_json_obj["DeviceID"].type() == Json::intValue) {
			device_id = tmp_json_obj["DeviceID"].asInt();
		}
		else {
			tracee("there is no device id.");
			return NULL;
		}
	}
	else {
		tracee("there is no device.");
		return NULL;
	}

	hd = NULL;
	switch (device_id) {
	case CONTROLLER_DEVICE_ID:
		hd = new ControllerDevice();
		break;
	case LIGHT_DEVICE_ID:
		hd = new LightDevice();
		break;
	case DOORLOCK_DEVICE_ID:
		hd = new DoorLockDevice();
		break;
	case VANTILATOR_DEVICE_ID:
		hd = new VantilatorDevice();
		break;
	case GASVALVE_DEVICE_ID:
		hd = new GasValveDevice();
		break;
	case CURTAIN_DEVICE_ID:
		hd = new CurtainDevice();
		break;
	case BOILER_DEVICE_ID:
		hd = new BoilerDevice();
		break;
	case TEMPERATURECONTROLLER_DEVICE_ID:
		hd = new TemperatureControllerDevice();
		break;
	case BREAKER_DEVICE_ID:
		hd = new BreakerDevice();
		break;
	case PREVENTCRIMEEXT_DEVICE_ID:
		hd = new PreventCrimeExtDevice();
		break;
	case SYSTEMAIRCON_DEVICE_ID:
		hd = new SystemAirconDevice();
		break;
	case POWERGATE_DEVICE_ID:
		hd = new PowerGateDevice();
		break;
	case REMOTEINSPECTOR_DEVICE_ID:
		hd = new RemoteInspectorDevice();
		break;
	case PHONE_DEVICE_ID:
		hd = new PhoneDevice();
		break;
	case ENTRANCE_DEVICE_ID:
		hd = new EntranceDevice();
		break;
	default:
		tracee("invalid device id: %d", device_id);
		return NULL;
	}

	trace("init: %s", filename.c_str());
	if (hd->init(json_obj) < 0) {
		tracee("home device init failed: %s", filename.c_str());
		delete hd;
		return NULL;
	}

	return hd;
}
