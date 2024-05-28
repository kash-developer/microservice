
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

#include "trace.h"
#include <tools.h>

#ifdef WIN32
#include <WinSock2.h>
#else
#include <sys/time.h>
#endif

#ifdef __ANDROID__
#else
MyTrace __g_trace_debug__;
#endif

void traceStart(const char* id)
{
#ifdef __ANDROID__
#else
	sprintf(__g_trace_debug__.m_filename, "./log/trace_%s.log", id);
	sprintf(__g_trace_debug__.m_filename2, "./log/trace_process_%s.log", id);
	__g_trace_debug__.reopen(__g_trace_debug__.m_filename, __g_trace_debug__.m_filename2);
#endif
}

void traceEnableTrace2()
{
#ifdef __ANDROID__
#else
	__g_trace_debug__.m_enable_trace2 = 1;
#endif
}

/**
	@brief	constructor
*/
MyTrace::MyTrace()
{
	m_fp = NULL;
	m_fp2 = NULL;

	m_callback = NULL;

	m_level = TRACE_DEFAULT_DEBUG_LEVEL;
	m_callback_level = TRACE_DEFAULT_DEBUG_LEVEL;
	m_enable_trace2 = 0;

	strcpy(m_filename, TRACE_DEFAULT_FILENAME);
	strcpy(m_filename2, "");

	printf("open for log: %s\n", m_filename);
	m_fp = fopen(m_filename, "w");

	if(m_fp == NULL){
		printf("cannot create trace.log: %s\n", m_filename);
		strcpy(m_filename, "./trace.log");
		printf("open for log: %s", m_filename);
		m_fp = fopen("./trace.log", "w");
		//return;
	}
	fputs("Start---------------------", m_fp);

	fflush(m_fp);
}

MyTrace::~MyTrace()
{
	if(m_fp != NULL){
		fclose(m_fp);
	}
	if(m_fp2 != NULL){
		fclose(m_fp2);
	}
}

void MyTrace::setLevel(int level)
{
	m_level = level;
}

void MyTrace::getFilename(char* filename, char* filename2)
{
	strcpy(filename, m_filename);
	strcpy(filename2, m_filename2);
}

void MyTrace::setCallback(void* instance, int level, traceLogCallback callback)
{
	m_callback_instance = instance;
	m_callback_level = level;
	m_callback = callback;
}


void MyTrace::reopen(const char* filename, const char* filename2)
{
	//open filename
	if(m_fp != NULL){
		fclose(m_fp);
	}
	remove(filename);
	remove(filename2);
	rename(TRACE_DEFAULT_FILENAME, filename);

	m_fp = NULL;

	printf("reopen for log: %s, %s\n", filename, filename2);
	m_fp = fopen(filename, "a");

	if(m_fp == NULL){
		printf("reopen for log failed: %s, %s\n", filename, filename2);
		return;
	}

	//open filename2
	if(m_enable_trace2 == 1){
		if(m_fp2 != NULL){
			fclose(m_fp2);
		}
		m_fp2 = NULL;
		m_fp2 = fopen(filename2, "w+");

		if(m_fp2 == NULL){
			return;
		}

		fflush(m_fp2);
	}
}

void MyTrace::printLog(int level, const char* file, int line, const char* func, const char* fmt, ...)
{
	const char* tc;
	char file2[1024];
	va_list args;
	time_t cur_time;
	struct timeval tv;
	struct tm *t;
	char* buffer;
	char* buffer2;

	int arg_started;
	int long_flag;
	int string_length;
	const char* p;
	const char* param_string;

	m_trace_mutex.lock();

	string_length = (int)strlen(fmt);

	va_start(args, fmt);
	arg_started = 0;
	long_flag = 0;
	for (p=fmt; *p; p++) {
		if(arg_started == 1){
			if(*p == '%'){
				arg_started = 1;
				printf("invalid arg: %s\n", fmt);
				va_end(args);
				return;
			}

			switch(*p){
			case 'u':
			case 'd':
			case 'x':
				if(long_flag == 0){
					va_arg(args, int);
				}
				else if(long_flag == 1){
					va_arg(args, long);
				}
				else{
					va_arg(args, long long);
				}
				arg_started = 0;
				break;
			case 'f':
				va_arg(args, double);
				arg_started = 0;
				break;
			case 'c':
				//va_arg(args, char);
				va_arg(args, int);
				arg_started = 0;
				break;
			case 's':
				param_string = va_arg(args, char*);
				string_length += (int)strlen(param_string);
				arg_started = 0;
				break;
			case 'l':
				long_flag++;
				break;
			default:
				arg_started = 0;
			}
		}
		else{
			if(*p == '%'){
				arg_started = 1;
			}
		}
	}
	va_end(args);

	buffer = new char[string_length + TRACE_BUFFER_MARGINE];
	buffer2 = new char[string_length + TRACE_BUFFER_MARGINE];
	memset(buffer, 0, string_length + TRACE_BUFFER_MARGINE);
	memset(buffer2, 0, string_length + TRACE_BUFFER_MARGINE);

	strcpy(file2, file);

	tc = strrchr(file, '/');
	if(tc != NULL){
		strcpy(file2, tc+1);
	}
	else{
		tc = strrchr(file, '\\');
		if(tc != NULL){
			strcpy(file2, tc+1);
		}
	}

	time(&cur_time);
	t = localtime(&cur_time);
	gettimeofday2(&tv);

	va_start(args, fmt);
	vsprintf(buffer2, fmt, args);
	strcat(buffer2, "\n");
	va_end(args);

	sprintf(buffer, "<%02d:%02d:%02d:%03d> [%s:%d:%s()] \t", 
										t->tm_hour, t->tm_min, t->tm_sec, (int)tv.tv_usec/1000,
										file2, line, func);

	strcat(buffer, buffer2);

	if(level <= m_level){
		if(m_fp != NULL){
			fputs(buffer, m_fp);
			fflush(m_fp);
		}
		printf("%s", buffer);
	}

	if(level <= m_callback_level){
		if(m_callback != NULL){
			m_callback(m_callback_instance, level, buffer);
		}
	}

	delete[] buffer;
	delete[] buffer2;

	m_trace_mutex.unlock();
}

