#include "FileIO.h"
#include "Profile.h"

// File IO
void OpenFile(const WCHAR* filename, FILE** pFile, const WCHAR* mode)
{
	PRO_BEGIN(L"Open File");
	errno_t ret = _wfopen_s(pFile, filename, mode);
	if (ret != 0)
	{
		wprintf(L"Fail to open %s: %d\n", filename, ret);
	}
	PRO_END(L"Open File");
}

void CloseFile(FILE** pFile)
{
	PRO_BEGIN(L"Close File");
	errno_t ret = fclose(*pFile);
	if (ret != 0)
	{
		wprintf(L"Fail to close: %d\n", ret);
	}
	PRO_END(L"Close File");
}

void ReadFile(FILE** pFile, const int& size, char* data)
{
	fread(data, 1, size, *pFile);
	errno_t ret = ferror(*pFile);
	if (ret != 0)
	{
		wprintf(L"Fail to read data : %d\n", ret);
	}

}

void WriteFile(FILE** pFile, const int& size, char* data)
{
	fwrite(data, size, 1, *pFile);
	errno_t ret = ferror(*pFile);

	if (ret != 0)
	{
		wprintf(L"Fail to write data : %d\n", ret);
	}

}

int GetFileSize(FILE** pFile)
{
	fseek(*pFile, 0, SEEK_END);
	int size = ftell(*pFile);
	rewind(*pFile);

	if (size == -1)
	{
		wprintf(L"Fail to get file size\n");
	}

	return size;
}

