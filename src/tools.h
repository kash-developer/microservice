
#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <json/json.h>

#include <string>

bool parseJson(const std::string filepath, Json::Value* json_obj, std::string* err_str);
bool parseJson(const char* start_str, const char* end_str, Json::Value* json_obj, std::string* err_str);

std::string getFileString(const std::string filename, std::string& mime);

int gettimeofday2(struct timeval *tv);
std::string int2hex_str(int i, int size = 2);
unsigned int hex_str2int(std::string hex_str);

#endif
