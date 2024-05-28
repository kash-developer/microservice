
#include "tools.h"
#include "trace.h"

#include <json/json.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <fstream>
//#include <magic.h>


#ifdef WIN32
#include <Windows.h>
//#include <WinSock2.h>
#else
#include <sys/time.h>
#endif

bool parseJson(const std::string filepath, Json::Value* json_obj, std::string* err_str)
{
	std::string json_str;
	std::string line;
	std::ifstream fin;

	fin.open(filepath.c_str(), std::ios::in);
	if(fin.is_open()){
		while(getline(fin, line)){
			json_str += line;
		}
	}
	else{
		tracee("file open failed: %s", filepath.c_str());
		return false;
	}
	fin.close();

	return parseJson(json_str.c_str(), json_str.c_str() + json_str.size(), json_obj, err_str);
}

bool parseJson(const char* start_str, const char* end_str, Json::Value* json_obj, std::string* err_str)
{
	Json::CharReaderBuilder builder;
	Json::CharReader* reader;

	reader = builder.newCharReader();

	if(reader->parse(start_str, end_str, json_obj, err_str) == false){
		tracee("parse conf file failed.");
		tracee("%s", err_str->c_str());
		delete reader;
		return false; 
	}   
	delete reader;

	return true;
}

std::string getFileString(const std::string filepath, std::string& mime)
{
	std::string line;
	std::ifstream fin;
	std::string ret;

	trace("get file: %s", filepath.c_str());

	fin.open(filepath.c_str(), std::ios::in);
	if(fin.is_open()){
		while(getline(fin, line)){
			ret += line + std::string("\r\n");
		}
	}
	else{
		tracee("fileopen failed: %s", filepath.c_str());
		return ret;
	}
	fin.close();

	if(filepath.find(".css") != std::string::npos){
		mime = "text/css";
	}
	else if(filepath.find(".js") != std::string::npos){
		mime = "application/js";
	}
	else if(filepath.find(".html") != std::string::npos){
		mime = "text/js";
	}
	else if(filepath.find(".htm") != std::string::npos){
		mime = "text/js";
	}

	return ret;
}

/*
magic_t __g_magic = NULL;
std::string getFileString(const std::string filepath, std::string& mime)
{
	std::string line;
	std::ifstream fin;
	std::string ret;

	if(__g_magic == NULL){
		__g_magic = magic_open(MAGIC_MIME_TYPE);
		magic_load(__g_magic, NULL);
		magic_compile(__g_magic, NULL);
	}

	fin.open(filepath.c_str(), std::ios::in);
	if(fin.is_open()){
		while(getline(fin, line)){
			ret += line + std::string("\r\n");
		}
	}
	else{
		tracee("fileopen failed: %s", filepath.c_str());
		return ret;
	}
	fin.close();

	mime = magic_file(__g_magic, filepath.c_str());
	tracee("get file success: %s, %s", filepath.c_str(), mime.c_str());

	if(filepath.find(".css") != std::string::npos){
		mime = "text/css";
	}
	else if(filepath.find(".js") != std::string::npos){
		mime = "application/js";
	}

	return ret;
}
*/


int gettimeofday2(struct timeval *tv)
{
#ifdef WIN32
	SYSTEMTIME t;
	if (NULL != tv)
	{
		GetSystemTime(&t);

		tv->tv_sec = (long)time(NULL);
		tv->tv_usec = (long)(t.wMilliseconds * 1000);
	}
#else
	gettimeofday(tv, NULL);
#endif

	return 0;
}

std::string int2hex_str(int i, int size) 
{
	std::string result;
	char format[255];
	char str[255];

	sprintf(format, "0x%%0%dx", size);
	sprintf(str, format, i);
	result = std::string(str);

	return result;
}

unsigned int hex_str2int(std::string hex_str)
{
	unsigned int hex;
	char* endptr[1];
	strtol(hex_str.c_str(), endptr, 0);
	if (*endptr == hex_str.c_str()) {
		tracee("No conversion could be performed: %s", hex_str.c_str());
		return 0;
	}
	hex = std::stoi(hex_str, 0, 16);

	return hex;
}
