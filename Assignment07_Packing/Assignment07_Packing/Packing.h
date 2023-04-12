#pragma once
#include "FileIO.h"

#define MENU_INPUT 4
#define FILE_CNT 32
#define FILE_NAME 32
#define INPUT 512
#define TYPE 0x99886655
typedef struct _PACKINFOHEADER
{
	DWORD64 type = 0x99886655;
	DWORD fileNum = 0;

}PACKINFOHEADER;

typedef struct _FILEINFOHEADER
{
	DWORD fileSize[FILE_CNT] = { 0, };
	DWORD offset[FILE_CNT] = { 0, };
	TCHAR fileName[FILE_CNT][FILE_NAME];

}FILEINFOHEADER;

typedef struct _PACKING
{
	PACKINFOHEADER pi;
	FILEINFOHEADER fi;
	char* data;

}PACKING;

void Pack(TCHAR* packName, TCHAR* inputFilesName);
void Unpack(TCHAR* packName);
void ShowFiles(TCHAR* packName);
void AddPack(TCHAR* packName, TCHAR* inputFilesName);
void UnpackFiles(TCHAR* packName, TCHAR* inputFilesName);
void UpdateFile(TCHAR* packName, TCHAR* fileName);

//For Common ================================
void RemoveChar(TCHAR* fileName);
void SetPack(PACKING& pack);

template <typename T>
void ChangeDataToChar(char* output, const DWORD& idx, const T& input);
template <typename T>
void ChangeDataArrToChar(char* output, const T& input, const DWORD& idx, const DWORD& cnt);
template <typename T>
void ChangeCharToData(const char* input, const DWORD& idx, T& output);
template <typename T>
void ChangeCharToDataArr(const char* input, T& output, const DWORD& idx, const DWORD& cnt);

