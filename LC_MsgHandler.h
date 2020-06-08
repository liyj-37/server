#pragma once
#include "UT_Sigtion.h"
#include "LC_Msg_quene.h"
#include "LC_Net_System.h"
#include "LC_SystemBase.h"
#include "UT_Pb_Center.h"
#include "LC_Dispatcher.h"
#include "UT_Msg_Type.h"

constexpr static auto max_pop_times = 500;

class CMsgHandler :public Sigtion<CMsgHandler>,
	public CBase
{
public:
	CMsgQuene m_kQueneInput;
	CMsgQuene m_kQueneOutPut;

	dispatcher<uint32_t, void(char*, int)> dp;

	char m_cInputBuffer[message_buff_size] = {};
	char m_cOutputBuffer[message_buff_size] = {};

	virtual bool Init() override;

	virtual void Update()override;

	void Test1111(char* pBuf, int iLen)
	{

	}
};

#define REG_GLOBAL_FUNC(id,fc) CMsgHandler::instance().dp.register_handler(id,fc);
#define UNREG_GLOBAL_FUNC(id,fc) CMsgHandler::instance().dp.unregister_handler(id,fc);
#define REG_CLASS_FUNC(id,fc) CMsgHandler::instance().dp.register_handler(id,*this,&fc);
#define UNREG_CLASS_FUNC(id,fc) CMsgHandler::instance().dp.unregister_handler(id,*this,&fc);

#define OUT_PUSH(_BUF,_LEN) CMsgHandler::instance().m_kQueneOutPut.push(_BUF,_LEN);
#define OUT_POP(_BUF,_LEN) CMsgHandler::instance().m_kQueneOutPut.pop(_BUF,_LEN);
#define IN_PUSH(_BUF,_LEN) CMsgHandler::instance().m_kQueneInput.push(_BUF,_LEN);
#define IN_POP(_BUF,_LEN) CMsgHandler::instance().m_kQueneInput.pop(_BUF,_LEN);
