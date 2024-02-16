#pragma once

#define dfTHREAD_CNT 10
#define dfTEST_TOTAL_CNT 2500000

template<typename T>
class CBasicStack
{
public:
	void push(T value)
	{
		_stack[_count] = value;
		_count++;
	}

	T pop()
	{
		if (_count <= 0) return 0;
		_count--;
		return _stack[_count];
	}

private:
	T _stack[dfTHREAD_CNT * dfTEST_TOTAL_CNT] = { 0, };
	int _count = 0;

};