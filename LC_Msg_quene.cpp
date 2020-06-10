#include "LC_Msg_quene.h"
#include <assert.h>

CMsgQuene::CMsgQuene()
{
	m_pStart = (char*)malloc(msg_quene_size);

	assert(m_pStart);

	m_pWrite = m_pRead = m_pStart;

	memset(m_pStart, 0, msg_quene_size);
}

CMsgQuene::~CMsgQuene()
{
	if (m_pStart)
	{
		free(m_pStart);
		m_pStart = nullptr;
	}
}

void CMsgQuene::push(char* pBuf, size_t iLen)
{
	if (!pBuf || iLen < MSG_HEAD_SIZE)
		return;

	if (m_pWrite + iLen < m_pStart + msg_quene_size)
	{
		memcpy(m_pWrite, pBuf, iLen);
		m_pWrite += iLen;
	}
	else
	{
		auto iSize = m_pStart + msg_quene_size - m_pWrite;
		memcpy(m_pWrite, pBuf, iSize);
		memcpy(m_pStart, pBuf + iSize, iLen - iSize);
		m_pWrite = m_pStart + iLen - iSize;
	}
}

void CMsgQuene::pop(char* pBuf, size_t& iLen)
{
	if (pBuf == nullptr) return;

	static stMsgHead head;

	auto p = reinterpret_cast<char*>(&head);

	for (size_t i = 0; i < MSG_HEAD_SIZE; ++i)
	{
		if (m_pRead + i - m_pStart < msg_quene_size)
		{
			p[i] = *(m_pRead + i);
		}
		else
		{
			p[i] = *(m_pStart +
				(m_pRead + i - m_pStart) % msg_quene_size);
		}
	}

	if (head.m_uiMessageSize < MSG_HEAD_SIZE)
	{
		return;
	}
	iLen = head.m_uiMessageSize;

	if (m_pRead + iLen < m_pStart + msg_quene_size)
	{
		memcpy(pBuf, m_pRead, iLen);
		memset(m_pRead, 0, iLen);
		m_pRead += iLen;
	}
	else
	{
		auto iSize = m_pStart + msg_quene_size - m_pRead;
		memcpy(pBuf, m_pRead, iSize);
		memset(m_pRead, 0, iSize);
		memcpy(pBuf + iSize, m_pStart, iLen - iSize);
		memset(m_pStart, 0, iLen - iSize);
		m_pRead = m_pStart + iLen - iSize;
	}
}
