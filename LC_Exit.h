#pragma once

class CExit :public Sigtion<CExit>
{
public:
	enum:uint8_t
	{
		ET_RUN,
		ET_EXIT,
	};

	uint8_t exit_flag = ET_RUN;

	bool check_exit()
	{
		if (exit_flag == ET_RUN)
			return true;
		return false;
	}
};