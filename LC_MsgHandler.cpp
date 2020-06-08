#include "LC_MsgHandler.h"

static void Test(char* pBuf, int iLen)
{
	static uint32_t c = 0;
	auto [rGameEvent, bResult] = MsgCenter::ParseFromArray<testprotocol::Test>(pBuf + MSG_HEAD_SIZE, iLen - MSG_HEAD_SIZE);
	if (!bResult)
	{
		return;
	}

	if (rGameEvent.strparam_size() > 0)
	{
		std::cout << rGameEvent.strparam(0) << " " << c++ << std::endl;
	}
}

bool CMsgHandler::Init()
{
	REG_GLOBAL_FUNC(CS_TEST, Test);
	//REG_CLASS_FUNC(0, CMsgHandler::Test1111);

	return true;
}

void CMsgHandler::Update()
{
	for (size_t i = 0; i < max_pop_times; ++i)
	{
		size_t iLen = 0;
		m_kQueneInput.pop(m_cInputBuffer, iLen);
		if (iLen < MSG_HEAD_SIZE)
		{
			break;
		}

		dp[reinterpret_cast<stMsgHead*>(m_cInputBuffer)->m_uiMessageid](m_cInputBuffer, iLen);
	}


	//static uint32_t uiLastSendTime;
	//auto uiTimeNow = time(NULL);
	//if (uiTimeNow - uiLastSendTime >= 1)
	//{
	//	uiLastSendTime = uiTimeNow;
	//	char b[message_buff_size] = {};

	//	auto p = reinterpret_cast<stMsgHead*>(b);

	//	p->m_uiMessageid = CS_TEST;
	//	p->m_bBordcast = true;

	//	testprotocol::Test rGameevent;

	//	rGameevent.add_strparam("---------------------------hello,world------------------------------------");

	//	rGameevent.SerializePartialToArray((void*)(b + MSG_HEAD_SIZE), message_buff_size - MSG_HEAD_SIZE);
	//	p->m_uiMessageSize = MSG_HEAD_SIZE + rGameevent.ByteSize();

	//	m_kQueneOutPut.push(b, p->m_uiMessageSize);
	//}

}
