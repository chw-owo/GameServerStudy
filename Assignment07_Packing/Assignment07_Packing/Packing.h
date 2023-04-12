#pragma once
#include <iostream>
#include <windows.h>

#define FILE_CNT 32
#define FILE_NAME 32

typedef struct _PACKINFOHEADER
{
	DWORD64 type = 0x99886655;
	int fileNum = 0;

}PACKINFOHEADER;

typedef struct _FILEINFOHEADER
{
	int fileSize[FILE_CNT] = { 0, };
	int offset[FILE_CNT] = { 0, };
	char fileName[FILE_CNT][FILE_NAME];

}FILEINFOHEADER;

typedef struct _PACKING
{
	PACKINFOHEADER pi;
	FILEINFOHEADER fi;
	char* data;

}PACKING;

void Pack(char* packName, char* inputFilesName);
void Unpack(char* packName);
void ShowFiles(char* packName);
void AddPack(char* packName, char* inputFilesName);
void UnpackFiles(char* packName, char* inputFilesName);
void UpdateFile(char* packName, char* fileName);

//For Common ================================
void RemoveChar(char* fileName);
void SetPack(PACKING& pack);
int GetFileSize(FILE** file);

template <typename T>
void ChangeCharToData(const char* input, int idx, T output);
template <typename T>
void ChangeCharToDataArr(const char* input, T output, int idx, int cnt);
template <typename T>
void ChangeDataToChar(char* output, int idx, T input);
template <typename T>
void ChangeDataArrToChar(char* output, T input, int idx, int cnt);


