#include "LC_Net_System.h"
#include "LC_MsgHandler.h"
#include "LC_Msg_quene.h"
#include "UT_Pb_Center.h"
#include "LC_Exit.h"
#include <errno.h>

CSocketGroup::~CSocketGroup()
{
	for (const auto [_, pInfo] : m_mSocket)
	{
		if (pInfo != nullptr)
		{
			delete pInfo;
		}
	}
	m_mSocket.clear();
}

void CSocketGroup::TimeCheck()
{
	uint32_t uiTimeNow = time(NULL);

	static auto uiLastCheckTime = uiTimeNow;

	if (uiTimeNow - uiLastCheckTime < 2)
	{
		return;
	}

	uiLastCheckTime = uiTimeNow;

	std::cout << "socket size:" << m_mSocket.size() << std::endl;

	for (auto iter = m_mSocket.begin();
		iter != m_mSocket.end();)
	{
		if (iter->second->m_uiSocketType == SKT_CONNECT &&
			uiTimeNow >= iter->second->m_uiLastActiveTime + socket_expire_time)
		{
			iter->second->m_bCloseFlag = true;
		}

		if (iter->second->m_bCloseFlag)
		{
			printf("close socket (ip:%s port:%d)\n", iter->second->m_cIp, iter->second->m_uiPort);
#ifdef WIN32
			closesocket(iter->first);
#else
			CNetWork::instance().epoll_change_event(EPOLL_CTL_DEL, iter->first, 0);
			close(iter->first);
#endif // WIN32
			delete iter->second;
			m_mSocket.erase(iter++);
			continue;
		}
		++iter;
	}
}

bool CNetWork::Init()
{
#ifdef WIN32
	WSADATA     wsaData;
	WSAStartup(0x0202, &wsaData);
#else
	epoll_fd = epoll_create(default_socket_num);
	if (epoll_fd <= 0)
	{
		return false;
	}
#endif // WIN32
	printf("Game Server Init Success!\n");

	return true;
}

bool CNetWork::startListen(uint32_t uiPort)
{
	if (uiPort == 0)
	{
		printf("error listen args\n");
		return false;
	}

	auto pInfo = new stSocketInfo;

	pInfo->m_iSocketfd = socket(AF_INET, SOCK_STREAM, 0);

	if (pInfo->m_iSocketfd < 0)
	{
		printf("create socket error\n");
		delete pInfo;
		return false;
	}

	int32_t flag = 1;
	struct linger ling = { 0, 0 };
	setsockopt(pInfo->m_iSocketfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, (int32_t)sizeof(flag));
	setsockopt(pInfo->m_iSocketfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&flag, (int32_t)sizeof(flag));
	setsockopt(pInfo->m_iSocketfd, SOL_SOCKET, SO_LINGER, (const char*)&ling, sizeof(ling));

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;

#ifdef WIN32
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#else
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif // WIN32

	server_addr.sin_port = htons((u_short)uiPort);

	if (-1 == bind(pInfo->m_iSocketfd, (const sockaddr*)&server_addr, sizeof(server_addr)))
	{
		printf("func bind error\n");
		delete pInfo;
		return false;
	}

	int option_value = 0xFFFF;
	int option_length = sizeof(option_value);

	if (0 != setsockopt(pInfo->m_iSocketfd, SOL_SOCKET, SO_SNDBUF, (const char*)&option_value, option_length))
	{
		delete pInfo;
		return false;
	}
	if (0 != setsockopt(pInfo->m_iSocketfd, SOL_SOCKET, SO_RCVBUF, (const char*)&option_value, option_length))
	{
		delete pInfo;
		return false;
	}
	if (listen(pInfo->m_iSocketfd, 30) != 0)
	{
		printf("func listen error\n");
		delete pInfo;
		return false;
	}

#ifdef WIN32
	int iMode = 1;
	ioctlsocket(pInfo->m_iSocketfd, FIONBIO, reinterpret_cast<u_long*>(&iMode));
#else
	if (1 != set_non_block(pInfo->m_iSocketfd))
	{
		printf("set noblock false");
		delete pInfo;
		return false;
	}
#endif // WIN32

	printf("init listen socket success port: %d\n", uiPort);

	pInfo->m_uiSocketType = SKT_LISTEN;
	pInfo->m_uiPort = uiPort;

	CSocketGroup::instance().m_mSocket.emplace(pInfo->m_iSocketfd, pInfo);

#ifndef WIN32
	epoll_change_event(EPOLL_CTL_ADD, pInfo->m_iSocketfd, EPOLLIN);
#endif // !WIN32
	return true;
	}

