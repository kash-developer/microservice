
#ifndef MYTRACE_H
#define MYTRACE_H

#include <stdio.h>
#include <mutex>

#define TRACE_DEFAULT_FILENAME	"./trace.log"

typedef void (*traceLogCallback)(void* instance, int level, const char* msg);

#define TRACE_BUFFER_MARGINE	1024
#define TRACE_DEFAULT_DEBUG_LEVEL	8

class MyTrace {
private:
	int m_level;
	int m_callback_level;

	traceLogCallback m_callback;
	void* m_callback_instance;

	std::mutex m_trace_mutex;


	FILE* m_fp;
	FILE* m_fp2;

public:
	MyTrace();
	~MyTrace();

	char m_filename[255];
	char m_filename2[255];

	int m_enable_trace2;

	void reopen(const char* filename, const char* filename2);
	void printLog(int level, const char* file, int line, const char* func, const char* fmt, ...);
	void printLog2(int level, const char* file, int line, const char* func, const char* fmt, ...);

	void setLevel(int level);
	void setCallback(void* instance, int level, traceLogCallback callback);
	void getFilename(char* filename, char* filename2);
};

#ifdef WIN32
extern MyTrace __g_trace_debug__;

#define trace(fmt, ...) __g_trace_debug__.printLog(9, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define trace2(fmt, ...) __g_trace_debug__.printLog2(3, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define tracel(l, fmt, ...) __g_trace_debug__.printLog(l, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define tracel2(l, fmt, ...) __g_trace_debug__.printLog2(l, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define tracee(fmt, ...) __g_trace_debug__.printLog(0, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define traceSetLevel(l)	__g_trace_debug__.setLevel(l)
#define traceSetCallback(ins, level, callback)	__g_trace_debug__.setCallback(ins, level, callback)
#define traceGetFilename(filename, filename2)	__g_trace_debug__.getFilename(filename, filename2)
#else
#ifdef __ANDROID__
#include <android/log.h>

#define trace(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "**cpp", fmt, ##__VA_ARGS__);
#define trace2(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "**cpp", fmt, ##__VA_ARGS__);
#define tracel(l, fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "**cpp", fmt, ##__VA_ARGS__);
#define tracel2(l, fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "**cpp", fmt, ##__VA_ARGS__);
#define tracee(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "**cpp", fmt, ##__VA_ARGS__);
#define traceSetLevel(l)	;
#define traceSetCallback(ins, level, callback)	;
#define traceGetFilename(filename, filename2)	;
#else
extern MyTrace __g_trace_debug__;

#define trace(fmt, ...) __g_trace_debug__.printLog(9, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define trace2(fmt, ...) __g_trace_debug__.printLog2(3, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define tracel(l, fmt, ...) __g_trace_debug__.printLog(l, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define tracel2(l, fmt, ...) __g_trace_debug__.printLog2(l, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define tracee(fmt, ...) __g_trace_debug__.printLog(0, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define traceSetLevel(l)	__g_trace_debug__.setLevel(l)
#define traceSetCallback(ins, level, callback)	__g_trace_debug__.setCallback(ins, level, callback)
#define traceGetFilename(filename, filename2)	__g_trace_debug__.getFilename(filename, filename2)
#endif
#endif


//#define traceStart(a)	MyTrace(a)
void traceEnableTrace2();
void traceStart(const char* id);


#endif
