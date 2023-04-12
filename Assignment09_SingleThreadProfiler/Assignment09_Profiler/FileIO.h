#pragma once
#include <windows.h>
#include <stdio.h>

void OpenFile(const WCHAR* filename, FILE** pFile, const WCHAR* mode);
void ReadFile(FILE** pFile, const int& size, char* data);
void WriteFile(FILE** pFile, const int& size, char* data);
void CloseFile(FILE** pFile);
int GetFileSize(FILE** pFile);
