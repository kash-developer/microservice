

#include "win_porting.h"
#include "serial_device.h"
#include "trace.h"
#include "tools.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>

///for UDP
#ifdef WIN32
#include <Ws2tcpip.h>
//#include <WinBase.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

void serial_multicast_udp_callback(void* instance, uint8_t* bytes, int length, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	SerialDevice* serial_device;

	serial_device = (SerialDevice*)instance;
	serial_device->runReadThread_multicast(bytes, length, sock_info, from_addr);
}

SerialDevice::SerialDevice()
{
	m_use_serial = -1;
}

SerialDevice::~SerialDevice()
{
	stop();
}

int SerialDevice::init()
{
	return init(SERIAL_MULTICAST_PORT);
}


int SerialDevice::init(int port)
{
	m_serial_read_thread = NULL;
	m_run_flag = false;
	m_byte_delay_test_flag = false;

	m_httpu_server = new HttpuServer();
	if (m_httpu_server->init(SERIAL_MULTICAST_ADDR, SERIAL_MULTICAST_PORT) < 0) {
		tracee("httpu server init failed.");
		delete m_httpu_server;
		m_httpu_server = NULL;
		return -1;
	}
	m_httpu_server->setCallback(this, serial_multicast_udp_callback);
	m_use_serial = 0;

	return 0;
}

//init for serial port
int SerialDevice::init(const std::string port, int speed)
{
	if(m_use_serial != -1){
		tracee("SerialDevice is already initialized: %d", m_use_serial);
		return -1;
	}

	m_serial_read_thread = NULL;
	m_run_flag = false;
	m_byte_delay_test_flag = false;

#ifdef WIN32
	std::wstring com_port;

	//wsprintf(com_port, TEXT("\\\\.\\%s"), m_portname);
	com_port = std::wstring((port).begin(), (port).end());
	m_sfd = CreateFile(com_port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (m_sfd == INVALID_HANDLE_VALUE) {
		tracee("open serial port failed: %s, %s", port.c_str(), strerror(errno));
		return -1;
	}
#else
	m_sfd = open(port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
	if(m_sfd < 0){
		tracee("open serial port failed.");
		return -1;
	}
#endif

	if(setInterfaceAttrib(m_sfd, speed) < 0){
		tracee("set attribute failed.");
		return -1;
	}

	m_com_port = port;
	m_com_speed = speed;

	m_use_serial = 1;
	trace("open serial success: %s, %d", port.c_str(), speed);
	return 0;
}

int SerialDevice::setInterfaceAttrib (HANDLE fd, int speed)
{
#ifdef WIN32
	DCB dcbSerialParams = { 0 };

	if (!GetCommState(m_sfd, &dcbSerialParams)) {
		tracee("GetCommState failed.");
		return -1;
	}

	//dcbSerialParams.BaudRate=CBR_38400;
	dcbSerialParams.BaudRate = speed;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;

	if (!SetCommState(m_sfd, &dcbSerialParams)) {
		tracee("SetCommState failed.");
		return -1;
	}

	COMMTIMEOUTS timeouts = { 0 };

	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (!SetCommTimeouts(m_sfd, &timeouts)) {
		return -1;
	}
#else
	struct termios tty;
	int parity = 0;

	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0){
		tracee("error %d from tcgetattr\n", errno);
		return -1;
	}

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;	 // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;		 // disable break processing
	tty.c_lflag = 0;				// no signaling chars, no echo,
										// no canonical processing
	tty.c_oflag = 0;				// no remapping, no delays
	tty.c_cc[VMIN]  = 0;			// read block
	tty.c_cc[VTIME] = 10;			// 1 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
										// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);	  // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0){
		tracee("error %d from tcsetattr\n", errno);
		return -1;
	}

	if (tcgetattr (fd, &tty) != 0){
		tracee("error %d from tggetattr\n", errno);
		return -1;
	}
#endif

	return 0;
}


void SerialDevice::setByteDelayTest(bool flag)
{
	m_byte_delay_test_flag = flag;
	return;
}

