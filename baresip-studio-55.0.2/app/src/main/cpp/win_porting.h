
#ifndef __WIN_PORTING_H__
#define __WIN_PORTING_H__

#ifdef WIN32
//#include <Windows.h>

#define sleep(x) Sleep((x)*1000)
#define usleep(x) Sleep((x)/1000)

#else
#define HANDLE int
#define SOCKET int
#define INVALID_HANDLE_VALUE -1
#define INVALID_SOCKET -1
#define closesocket close
#define CloseHandle close
#endif

#endif

