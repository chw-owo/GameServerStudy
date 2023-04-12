#include "FileIO.h"

// File IO
void OpenFile(const TCHAR* filename, FILE** file, const TCHAR* mode)
{
	errno_t ret = _tfopen_s(&(*file), filename, mode);
	if (ret != 0)
		_tprintf(_T("Fail to open %s : %d\n"), filename, ret);
}

void CloseFile(FILE** file)
{
	errno_t ret = fclose(*file);
	if (ret != 0)
		_tprintf(_T("Fail to close file : %d\n"), ret);
}

void ReadFile(FILE** file, const DWORD& size, char* data)
{
	fread(data, size, 1, *file);
	errno_t ret = ferror(*file);

	if (ret != 0)
		_tprintf(_T("Fail to read data : %d\n"), ret);
}

void WriteFile(FILE** file, const DWORD& size, char* data)
{
	fwrite(data, size, 1, *file);
	errno_t ret = ferror(*file);

	if (ret != 0)
		_tprintf(_T("Fail to write data : %d\n"), ret);
}

DWORD GetFileSize(FILE** file)
{
	fseek(*file, 0, SEEK_END);
	LONG size = ftell(*file);
	fseek(*file, 0, SEEK_SET);

	if (size == -1)
		_tprintf(_T("Fail to get file size\n"));

	return size;
}