int SerialDevice::run()
{
	if(m_use_serial == 0) {
		m_httpu_server->run();
		//m_read_thread = new std::thread(&SerialDevice::runReadThread_multicast, this);
	}
	if((m_use_serial == 1) && (m_serial_read_thread == NULL)){
		m_serial_read_thread = new std::thread(&SerialDevice::runReadThread_serial, this);
	}

	m_run_flag = true;
	return 0;
}

void SerialDevice::stop()
{
	trace("let's stop serial device.");
	m_run_flag = false;

	if (m_httpu_server != NULL) {
		m_httpu_server->stop();
	}

	if(m_serial_read_thread != NULL){
		m_serial_read_thread->join();
		delete m_serial_read_thread;
	}
	m_serial_read_thread = NULL;

	trace("serial device stopped.");

	return;
}

int SerialDevice::get(SerialQueueInfo* q_info)
{
	struct SerialQueueInfo* tmp_q_info;

	m_queue_mutex.lock();
	if(m_queue.empty() == true){
		m_queue_mutex.unlock();
		return -1;
	}

	tmp_q_info = m_queue.front();
	trace("get serial: %x, %d", tmp_q_info, m_queue.size());
	memcpy(q_info, tmp_q_info, sizeof(struct SerialQueueInfo));
	delete tmp_q_info;
	m_queue.pop();
	
	m_queue_mutex.unlock();
	return 0;
}

void SerialDevice::getChecksum(const uint8_t* bytes, int len, uint8_t* xor_sum, uint8_t* add_sum)
{
	std::string str;

	*xor_sum = 0;
	*add_sum = 0;

	for(int i=0; i<len; i++){
		*xor_sum = (*xor_sum) ^ bytes[i];
		*add_sum = (*add_sum) + bytes[i];
	}
	*add_sum = *(add_sum) + (*xor_sum);

	//trace("checksum: %d, %s ==> %x, %x", len, printBytes(bytes, len).c_str(), *xor_sum, *add_sum);

	return;
}


void SerialDevice::runReadThread_multicast(uint8_t* bytes, int length, struct UdpSocketInfo sock_info, struct sockaddr_in from_addr)
{
	struct SerialQueueInfo* q_info;
	std::string bytes_string;

	bytes_string = printBytes(bytes, length);
	trace("serial data received: %s", bytes_string.c_str());

	if (bytes[0] != FRAME_START_BYTE) {
		tracee("invalid start: %x", bytes[0]);
		return;
	}

	m_queue_mutex.lock();
	q_info = new struct SerialQueueInfo;
	//q_info.m_addr = new uint8_t[length];
	memcpy(q_info->m_addr, bytes, length);
	q_info->m_len = length;
	gettimeofday2(&(q_info->m_arrived));
	q_info->m_serial_byte_delay = 0;
	//trace("serial arrived: %ld, %ld", q_info.m_arrived.tv_sec, q_info.m_arrived.tv_usec);

	m_queue.push(q_info);
	trace("push queue: %x, %d", q_info, q_info->m_len);
	m_queue_mutex.unlock();
}

