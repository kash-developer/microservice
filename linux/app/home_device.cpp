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

#include <json/json.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define ORG_DATA_HOME_PATH	"../data"

void copyFiles(std::string base_dir);
HomeDevice* createHomeDevice(std::string filename);

struct DeviceConf {
	std::string m_type;
	std::string m_conf;
};

#define MAX_DEVICE_LENGTH	14	
struct DeviceConf g_device_confs[] = {
	{"Forwarder", "forwarder_linux.json"},
	{"Controller", "controller.json"},
	{"SystemAircon", "system_aircon.json"},
	{"Light", "light.json"},
	{"GasValve", "gasvalve.json"},
	{"Curtain", "curtain.json"},
	{"RemoteInspector", "remote_inspector.json"},
	{"DoorLock", "doorlock.json"},
	{"Vantilator", "vantilator.json"},
	{"Breaker", "breaker.json"},
	{"PreventCrimeExt", "prevent_crime_ext.json"},
	{"Boiler", "boiler.json"},
	{"TemperatureController", "temperature_controller.json"},
	{"PowerGate", "powergate.json"}
};

int main(int argc, char* argv[])
{
	std::vector<HomeDevice*> devices;
	Forwarder forwarder;

	HomeDevice* hd;
	std::string device_type;
	std::string filename;
	std::string base_dir;
	std::string err;
	char cmd;
	Json::Value conf_json;

	traceSetLevel(99);

	if(argc < 3){
		printf("usage: ./home_device <basedir> <device1 [device2 ...]>\n");
		return -1;
	}

	base_dir = argv[1];

	copyFiles(base_dir);

	for(int i=2; i<argc; i++){
		device_type = argv[i];
		filename.clear();

		for(int j=0; j<MAX_DEVICE_LENGTH; j++){
			if(device_type.compare(g_device_confs[j].m_type) == 0){
				filename = g_device_confs[j].m_conf;
				filename = base_dir + "/" + filename;
				break;
			}
		}

		if(filename.empty() == true){
			tracee("invalid type. skip: %s", device_type.c_str());
			continue;
		}

		if(device_type.compare("Forwarder") == 0){
			if(parseJson(filename, &conf_json, &err) == false){
				tracee("parse conf file failed: %s", argv[1]);
				tracee("%s", err.c_str());
				return -1;
			}

			if(forwarder.init(conf_json) < 0){
				tracee("forwarder init failed.");
				return -1;
			}

			if(forwarder.run() < 0){
				tracee("forwarder init failed.");
				return -1;
			}

			continue;
		}

		hd = createHomeDevice(filename);
		if(hd == NULL){
			tracee("create device failed: %s", filename.c_str());
			continue;
		}

		hd->setDataHomePath(base_dir);
		if(hd->run() < 0){
			tracee("run home device failed.");
			return -1;
		}

		devices.push_back(hd);
	}

	while(true){
		printf("enter command: ");
		scanf("%c", &cmd);
		if(cmd == 'x'){
			for(unsigned int i=0; i<devices.size(); i++){
				hd = devices[i];
				hd->stop();
				delete hd;
			}
			break;
		}
		sleep(1);
	}

	return 0;
}

HomeDevice* createHomeDevice(std::string filename)
{
	HomeDevice* hd;
	Json::Value json_obj;
	Json::Value tmp_json_obj;
	std::string err;
	int device_id;

	if(parseJson(filename, &json_obj, &err) == false){
		tracee("parse conf file failed: %s", filename.c_str());
		tracee("%s", err.c_str());
		return NULL;
	}

	device_id = 0;
	if(json_obj["Device"].type() == Json::objectValue){
		tmp_json_obj = json_obj["Device"];
		if(tmp_json_obj["DeviceID"].type() == Json::intValue){
			device_id = tmp_json_obj["DeviceID"].asInt();
		}
		else{
			tracee("there is no device id.");
			return NULL;
		}
	}
	else{
		tracee("there is no device.");
		return NULL;
	}

	hd = NULL;
	switch(device_id){
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
	default:
		tracee("invalid device id: %d", device_id);
		return NULL;
	}

	if(hd->init(json_obj) < 0){
		tracee("home device init failed: %s", filename.c_str());
		delete hd;
		return NULL;
	}

	return hd;
}

void copyFiles(std::string base_dir)
{
	struct stat buf;
	std::string path_org;
	std::string path_target;
	std::string cmd;

	cmd = "mkdir -p " + base_dir;
	system(cmd.c_str());

	for(int i=0; i<MAX_DEVICE_LENGTH; i++){
		path_org = ORG_DATA_HOME_PATH + std::string("/") + g_device_confs[i].m_conf;
		path_target = base_dir + std::string("/") + g_device_confs[i].m_conf;

		if(stat(path_target.c_str(), &buf) < 0){
			cmd = "cp -f " + path_org + " " + path_target;
			system(cmd.c_str());
		}
	}
	
	path_org = ORG_DATA_HOME_PATH + std::string("/web");
	path_target = base_dir + std::string("/web");
	if(stat(path_target.c_str(), &buf) < 0){
		cmd = "cp -rf " + path_org + " " + path_target;
		system(cmd.c_str());
	}

	return;
}
