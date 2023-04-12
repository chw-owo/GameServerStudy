#include "fileIO.h"

// file IO
void Openfile(const char* filename, file** file, const char* mode)
{
	errno_t ret = _tfopen_s(&(*file), filename, mode);
	if (ret != 0)
		_tprintf("Fail to open %s : %d\n", filename, ret);
}

void Closefile(file** file)
{
	errno_t ret = fclose(*file);
	if (ret != 0)
		_tprintf("Fail to close file : %d\n", ret);
}

void Readfile(file** file, const DWORD& size, char* data)
{
	fread(data, size, 1, *file);
	errno_t ret = ferror(*file);

	if (ret != 0)
		_tprintf("Fail to read data : %d\n", ret);
}

void Writefile(file** file, const DWORD& size, char* data)
{
	fwrite(data, size, 1, *file);
	errno_t ret = ferror(*file);

	if (ret != 0)
		_tprintf("Fail to write data : %d\n", ret);
}

DWORD GetfileSize(file** file)
{
	fseek(*file, 0, SEEK_END);
	LONG size = ftell(*file);
	fseek(*file, 0, SEEK_SET);

	if (size == -1)
		_tprintf("Fail to get file size\n");

	return size;
}