void CNetWork::handleAccept(uint32_t uiFd)
{
	auto iter = CSocketGroup::instance().m_mSocket.find(uiFd);
	if (iter == CSocketGroup::instance().m_mSocket.end())
	{
		return;
	}

	auto pInfo = new stSocketInfo;

	struct sockaddr_in client_address;
	int32_t client_addrLength = sizeof(client_address);
#ifdef WIN32
	auto connfd = accept(iter->first, (struct sockaddr*) & client_address, &client_addrLength);
#else
	auto connfd = accept(iter->first, (struct sockaddr*) & client_address, (socklen_t*)&client_addrLength);
#endif // WIN32

	if (connfd == -1)
	{
		printf("accept failed:errno:%d,%s\n", errno, strerror);
		delete pInfo;
		return;
	}
	pInfo->m_iSocketfd = connfd;

	pInfo->m_uiSocketType = SKT_CONNECT;
	pInfo->m_uiLastActiveTime = (uint32_t)time(NULL);
	pInfo->m_uiPort = ntohs(client_address.sin_port);
	auto p = inet_ntoa(client_address.sin_addr);
	memcpy(pInfo->m_cIp, p, (strlen(p) > 30 ? 30 : strlen(p)));

	printf("client connect success ip:%s port:%d\n", pInfo->m_cIp, pInfo->m_uiPort);

	CSocketGroup::instance().m_mSocket.emplace(pInfo->m_iSocketfd, pInfo);

#ifndef WIN32
	epoll_change_event(EPOLL_CTL_ADD, connfd, EPOLLIN);
#endif // !WIN32
}

bool CNetWork::handlerConnect(const std::string& strIp, int32_t uiPort)
{
	if ("" == strIp || 0 == uiPort)
		return false;

	unsigned long ip;
	if ((ip = inet_addr(strIp.c_str())) == -1) {
		printf("illegal ip£º%s\n", strIp.c_str());
		return false;
	}

	auto pInfo = new stSocketInfo;

	printf("Connecting to %s:%d......\n", strIp.c_str(), uiPort);

	pInfo->m_iSocketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (pInfo->m_iSocketfd == -1) {
		printf("create socket error\n");
		delete pInfo;
		return false;
	}

	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(sockaddr_in));
	serverAddress.sin_family = AF_INET;
#ifdef WIN32
	serverAddress.sin_addr.S_un.S_addr = ip;
#else
	serverAddress.sin_addr.s_addr = ip;
#endif // WIN32
	serverAddress.sin_port = htons(static_cast<u_short>(uiPort));

	if (-1 == connect(pInfo->m_iSocketfd, (sockaddr*)&serverAddress, sizeof(serverAddress)))
	{
		printf("connect to %s:%d failed\n", strIp.c_str(), uiPort);
		delete pInfo;
		return false;
	}

	printf("connect to %s:%d success\n", strIp.c_str(), uiPort);

#ifdef WIN32
	int iMode = 1;
	ioctlsocket(pInfo->m_iSocketfd, FIONBIO, reinterpret_cast<u_long*>(&iMode));
#else
	if (1 != set_non_block(pInfo->m_iSocketfd))
	{
		printf("set noblock false");
		delete pInfo;
		return false;
	}
#endif // WIN32

	pInfo->m_uiPort = (uint16_t)uiPort;
	pInfo->m_uiSocketType = SKT_CONNECT;
	pInfo->m_uiLastActiveTime = (uint32_t)time(NULL);
	memcpy(pInfo->m_cIp, strIp.c_str(), strIp.size() > 30 ? 30 : strIp.size());

	CSocketGroup::instance().m_mSocket.emplace(pInfo->m_iSocketfd, pInfo);

	return true;
}

int32_t CNetWork::handleRecv(uint32_t uiFd)
{
	auto iter = CSocketGroup::instance().m_mSocket.find(uiFd);
	if (iter == CSocketGroup::instance().m_mSocket.end())
	{
		return 0;
	}

	auto iRecvByte = recv(iter->first, reinterpret_cast<char*>(iter->second->m_MessageBuff + iter->second->m_uiRecvSize),
		message_buff_size - iter->second->m_uiRecvSize, 0);
	if (0 == iRecvByte)
	{
		iter->second->m_bCloseFlag = true;
		return 0;
	}
	if (iRecvByte < 0)
		return 0;

	iter->second->m_uiRecvSize += iRecvByte;
	iter->second->m_uiLastActiveTime = (uint32_t)time(NULL);

	while (1)
	{
		if (iter->second->m_uiRecvSize < MSG_HEAD_SIZE)
		{
			break;
		}

		auto p = reinterpret_cast<stMsgHead*>(iter->second->m_MessageBuff);
		if (p->m_uiMessageSize > message_buff_size)
		{
			iter->second->m_bCloseFlag = true;
			break;
		}
		if (iter->second->m_uiRecvSize < p->m_uiMessageSize)
		{
			break;
		}
		p->m_uiFd = iter->first;

		IN_PUSH((char*)iter->second->m_MessageBuff, p->m_uiMessageSize);
		iter->second->m_uiRecvSize -= p->m_uiMessageSize;
		memmove(iter->second->m_MessageBuff, iter->second->m_MessageBuff + p->m_uiMessageSize, message_buff_size - p->m_uiMessageSize);
	}
	return iRecvByte;
}