void MyTrace::printLog2(int level, const char* file, int line, const char* func, const char* fmt, ...)
{
	const char* tc;
	char file2[1024];
	va_list args;
	time_t cur_time;
	struct tm *t;
	char* buffer;
	char* buffer2;

	int arg_started;
	int long_flag;
	int string_length;
	const char* p;
	const char* param_string;

	m_trace_mutex.lock();

	string_length = (int)strlen(fmt);

	va_start(args, fmt);
	arg_started = 0;
	long_flag = 0;
	for (p=fmt; *p; p++) {
		if(arg_started == 1){
			if(*p == '%'){
				arg_started = 1;
				printf("invalid arg: %s\n", fmt);
				va_end(args);
				return;
			}

			switch(*p){
			case 'u':
			case 'd':
			case 'x':
				if(long_flag == 0){
					va_arg(args, int);
				}
				else if(long_flag == 1){
					va_arg(args, long);
				}
				else{
					va_arg(args, long long);
				}
				arg_started = 0;
				break;
			case 'f':
				va_arg(args, double);
				arg_started = 0;
				break;
			case 'c':
				//va_arg(args, char);
				va_arg(args, int);
				arg_started = 0;
				break;
			case 's':
				param_string = va_arg(args, char*);
				string_length += (int)strlen(param_string);
				arg_started = 0;
				break;
			case 'l':
				long_flag++;
				break;
			default:
				arg_started = 0;
			}
		}
		else{
			if(*p == '%'){
				arg_started = 1;
			}
		}
	}
	va_end(args);

	buffer = new char[string_length + TRACE_BUFFER_MARGINE];
	buffer2 = new char[string_length + TRACE_BUFFER_MARGINE];
	memset(buffer, 0, string_length + TRACE_BUFFER_MARGINE);
	memset(buffer2, 0, string_length + TRACE_BUFFER_MARGINE);

	strcpy(file2, file);

	tc = strrchr(file, '/');
	if(tc != NULL){
		strcpy(file2, tc+1);
	}
	else{
		tc = strrchr(file, '\\');
		if(tc != NULL){
			strcpy(file2, tc+1);
		}
	}

	time(&cur_time);
	t = localtime(&cur_time);

	va_start(args, fmt);
	vsprintf(buffer2, fmt, args);
	strcat(buffer2, "\n");
	va_end(args);

	sprintf(buffer, "<%02d:%02d:%02d> [%s:%d:%s()] \t", 
										t->tm_hour, t->tm_min, t->tm_sec, file2, line, func);
	strcat(buffer, buffer2);


	if(level <= m_level){
		if(m_fp != NULL){
			fputs(buffer, m_fp);
			fflush(m_fp);
		}
		printf("%s\n", buffer);
	}

	sprintf(buffer, "<%02d:%02d:%02d> ", t->tm_hour, t->tm_min, t->tm_sec);
	strcat(buffer, buffer2);

	if(level <= m_level){
		if(m_fp2 != NULL){
			fputs(buffer, m_fp2);
			fflush(m_fp2);
		}
	}


	if(level <= m_callback_level){
		if(m_callback != NULL){
			m_callback(m_callback_instance, level, buffer);
		}
	}

	delete[] buffer;
	delete[] buffer2;

	m_trace_mutex.unlock();
}

