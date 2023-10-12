#pragma once
class CPoolTestData
{
public:
	CPoolTestData() : _idx(-1), _value(-1) {}
	CPoolTestData(int idx, int value) : _idx(idx), _value(value) {}
	void SetData(int idx, int value) { _idx = idx; _value = value; }

	int _idx;
	int _value;
};