void SerialDevice::runReadThread_serial()
{
	uint8_t c;
	uint8_t buf[1024];
	std::string bytes_string;
	long sec_diff, msec_diff;
	struct timeval start, end;
	int idx;
	uint8_t data_len;

	struct SerialQueueInfo* q_info;
	long max_byte_delay, delay;

#ifdef WIN32
	DWORD dw_bytes_read = 0;
#endif

	trace("serial receiver thread start.");

	idx = 0;
	data_len = 0;
	max_byte_delay = 0;

	while(true){
		if(m_run_flag == false){
			break;
		}

#ifdef WIN32
		if (ReadFile(m_sfd, &c, sizeof(char), &dw_bytes_read, NULL) == FALSE) {
			tracee("read serial port failed.");
			sleep(1);
			break;
		}
		if (dw_bytes_read <= 0) {
			continue;
		}
#else
		if(read(m_sfd, &c, sizeof(char)) < 0){
			tracee("read serial port failed.");
			sleep(1);
			break;
		}
#endif

		//get start frame
		if(c == FRAME_START_BYTE){
			if(idx !=0){
				tracee("something wrong. unexpected start bit: %d", idx);
				bytes_string = printBytes(buf, idx);
				tracee("%s", bytes_string.c_str());
			}
			idx = 0;
			data_len = 0;
			if (m_byte_delay_test_flag == true) {
				max_byte_delay = 0;
				gettimeofday2(&start);
			}
		}

		buf[idx] = c;

		//get data len
		if(idx == FRAME_DATALENGTH_IDX){
			data_len = c;
		}
		
		idx++;

		if (m_byte_delay_test_flag == true) {
			gettimeofday2(&end);

			sec_diff = (end.tv_sec - start.tv_sec) * 1000;
			msec_diff = (end.tv_usec - start.tv_usec) / 1000;
			delay = sec_diff + msec_diff;

			if (delay > max_byte_delay) {
				max_byte_delay = delay;
			}

			//trace("start: %ld, %ld,   end: %ld, %ld", start.tv_sec, start.tv_usec, end.tv_sec, end.tv_usec);
			//trace("sec_diff: %ld, msec_diff: %ld / %d", sec_diff, msec_diff, q_info.m_len);
		}

		if(idx == FRAME_HEADER_LENGTH + data_len + FRAME_TAIL_LENGTH){
			//bytes_string = printBytes(buf, idx);
			//trace("received: %s", bytes_string.c_str());

			gettimeofday2(&end);

			m_queue_mutex.lock();
			q_info = new struct SerialQueueInfo;
			memset(q_info->m_addr, 0, 4096);
			//q_info.m_addr = new uint8_t[idx];
			q_info->m_len = idx;
			q_info->m_serial_byte_delay = max_byte_delay;
			gettimeofday2(&(q_info->m_arrived));
			memcpy(q_info->m_addr, buf, idx);
			m_queue.push(q_info);
			m_queue_mutex.unlock();

			idx = 0;
		}
	}
}

std::string SerialDevice::printBytes(const uint8_t* bytes, int len)
{
	char str_buf[16];
	std::string ret_string;

	for(int i=0; i<len; i++){
		sprintf(str_buf, "%02x ", bytes[i]);
		ret_string += std::string(str_buf);
	}

	return ret_string;
}


#ifdef __ANDROID__
extern "C" int jni_writeSerial(const uint8_t* data, int len);
#endif
int SerialDevice::send(const uint8_t* bytes, int len)
{
	int ret;

	ret = -1;
	if (m_use_serial == 0) {
		ret = send_multicast(bytes, len);
#ifdef __ANDROID__
		jni_writeSerial(bytes, len);
#endif
	}
	else if (m_use_serial == 1) {
		ret = send_serial(bytes, len);
	}
	else {
		tracee("use serial flag: %d", m_use_serial);
	}

	return ret;
}


int SerialDevice::send_multicast(const uint8_t* bytes, int len)
{
	std::vector<struct UdpSocketInfo> sock_infos;

	std::string bytes_string;

	bytes_string = printBytes(bytes, len);
	//trace("serial data send: %s", bytes_string.c_str());
	sock_infos = m_httpu_server->getSocketInfos();
	for (unsigned int i = 0; i < sock_infos.size(); i++) {
		m_httpu_server->send(sock_infos[i].m_socket, SERIAL_MULTICAST_ADDR, SERIAL_MULTICAST_PORT, bytes, len);
	}

	return 0;
}

int SerialDevice::send_serial(const uint8_t* bytes, int len)
{
	int data_len;
	//std::string bytes_string;
#ifdef WIN32
	DWORD dw_bytes_write;
#endif

	if(bytes[0] != FRAME_START_BYTE){
		trace("first byte is not start byte: %02x / %02x", bytes[0], FRAME_START_BYTE);
		return -1;
	}

	data_len = bytes[4];
	if(data_len + FRAME_HEADER_LENGTH + FRAME_TAIL_LENGTH != len){
		trace("invalid length: %d / %d", len, data_len + FRAME_HEADER_LENGTH + FRAME_TAIL_LENGTH);
		return -1;
	}

	//bytes_string = printBytes(bytes, len);
	//trace("sent: %s", bytes_string.c_str());

#ifdef WIN32
	if (WriteFile(m_sfd, bytes, len, &dw_bytes_write, NULL) == FALSE) {
		tracee("write to serial port failed.");
		return -1;
	}
#else
	if (write(m_sfd, bytes, len) < 0) {
		tracee("write to serial port failed.");
		return -1;
}
#endif

	return 0;
}
