#pragma once

template<class T>
class Sigtion
{
public:
	static T& instance(void)
	{
		static T t;
		return t;
	}
};


