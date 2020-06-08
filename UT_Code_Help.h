#pragma once
//code help
template <class T, size_t N>
static size_t arraySize(const T(&)[N])
{
	return N;
}