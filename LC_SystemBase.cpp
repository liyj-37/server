#include "LC_SystemBase.h"
#include "UT_Pb_Center.h"
#include "LC_Net_System.h"
#include "LC_MsgHandler.h"

bool CMain::Init()
{
	for (auto pThis : Sbase)
	{
		if (!pThis->Init())
		{
			return false;
		}
	}
	return true;
}

void CMain::Update()
{
	for (auto pThis : Sbase)
	{
		pThis->Update();
	}
}

CBase::CBase()
{
	for (auto pThis : CMain::instance().Sbase)
	{
		if (pThis == this)
			return;
	}
	CMain::instance().Sbase.push_back(this);
}

CBase::~CBase()
{
	CMain::instance().Sbase.clear();
}
