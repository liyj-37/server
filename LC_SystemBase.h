#pragma once
#include <list>
#include "UT_Sigtion.h"

class CBase;
class CMain;

class CBase
{
public:
	CBase();
	~CBase();
	virtual bool Init() { return true; }
	virtual void Update() { return; }
};


class CMain :public Sigtion<CMain>
{
public:
	std::list<CBase*> Sbase;

	bool  Init();
	void Update();
};
