#include <stdio.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <csignal>

#include "UT_Sigtion.h"
#include "UT_Pb_Center.h"
#include "LC_Net_System.h"
#include "LC_SystemBase.h"
#include "LC_MsgHandler.h"
#include "LC_Exit.h"

static void quit_handler(int32_t iSigVal)
{
	CExit::instance().exit_flag = 1;
	signal(SIGINT, quit_handler);
}

int main()
{
	printf("server start ....\n");

	CMsgHandler::instance();
	CExit::instance();
	if (!(CNetWork::instance().Init() && CMain::instance().Init()))
	{
		printf("something error\n");
		return -1;
	}


#ifndef WIN32
	signal(SIGINT, quit_handler);
#endif // !WIN32

	std::cout << R"(choose type:1.server 2.client )";
	uint32_t uiType;
	std::cin >> uiType;

	switch (uiType)
	{
	case 1:
	{
		std::cout << "put in port:";
		uint32_t uiPort = 9999;
		//std::cin >> uiPort;
		CNetWork::instance().startListen(uiPort);
		break;
	}
	case 2:
	{
		std::string strip = "127.0.0.1";
		uint32_t uiPort = 9999;
		//std::cout << "put in ip:";
		//std::cin >> strip;
		//std::cout << "put in port:";
		//std::cin >> uiPort;

		for (size_t i = 0; i < 9999; ++i)
		{
			if (!CNetWork::instance().handlerConnect(strip, uiPort))
			{
				return -1;
			}
		}

		break;
	}
	default:
		return -1;
	}

	std::thread t([] {
		while (CExit::instance().check_exit())
		{
			CMain::instance().Update();
		}
		});

	CNetWork::instance().Run();

	t.join();

	printf("\nserver exit...\n");

	return 0;
}