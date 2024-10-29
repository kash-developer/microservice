// Write C++ code here.
//
// Do not forget to dynamically load the C++ library into your application.
//
// For instance,
//
// In MainActivity.java:
//    static {
//       System.loadLibrary("microdeviceservice");
//    }
//
// Or, in MainActivity.kt:
//    companion object {
//      init {
//         System.loadLibrary("microdeviceservice")
//      }
//    }
#include <jni.h>
#include <stdio.h>
#include <android/log.h>

#include "include/json/json.h"

#include "serial_device.h"
#include "home_device_lib.h"
#include "trace.h"
#include "tools.h"

#include "controller_device.h"

#include "light_device.h"
#include "doorlock_device.h"
#include "vantilator_device.h"
#include "gasvalve_device.h"
#include "curtain_device.h"
#include "boiler_device.h"
#include "temperature_controller_device.h"
#include "breaker_device.h"
#include "prevent_crime_ext_device.h"
#include "system_aircon_device.h"
#include "powergate_device.h"
#include "remote_inspector_device.h"
#include "forwarder_lib.h"
#include "phone_device.h"
#include "http_message.h"
#include "http_request.h"
#include "http_response.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <queue>

JavaVM* g_vm = NULL;

struct SerialDataBuf {
    int m_len;
    uint8_t* m_data;
};
std::queue<struct SerialDataBuf> g_serial_q;

struct JNI_DeviceStructure {
    int m_device_id;
    HomeDevice* m_device;
};

std::vector<struct JNI_DeviceStructure> g_devices;
Forwarder* g_forwarder = NULL;
std::string g_base_folder;


extern "C" int
parseConfJson(std::string str_json, Json::Value* json_result_obj)
{
    Json::Value json_obj;
    Json::Value tmp_json_obj;
    std::string err;
    int device_id;

    if(parseJson(str_json.c_str(), str_json.c_str() + str_json.size(), &json_obj, &err) == false){
        tracee("parse conf file failed: %s", str_json.c_str());
        tracee("%s", err.c_str());
        return -1;
    }

    device_id = 0;
    if(json_obj["Device"].type() == Json::objectValue){
        tmp_json_obj = json_obj["Device"];
        if(tmp_json_obj["DeviceID"].type() == Json::intValue){
            device_id = tmp_json_obj["ID"].asInt();
        }
        else{
            tracee("there is no device id.");
            return -1;
        }
    }
    else{
        tracee("there is no device.");
        return -1;
    }

    *json_result_obj = json_obj;

    return device_id;
}

