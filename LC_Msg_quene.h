#pragma once
#include <iostream>
#include "LC_Net_System.h"


constexpr auto Msg_Quene_Size = 0xA000000;
//constexpr auto Message_Quene_Size = 0x10000;

struct stMsgHead
{
	uint32_t m_uiFd = 0;
	uint32_t m_uiMessageid = 0;
	uint32_t m_uiMessageSize = 0;
	bool	 m_bBordcast = false;
};


class CMsgQuene
{
public:
	char* m_pStart = nullptr;
	char* m_pRead = nullptr;
	char* m_pWrite = nullptr;

	CMsgQuene();
	~CMsgQuene();

	void push(char* pBuf, size_t iLen);

	void pop(char* pBuf, size_t& iLen);
};


#define MSG_HEAD_SIZE (sizeof(stMsgHead))