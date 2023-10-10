#pragma once
class TestData
{
public: 
	TestData() : _idx(-1), _value(-1) {}
	TestData(int idx, int value) : _idx(idx), _value(value) {}
	void SetData(int idx, int value) { _idx = idx; _value = value; }

	int _idx;
	int _value;
};