int32_t CNetWork::handleSend()
{
	uint32_t uiSendByte = 0;
	auto p = CMsgHandler::instance().m_cOutputBuffer;

	for (size_t i = 0; i < max_pop_times; ++i)
	{
		size_t iLen = 0;
		OUT_POP(p, iLen);
		if (iLen < MSG_HEAD_SIZE)
		{
			break;
		}

		auto pHead = reinterpret_cast<stMsgHead*>(p);
		if (!pHead->m_bBordcast || (pHead->m_uiFd != 0 && CSocketGroup::instance().m_mSocket.find(pHead->m_uiFd) !=
			CSocketGroup::instance().m_mSocket.end()))
		{
			continue;
		}
		if (pHead->m_bBordcast)
		{
			for (const auto& [_, pInfo] : CSocketGroup::instance().m_mSocket)
			{
				if (pInfo->m_uiSocketType == SKT_LISTEN ||
					pInfo->m_bCloseFlag)
				{
					continue;
				}

				if (iLen != send(pInfo->m_iSocketfd, p, iLen, 0))
				{
					pInfo->m_bCloseFlag = true;
				}
				else
				{
					uiSendByte += iLen;
					pInfo->m_uiLastActiveTime = time(NULL);
				}
			}
		}
		else
		{
			auto iter = CSocketGroup::instance().m_mSocket.find(pHead->m_uiFd);
			if (iter != CSocketGroup::instance().m_mSocket.end())
			{
				if (iLen != send(pHead->m_uiFd, p, iLen, 0))
				{
					iter->second->m_bCloseFlag = true;
				}
				else
				{
					uiSendByte += iLen;
					iter->second->m_uiLastActiveTime = time(NULL);
				}
			}
		}
	}
	return uiSendByte;
}

#ifdef WIN32
void CNetWork::run_select()
{
	fd_set  fdRead;
	struct timeval tv = { 0, 0 };

	FD_ZERO(&fdRead);

	for (const auto& [_, pInfo] : CSocketGroup::instance().m_mSocket)
	{
		if (pInfo->m_uiSocketType != SKT_DEFAULT && !pInfo->m_bCloseFlag)
		{
			FD_SET(pInfo->m_iSocketfd, &fdRead);
		}
	}

	auto ret = select(0, &fdRead, NULL, NULL, &tv);

	if (ret <= 0)
	{
		return;
	}
	else
	{
		for (const auto& [iSocketid, pInfo] : CSocketGroup::instance().m_mSocket)
		{
			if (FD_ISSET(iSocketid, &fdRead))
			{
				switch (pInfo->m_uiSocketType)
				{
				case SKT_LISTEN:
					handleAccept(iSocketid);
					break;
				case SKT_CONNECT:
					handleRecv(iSocketid);
					break;
				default:
					break;
				}
			}
		}
	}
}

#else
void CNetWork::epoll_change_event(int op, int fd, int events)
{
	if (fd <= 0)
	{
		return;
	}
	struct epoll_event eEvent;

	eEvent.events |= events;
	eEvent.data.fd = fd;

	epoll_ctl(epoll_fd, op, fd, &eEvent);
}

int32_t CNetWork::set_non_block(int32_t fd)
{
	int32_t flag = 0;
	flag = fcntl(fd, F_GETFL, 0);
	if (flag < 0)
	{
		return 0;
	}

	if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0)
	{
		return 0;
	}

	return 1;
}

void CNetWork::run_epoll()
{
	struct epoll_event events[default_socket_num];

	auto ret = epoll_wait(epoll_fd, events, default_socket_num, 0);

	for (int32_t i = 0; i < ret; ++i)
	{
		if (events[i].events & EPOLLIN)
		{
			auto iter = CSocketGroup::instance().m_mSocket.find(events[i].data.fd);
			if (iter != CSocketGroup::instance().m_mSocket.end())
			{
				switch (iter->second->m_uiSocketType)
				{
				case SKT_LISTEN:
					handleAccept(iter->first);
					break;
				case SKT_CONNECT:
					handleRecv(iter->first);
					break;
				default:
					break;
			}
		}
	}
}
}
#endif // WIN32

void CNetWork::Run()
{
	while (CExit::instance().check_exit())
	{
#ifdef WIN32
		run_select();
#else
		run_epoll();
#endif // 
		handleSend();
		CSocketGroup::instance().TimeCheck();
	}
}