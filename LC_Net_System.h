#pragma once
#include <iostream>
#include <assert.h>

#include "UT_Sigtion.h"
#include "LC_SystemBase.h"
#include <ctime>
#include <cstring>
#include <map>

#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") 
#else
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif // WIN32

#ifdef WIN32
constexpr auto default_socket_num = 30;
#else
constexpr auto default_socket_num = 20000;
#endif // WIN32

constexpr auto socket_expire_time = 10;
constexpr auto ip_size = 30;
constexpr auto message_buff_size = 40 * 1024;

enum :uint8_t
{
	SKT_DEFAULT,
	SKT_LISTEN,
	SKT_CONNECT,
};
struct stSocketInfo
{
	int32_t m_iSocketfd = 0;
	char  m_cIp[ip_size] = {};
	uint16_t m_uiPort = 0;
	uint8_t m_uiSocketType = SKT_DEFAULT;
	uint32_t m_uiLastActiveTime = 0;
	uint8_t m_MessageBuff[message_buff_size] = {};
	uint16_t m_uiRecvSize = 0;
	bool	m_bCloseFlag = false;

	void Clear()
	{
		m_iSocketfd = 0;
		m_uiPort = 0;
		m_uiSocketType = SKT_DEFAULT;
		m_uiLastActiveTime = 0;
		m_bCloseFlag = false;
		memset(m_MessageBuff, 0, message_buff_size);
		memset(m_cIp, 0, ip_size);
	}
};

class CSocketGroup :public Sigtion<CSocketGroup>
{
public:

	std::map<uint32_t, stSocketInfo*> m_mSocket;

	~CSocketGroup();

	void TimeCheck();
};

class CNetWork :public Sigtion<CNetWork>
{
public:
	bool Init();
	bool startListen(uint32_t uiPort);
	void handleAccept(uint32_t uiFd);
	bool handlerConnect(const std::string& strIp, int32_t uiPort);

	int32_t handleRecv(uint32_t uiFd);
	int32_t handleSend();

#ifdef WIN32
	void run_select();

#else
	int32_t epoll_fd;

	void epoll_change_event(int op, int fd, int events);

	int32_t set_non_block(int32_t fd);

	void run_epoll();
#endif // WIN32

	void Run();
};