extern "C" JNIEXPORT void JNICALL
Java_com_etri_microdeviceservice_DeviceService_cppSetBaseFolder(JNIEnv *jenv, jobject , jstring jstr_path)
{
    const char* tmp_char_p;
    std::string path;
    Json::Value json_obj;

    trace("in set base folder....");

    jenv->GetJavaVM(&g_vm);

    tmp_char_p = jenv->GetStringUTFChars(jstr_path, NULL);
    path = std::string(tmp_char_p);
    jenv->ReleaseStringUTFChars(jstr_path, tmp_char_p);

    g_base_folder = path;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_etri_microdeviceservice_DeviceService_cppStartForwarder(JNIEnv *jenv, jobject , jstring jstr_json)
{
    const char* tmp_char_p;
    std::string str_json;
    Json::Value json_obj;

    trace("in start forwarder.....");

    if(g_forwarder != NULL){
        trace("forwarder is already started.");
        return 0;
    }

    tmp_char_p = jenv->GetStringUTFChars(jstr_json, NULL);
    str_json = std::string(tmp_char_p);
    jenv->ReleaseStringUTFChars(jstr_json, tmp_char_p);

    trace("start forwarder.");
    trace("json: %s", str_json.c_str());

    /*
    if(parseConfJson(str_json, &json_obj) < 0){
        tracee("parse json failed.");
        return -1;
    }
     */

    g_forwarder = new Forwarder();
    if(g_forwarder->init(json_obj) < 0){
        tracee("forwarder init failed.");
        return -1;
    }
    trace("forwarder init success")
    if(g_forwarder->run() < 0){
        tracee("forwarder run failed.");
        delete g_forwarder;
        g_forwarder = NULL;
        return -1;
    }
    trace("forwarder started.")
    return 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_etri_microdeviceservice_DeviceService_cppStartDevice(JNIEnv *jenv, jobject , jint jtype, jstring jstr_json)
{
    trace("in start Device");

    std::string str_json;
    const char* tmp_char_p;
    int type;
    Json::Value json_obj;
    HomeDevice* hd;
    struct JNI_DeviceStructure ds;

    type = jtype;
    tmp_char_p = jenv->GetStringUTFChars(jstr_json, NULL);
    str_json = std::string(tmp_char_p);
    jenv->ReleaseStringUTFChars(jstr_json, tmp_char_p);

    trace("start device: %x", type);
    trace("json: %s", str_json.c_str());

    //check if exist
    for(unsigned int i=0; i<g_devices.size(); i++){
        ds = g_devices[i];
        if(ds.m_device_id == type){
            trace("the device is already started: %d", type);
            return 0;
        }
    }

    ds.m_device_id = type;
    hd = NULL;
    switch(type){
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
        default:
            tracee("invalid device id: %d", type);
            return -1;
    }
    ds.m_device = hd;

    if(parseConfJson(str_json, &json_obj) < 0){
        tracee("parse json failed.");
        return -1;
    }
    if(hd->init(json_obj) < 0){
        tracee("home device init failed.");
        delete ds.m_device;
        return -1;
    }
    hd->setDataHomePath(g_base_folder);
    if(hd->run() < 0){
        tracee("run home device failed.");
        delete ds.m_device;
        return -1;
    }

    g_devices.push_back(ds);

    return 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_etri_microdeviceservice_DeviceService_cppStopDevice(JNIEnv *jenv, jobject , jint jtype)
{
    int type;
    struct JNI_DeviceStructure ds;

    trace("in stop.");

    type = jtype;
    //check if exist
    for(unsigned int i=0; i<g_devices.size(); i++){
        ds = g_devices[i];
        trace("check: %d", ds.m_device_id);
        if(ds.m_device_id == type){
            trace("before stop")
            ds.m_device->stop();
            trace("after stop")
            delete ds.m_device;
            trace("after delete")

            g_devices.erase(g_devices.begin() + i);
            break;
        }
    }

    trace("return stop.");
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_etri_microdeviceservice_DeviceService_cpponNewData(JNIEnv *jenv, jobject , jbyteArray jarray)
{
    jbyte* jbuf;
    jsize jlen;
    uint8_t* buf;
    int len;

    trace("cpponNewData");

    if(g_forwarder == NULL){
        tracee("forwarder is NULL.");
        return;
    }

    jbuf = jenv->GetByteArrayElements(jarray, NULL);
    jlen = jenv->GetArrayLength(jarray);

    len = jlen;
    buf = new uint8_t[jlen];
    memcpy(buf, jbuf, len);

    jenv->ReleaseByteArrayElements(jarray, jbuf, 0);

    trace("before recv serial");
    g_forwarder->receivedSerial(buf, len);
    trace("after recv serial");

    delete[] buf;

    return;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_etri_microdeviceservice_DeviceService_pollSerialData(JNIEnv *jenv, jobject)
{
    jbyteArray jarray;
    jbyte* jbuf;
    struct SerialDataBuf serial_data_buf;

    if(g_serial_q.empty() == true){
        return NULL;
    }

    serial_data_buf = g_serial_q.front();
    g_serial_q.pop();

    //create param
    jbuf = new jbyte[serial_data_buf.m_len];
    memcpy(jbuf, serial_data_buf.m_data, serial_data_buf.m_len);

    jarray = jenv->NewByteArray(serial_data_buf.m_len);
    jenv->SetByteArrayRegion(jarray, 0, serial_data_buf.m_len, jbuf);

    delete[] serial_data_buf.m_data;

    trace("poll data. len: %d", serial_data_buf.m_len);

    return jarray;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_etri_microdeviceservice_DeviceService_cppClearMyAddress(JNIEnv *env, jobject thiz) {
    // TODO: implement cppClearMyAddress()
    HttpuServer::clearMyAddress();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_etri_microdeviceservice_DeviceService_cppAddMyAddress(JNIEnv *env, jobject thiz,
                                                               jstring jaddr) {
    // TODO: implement cppAddMyAddress()
    const char* tmp_char_p;
    std::string addr;

    tmp_char_p = env->GetStringUTFChars(jaddr, NULL);
    addr = std::string(tmp_char_p);
    env->ReleaseStringUTFChars(jaddr, tmp_char_p);

    HttpuServer::addMyAddress(addr);
}

extern "C" int
jni_writeSerial(const uint8_t* data, int len)
{
    struct SerialDataBuf serial_data_buf;

    serial_data_buf.m_len = len;
    serial_data_buf.m_data = new uint8_t[len];
    memcpy(serial_data_buf.m_data, data, len);

    g_serial_q.push(serial_data_buf);

    tracee("write to q. len: %d", len);

    return 0;
}



//donghee
std::queue<std::string> g_command_q;
PhoneDevice* g_phone_device;

bool recvCommand(Json::Value cmd)
{
    trace("received command.....\n%s", cmd.toStyledString().c_str());
    g_command_q.push(cmd.toStyledString());
	return true;
}

/*
bool recvHttpRequest(void* instance, HttpRequest* request, HttpResponse* response)
{
    Json::Value jobj_status;
    std::string body, mime_type;
    std::string method, path;
    char* tmp_body;
    int len;

    std::string tmp_str;
    std::vector<std::string> paths;
    size_t pos;

    trace("called http callback.");

    request->getMethod(&method);
    if(method.compare("POST") != 0){
        return false;
    }

    request->getPath(&path);
    tmp_str = path;
    while ((pos = tmp_str.find("/")) != std::string::npos) {
        if (tmp_str.substr(0, pos).size() == 0) {
            tmp_str.erase(0, 1);
            continue;
        }
        paths.push_back(tmp_str.substr(0, pos));
        tmp_str.erase(0, pos + 1);
    }
    if (tmp_str.size() != 0) {
        paths.push_back(tmp_str);
    }

    if (paths.size() == 0) {
        response->setResponseCode(404, "Not Found");
        return true;
    }
    if(paths[0].compare("control") == 0) {
        request->getBody((uint8_t**)&tmp_body, &mime_type);
        body = std::string(tmp_body);
        g_command_q.push(body);

        jobj_status = g_devices[0].m_device->getStatus("", "");

        response->setResponseCode(200, "OK");
        response->setBody((uint8_t*)jobj_status.toStyledString().c_str(), jobj_status.toStyledString().length(), "application/json");
        return true;
    }

    return false;
}
*/

extern "C" JNIEXPORT jint JNICALL
Java_com_tutpro_baresip_MicroServiceCommandProcessor_cppInitMicroService(JNIEnv *jenv, jobject thiz) {
    // TODO: implement initMicroService()
    return 0;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_tutpro_baresip_MicroServiceCommandProcessor_cppGetCommand(JNIEnv *jenv, jobject thiz) {
    // TODO: implement getCommand()
    //trace("called getCommand\n");

    std::string org_msg;
    jstring jmessage;

    if(g_command_q.empty() == true){
        ;
    }
    else{
        org_msg = g_command_q.front();
        g_command_q.pop();
    }

    jmessage = jenv->NewStringUTF(org_msg.c_str());

    //trace("returns getCommand\n");
    return jmessage;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_tutpro_baresip_MicroServiceCommandProcessor_cppSetBaesFolder(JNIEnv *jenv, jobject thiz,
                                                                   jstring jstr_path) {
    // TODO: implement setBaesFolder()
    trace("jni set base folder");
    Java_com_etri_microdeviceservice_DeviceService_cppSetBaseFolder(jenv, thiz, jstr_path);
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_tutpro_baresip_MicroServiceCommandProcessor_cppStartDevice(JNIEnv *jenv, jobject thiz,
                                                                    jint type, jstring jstr_json) {
    // TODO: implement cppStartDevice()
    Java_com_etri_microdeviceservice_DeviceService_cppStartDevice(jenv, thiz, type, jstr_json);
    g_phone_device = (PhoneDevice*)g_devices[0].m_device;
    //g_phone_device->setCallback(NULL, recvHttpRequest);

    //recvHttpRequest
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tutpro_baresip_MicroServiceCommandProcessor_cppAddMyAddress(JNIEnv *jenv, jobject thiz,
                                                                     jstring jaddr) {
    // TODO: implement cppAddMyAddress()
    const char* tmp_char_p;
    std::string addr;

    tmp_char_p = jenv->GetStringUTFChars(jaddr, NULL);
    addr = std::string(tmp_char_p);
    jenv->ReleaseStringUTFChars(jaddr, tmp_char_p);

    HttpuServer::addMyAddress(addr);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tutpro_baresip_MicroServiceCommandProcessor_cppClearMyAddress(JNIEnv *env, jobject thiz) {
    // TODO: implement cppClearMyAddress()
    HttpuServer::clearMyAddress();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_tutpro_baresip_MicroServiceCommandProcessor_cppSetLastCall(JNIEnv *jenv, jobject thiz,
                                                                     jstring jlast_call) {
    // TODO: implement cppSetLastCCall()
    const char* tmp_char_p;
    std::string last_call;

    tmp_char_p = jenv->GetStringUTFChars(jlast_call, NULL);
    last_call = std::string(tmp_char_p);
    jenv->ReleaseStringUTFChars(jlast_call, tmp_char_p);

    g_phone_device->setLastCall(0x0f, last_call);
}
