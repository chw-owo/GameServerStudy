#pragma once
#include <iostream>
#include <TCHAR.h>
#include <windows.h>

//File IO
void OpenFile(const TCHAR* filename, FILE** file, const TCHAR* mode);
void ReadFile(FILE** file, const DWORD& size, char* data);
void WriteFile(FILE** file, const DWORD& size, char* data);
void CloseFile(FILE** file);
DWORD GetFileSize(FILE** file);