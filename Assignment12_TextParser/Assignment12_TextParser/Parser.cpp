#include "Parser.h"
#include "FileIO.h"
#include <tchar.h>
#include <Windows.h>

void txtParser::LoadFile(const TCHAR* filename)
{
	FILE* file;
	OpenFile(filename, &file, _T("r"));
	DWORD size = GetFileSize(&file);
	ReadFile(&file, size, data);
	CloseFile(&file);
}

void csvParser::LoadFile(const TCHAR* filename)
{
	FILE* file;
	OpenFile(filename, &file, _T("r"));
	DWORD size = GetFileSize(&file);
	ReadFile(&file, size, data);
	CloseFile(&file);
}

void csvParser::PrevColumn()
{

}
void csvParser::NextColumn()
{

}

void csvParser::PrevRow()
{

}
void csvParser::NextRow()
{

}