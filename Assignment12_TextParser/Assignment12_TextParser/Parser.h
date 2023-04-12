#pragma once
#include <tchar.h>
#include <Windows.h>
#include "FileIO.h"

class txtParser
{
public:
	void LoadFile(const TCHAR* filename);

	template <typename T>
	void GetValue(const TCHAR* key, T& value);

private:
	char* data;
};

template <typename T>
void txtParser::GetValue(const TCHAR* key, T& value)
{

}


class csvParser
{
public:
	void LoadFile(const TCHAR* filename);

	template <typename T>
	void GetColumn(T& value);
	void PrevColumn();
	void NextColumn();

	template <typename T>
	void GetRow(T& value);
	void PrevRow();
	void NextRow();

private:
	char* data;
	DWORD col = 0;
	DWORD row = 0;
};

template <typename T>
void csvParser::GetColumn(T& value)
{

}

template <typename T>
void csvParser::GetRow(T& value)
{

}