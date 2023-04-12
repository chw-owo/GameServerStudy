#include "Packing.h"
#include "FileIO.h"

void Pack(TCHAR* packName, TCHAR* inputFilesName)
{
	PACKING pack;
	RemoveChar(packName);
	SetPack(pack);

	// Add Files to Pack =========================================
	TCHAR seps[] = _T(" \t\n");
	TCHAR* next = nullptr;
	TCHAR* fileName;

	fileName = _tcstok_s(inputFilesName, seps, &next);
	char* data[FILE_CNT];
	DWORD dataSize;

	FILE* file;
	DWORD totalSize = 0;

	while (fileName != NULL)
	{
		RemoveChar(fileName);
		OpenFile(fileName, &file, _T("rb"));
		dataSize = GetFileSize(&file);

		data[pack.pi.fileNum] = static_cast<char*>(malloc(dataSize));
		memset(data[pack.pi.fileNum], _T('\0'), dataSize);

		ReadFile(&file, dataSize, data[pack.pi.fileNum]);
		totalSize += dataSize;
		if (pack.pi.fileNum == 0)
		{
			pack.fi.offset[pack.pi.fileNum] = 0;
		}
		else
		{
			pack.fi.offset[pack.pi.fileNum]
				= pack.fi.offset[pack.pi.fileNum - 1]
				+ pack.fi.fileSize[pack.pi.fileNum - 1];
		}

		memset(pack.fi.fileName[pack.pi.fileNum], _T('\0'), FILE_NAME);
		_tcscpy_s(pack.fi.fileName[pack.pi.fileNum], FILE_NAME, fileName);
		pack.fi.fileSize[pack.pi.fileNum] = dataSize;
		pack.pi.fileNum++;

		CloseFile(&file);

		fileName = _tcstok_s(NULL, seps, &next);
	}

	pack.data = static_cast<char*>(malloc(totalSize));
	memset(pack.data, _T('\0'), totalSize);

	DWORD offset;
	for (DWORD i = 0; i < pack.pi.fileNum; i++)
	{
		offset = pack.fi.offset[i];
		for (DWORD j = 0; j < pack.fi.fileSize[i]; j++)
		{
			pack.data[offset + j] = *(data[i] + j);
		}

		free(data[i]);
	}

	// Write pack to Packfile

	totalSize =
		sizeof(DWORD64) + sizeof(DWORD)			// PACK INFO HEADER
		+ sizeof(DWORD) * FILE_CNT * 2			// FILE INFO HEADER 1
		+ sizeof(TCHAR) * FILE_CNT * FILE_NAME	// FILE INFO HEADER 2
		+ pack.fi.offset[pack.pi.fileNum - 1]	// DATA
		+ pack.fi.fileSize[pack.pi.fileNum - 1];

	char* totalData = static_cast<char*>(malloc(totalSize));
	memset(totalData, '\0', totalSize);

	DWORD idx;

	idx = 0;
	ChangeDataToChar(totalData, idx, pack.pi.type);
	idx += sizeof(DWORD64);
	ChangeDataToChar(totalData, idx, pack.pi.fileNum);
	idx += sizeof(DWORD);
	ChangeDataArrToChar(totalData, pack.fi.fileSize, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;
	ChangeDataArrToChar(totalData, pack.fi.offset, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;

	DWORD tmpIdx = idx;
	char cTmp[FILE_NAME] = { '\0', };
	for (int i = 0; i < FILE_CNT; i++)
	{
		WideCharToMultiByte(CP_ACP, 0, pack.fi.fileName[i],
			FILE_NAME, cTmp, FILE_NAME, NULL, NULL);

		for (int j = 0; j < FILE_NAME; j++)
		{
			totalData[tmpIdx] = cTmp[j];
			tmpIdx++;
		}
	}
	idx += sizeof(TCHAR) * FILE_CNT * FILE_NAME;

	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		offset = pack.fi.offset[i];
		for (int j = 0; j < pack.fi.fileSize[i]; j++)
		{
			totalData[idx + offset + j] = pack.data[offset + j];
		}
	}
	free(pack.data);

	FILE* packfile;
	OpenFile(packName, &packfile, _T("wb"));
	WriteFile(&packfile, totalSize, totalData);
	CloseFile(&packfile);

	_tprintf(_T("Pack Complete!\n\n"));
}


void Unpack(TCHAR* packName)
{
	PACKING pack;
	RemoveChar(packName);

	FILE* file;
	OpenFile(packName, &file, _T("rb"));

	DWORD size = GetFileSize(&file);
	char* data = static_cast<char*>(malloc(size));
	memset(data, '\0', size);
	ReadFile(&file, size, data);
	CloseFile(&file);

	DWORD idx = 0;
	DWORD64 type = 0;
	ChangeCharToData(data, idx, type);

	if (type != pack.pi.type)
	{
		_tprintf(_T("It's not pack file!: %ux, %ux\n"), type, pack.pi.type);
		return;
	}

	idx += sizeof(DWORD64);
	ChangeCharToData(data, idx, pack.pi.fileNum);
	idx += sizeof(DWORD);
	ChangeCharToDataArr(data, pack.fi.fileSize, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;
	ChangeCharToDataArr(data, pack.fi.offset, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;

	char seps[] = " \t\n";
	char* next = nullptr;
	char* tok;

	char cTmp[FILE_NAME] = { '\0', };
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		for (int j = 0; j < FILE_NAME; j++)
		{
			cTmp[j] = *(data + idx + i * FILE_NAME + j);
		}

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cTmp,
			FILE_NAME, pack.fi.fileName[i], FILE_NAME);
	}

	idx += sizeof(TCHAR) * FILE_CNT * FILE_NAME;

	DWORD dataSize = size - idx;
	pack.data = static_cast<char*>(malloc(dataSize));
	memset(pack.data, '\0', dataSize);

	DWORD offset;
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		offset = pack.fi.offset[i];
		for (int j = 0; j < pack.fi.fileSize[i]; j++)
		{
			pack.data[offset + j] = data[idx + offset + j];
		}
	}

	free(data);
	
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		size = pack.fi.fileSize[i];
		char* data = static_cast<char*>(malloc(size));
		memset(data, '\0', size);

		offset = pack.fi.offset[i];
		for (int j = 0; j < size; j++)
		{
			data[j] = *(pack.data + offset + j);
		}

		FILE* file;
		OpenFile(pack.fi.fileName[i], &file, _T("wb"));
		WriteFile(&file, size, data);
		CloseFile(&file);
		free(data);
	}

	free(pack.data);
	_tprintf(_T("Unpack Complete!\n\n"));
}

void AddPack(TCHAR* packName, TCHAR* inputFilesName)
{
	PACKING pack;
	RemoveChar(packName);
	SetPack(pack);

	// ReadPackFile(TCHAR * packname, PACKING & pack)
	
	FILE* packfile;
	OpenFile(packName, &packfile, _T("rb"));

	DWORD size = GetFileSize(&packfile);
	char* data = static_cast<char*>(malloc(size));
	memset(data, '\0', size);
	ReadFile(&packfile, size, data);
	CloseFile(&packfile);

	DWORD idx = 0;
	DWORD64 type = 0;
	ChangeCharToData(data, idx, type);

	if (type != pack.pi.type)
	{
		_tprintf(_T("It's not pack file!\n"));
		return;
	}

	idx += sizeof(DWORD64);
	ChangeCharToData(data, idx, pack.pi.fileNum);
	idx += sizeof(DWORD);
	ChangeCharToDataArr(data, pack.fi.fileSize, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;
	ChangeCharToDataArr(data, pack.fi.offset, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;

	char seps[] = " \t\n";
	char* next = nullptr;
	char* tok;

	char cTmp[FILE_NAME] = { '\0', };
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		tok = strtok_s(data + idx + i * FILE_NAME, seps, &next);
		strcpy_s(cTmp, FILE_NAME, tok);

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cTmp,
			FILE_NAME, pack.fi.fileName[i], FILE_NAME);
	}

	idx += sizeof(TCHAR) * FILE_CNT * FILE_NAME;

	DWORD dataSize = size - idx;

	
	//AddFilesToPack
	
	TCHAR tSeps[] = _T(" \t\n");
	TCHAR* tNext = nullptr;
	TCHAR* fileName;

	fileName = _tcstok_s(inputFilesName, tSeps, &tNext);
	char* fileData[FILE_CNT];
	DWORD fileSize;

	FILE* file;
	DWORD originFileNum;

	if (pack.pi.fileNum == 0)
		originFileNum = 0;
	else
		originFileNum = pack.pi.fileNum;

	while (fileName != NULL)
	{
		RemoveChar(fileName);
		OpenFile(fileName, &file, _T("rb"));
		fileSize = GetFileSize(&file);

		fileData[pack.pi.fileNum] = static_cast<char*>(malloc(fileSize));
		memset(fileData[pack.pi.fileNum], _T('\0'), fileSize);

		ReadFile(&file, fileSize, fileData[pack.pi.fileNum]);
		dataSize += fileSize;

		if (pack.pi.fileNum == 0)
		{
			pack.fi.offset[pack.pi.fileNum] = 0;
		}
		else
		{
			pack.fi.offset[pack.pi.fileNum]
				= pack.fi.offset[pack.pi.fileNum - 1]
				+ pack.fi.fileSize[pack.pi.fileNum - 1];
		}

		memset(pack.fi.fileName[pack.pi.fileNum], _T('\0'), FILE_NAME);
		_tcscpy_s(pack.fi.fileName[pack.pi.fileNum], FILE_NAME, fileName);
		pack.fi.fileSize[pack.pi.fileNum] = fileSize;
		pack.pi.fileNum++;

		CloseFile(&file);

		fileName = _tcstok_s(NULL, tSeps, &tNext);
	}

	pack.data = static_cast<char*>(malloc(dataSize));
	memset(pack.data, _T('\0'), dataSize);

	DWORD offset;
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		offset = pack.fi.offset[i];
		if (i < originFileNum)
		{
			for (int j = 0; j < pack.fi.fileSize[i]; j++)
			{
				pack.data[offset + j] = data[idx + offset + j];
			}
		}	
		else
		{
			for (int j = 0; j < pack.fi.fileSize[i]; j++)
			{
				pack.data[offset + j] = *(fileData[i] + j);
			}
			free(fileData[i]);
		}
	}
	free(data);

	//WritePackToFile

	// 0. Setting
	DWORD totalSize = dataSize +
			( sizeof(DWORD64) + sizeof(DWORD)			// PACK INFO HEADER
			+ sizeof(DWORD) * FILE_CNT * 2				// FILE INFO HEADER 1
			+ sizeof(TCHAR) * FILE_CNT * FILE_NAME);	// FILE INFO HEADER 2


	char* totalData = static_cast<char*>(malloc(totalSize));
	memset(totalData, '\0', totalSize);

	//2. Get pack.header

	idx = 0;
	ChangeDataToChar(totalData, idx, pack.pi.type);
	idx += sizeof(DWORD64);
	ChangeDataToChar(totalData, idx, pack.pi.fileNum);
	idx += sizeof(DWORD);
	ChangeDataArrToChar(totalData, pack.fi.fileSize, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;
	ChangeDataArrToChar(totalData, pack.fi.offset, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;

	DWORD tmpIdx = idx;
	char cTmp2[FILE_NAME] = { '\0', };
	for (int i = 0; i < FILE_CNT; i++)
	{
		WideCharToMultiByte(CP_ACP, 0, pack.fi.fileName[i],
			FILE_NAME, cTmp2, FILE_NAME, NULL, NULL);

		for (int j = 0; j < FILE_NAME; j++)
		{
			totalData[tmpIdx] = cTmp2[j];
			tmpIdx++;
		}
	}
	idx += sizeof(TCHAR) * FILE_CNT * FILE_NAME;

	//2. Get pack.data
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		offset = pack.fi.offset[i];
		for (int j = 0; j < pack.fi.fileSize[i]; j++)
		{
			totalData[idx + offset + j] = pack.data[offset + j];
		}
	}

	free(pack.data);

	//3. Write Data to File

	OpenFile(packName, &packfile, _T("wb"));
	WriteFile(&packfile, totalSize, totalData);
	CloseFile(&packfile);

	_tprintf(_T("Add Files to Packfile Complete!\n\n"));
}

void ShowFiles(TCHAR* packName)
{
	PACKING pack;
	SetPack(pack);
	RemoveChar(packName);
	
	FILE* file;
	OpenFile(packName, &file, _T("rb"));

	DWORD size = GetFileSize(&file);
	char* data = static_cast<char*>(malloc(size));
	memset(data, '\0', size);
	ReadFile(&file, size, data);
	CloseFile(&file);

	DWORD idx = 0;
	DWORD64 type = 0;
	DWORD fileNum = 0;
	ChangeCharToData(data, idx, type);

	if (type != pack.pi.type)
	{
		_tprintf(_T("It's not pack file!\n"));
		return;
	}
		

	idx += sizeof(DWORD64);
	ChangeCharToData(data, idx, fileNum);
	idx += (sizeof(DWORD) + sizeof(DWORD) * FILE_CNT * 2);

	char seps[] = " \t\n";
	char* next = nullptr;
	char* tok;

	char cTmp[FILE_NAME] = { '\0', };
	TCHAR fileName[FILE_NAME] = { '\0', };
	for (int i = 0; i < fileNum; i++)
	{
		tok = strtok_s(data + idx + i * FILE_NAME, seps, &next);
		strcpy_s(cTmp, FILE_NAME, tok);

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cTmp,
			FILE_NAME, fileName, FILE_NAME);

		_tprintf(_T("%d. %s\n"), i, fileName);
	}

	_tprintf(_T("\n"));
}


void UnpackFiles(TCHAR* packName, TCHAR* inputFilesName)
{
	PACKING pack;
	SetPack(pack);

	FILE* packfile;
	RemoveChar(packName);
	OpenFile(packName, &packfile, _T("rb"));

	DWORD packSize = GetFileSize(&packfile);
	char* packData = static_cast<char*>(malloc(packSize));
	memset(packData, '\0', packSize);
	ReadFile(&packfile, packSize, packData);
	CloseFile(&packfile);

	DWORD idx = 0;
	DWORD64 type = 0;
	ChangeCharToData(packData, idx, type);

	if (type != pack.pi.type)
	{
		_tprintf(_T("It's not pack file!: %x, %x\n"), type, pack.pi.type);
	}
		
	
	idx += sizeof(DWORD64);
	ChangeCharToData(packData, idx, pack.pi.fileNum);
	idx += sizeof(DWORD);
	ChangeCharToDataArr(packData, pack.fi.fileSize, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;
	ChangeCharToDataArr(packData, pack.fi.offset, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;

	char seps[] = " \t\n";
	char* next = nullptr;
	char* tok;

	char cTmp[FILE_NAME] = { '\0', };
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		for (int j = 0; j < FILE_NAME; j++)
		{
			cTmp[j] = *(packData + idx + i * FILE_NAME + j);
		}

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cTmp,
			FILE_NAME, pack.fi.fileName[i], FILE_NAME);
	}

	idx += sizeof(TCHAR) * FILE_CNT * FILE_NAME;

	DWORD dataSize = packSize - idx;

	pack.data = static_cast<char*>(malloc(dataSize));
	memset(pack.data, '\0', dataSize);

	DWORD offset;
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		offset = pack.fi.offset[i];
		for (int j = 0; j < pack.fi.fileSize[i]; j++)
		{
			pack.data[offset + j] = packData[idx + offset + j];
		}
	}

	free(packData);

	DWORD fileSize;
	bool flag = false;

	TCHAR tSeps[] = _T(" \t\n");
	TCHAR* tNext = nullptr;
	TCHAR* fileName;
	fileName = _tcstok_s(inputFilesName, tSeps, &tNext);

	while (fileName != NULL)
	{
		RemoveChar(fileName);
		for (int i = 0; i < pack.pi.fileNum; i++)
		{
			if (_tcscmp(fileName, pack.fi.fileName[i]) == 0)
			{
				fileSize = pack.fi.fileSize[i];
				char* fileData = static_cast<char*>(malloc(fileSize));
				memset(fileData, '\0', fileSize);

				offset = pack.fi.offset[i];
				for (int j = 0; j < fileSize; j++)
				{
					fileData[j] = *(pack.data + offset + j);
				}

				FILE* file;
				OpenFile(pack.fi.fileName[i], &file, _T("wb"));
				WriteFile(&file, fileSize, fileData);
				CloseFile(&file);
				free(fileData);

				_tprintf(_T("%s is unpacked successfully\n"), fileName);
				flag = true;
				break;
			}
		}

		if (flag == false)
		{
			_tprintf(_T("%s does not exist in the pack file\n"), fileName);
		}

		fileName = _tcstok_s(NULL, tSeps, &tNext);
		flag = false;
	}
	free(pack.data);
	_tprintf(_T("Unpack Selected Files Complete!\n\n"));
}

void UpdateFile(TCHAR* packName, TCHAR* fileName)
{
	// Get file data to update
	FILE* file;
	DWORD fileSize;
	
	RemoveChar(fileName);
	OpenFile(fileName, &file, _T("rb"));
	fileSize = GetFileSize(&file);
	char* fileData = static_cast<char*>(malloc(fileSize));
	ReadFile(&file, fileSize, fileData);
	CloseFile(&file);

	// Get packfile to update
	FILE* packFile;
	DWORD packSize;

	RemoveChar(packName);
	OpenFile(packName, &packFile, _T("rb"));
	packSize = GetFileSize(&packFile);
	char* packData = static_cast<char*>(malloc(packSize));
	ReadFile(&packFile, packSize, packData);
	CloseFile(&packFile);

	// Write packfile to pack
	PACKING pack;
	SetPack(pack);

	DWORD idx = 0;
	DWORD64 type = 0;
	ChangeCharToData(packData, idx, type);

	if (type != pack.pi.type)
	{
		_tprintf(_T("It's not pack file!\n"));
		return;
	}
	
	idx += sizeof(DWORD64);
	ChangeCharToData(packData, idx, pack.pi.fileNum);
	idx += sizeof(DWORD);
	ChangeCharToDataArr(packData, pack.fi.fileSize, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;
	ChangeCharToDataArr(packData, pack.fi.offset, idx, FILE_CNT);
	idx += sizeof(DWORD) * FILE_CNT;


	char cTmp[FILE_NAME] = { '\0', };
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		for (int j = 0; j < FILE_NAME; j++)
		{
			cTmp[j] = *(packData + idx + i * FILE_NAME + j);
		}

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cTmp,
			FILE_NAME, pack.fi.fileName[i], FILE_NAME);
	}

	bool flag = false;
	// Update packfile data through pack
	for (int i = 0; i < pack.pi.fileNum; i++)
	{
		if (_tcscmp(fileName, pack.fi.fileName[i]) == 0)
		{
			pack.fi.fileSize[i] = fileSize;
			idx = sizeof(DWORD64) + sizeof(DWORD) 
					+ sizeof(DWORD) * i; 
			ChangeDataToChar(packData, idx, pack.fi.fileSize[i]);

			pack.fi.offset[i]
				= pack.fi.offset[pack.pi.fileNum - 1]
				+ pack.fi.fileSize[pack.pi.fileNum - 1];
			idx = sizeof(DWORD64) + sizeof(DWORD) + sizeof(DWORD) * FILE_CNT
					+ sizeof(DWORD) * i;
			ChangeDataToChar(packData, idx, pack.fi.offset[i]);
			flag = true;
			break;
		}
	}

	if (flag == false)
	{
		_tprintf(_T("%s doesn't exist in the packfile\n"), fileName);
		free(fileData);
		free(packData);
		return;
	}
	// Write pack to packfile
	packSize += fileSize;
	char* newData = static_cast<char*>(malloc(packSize));
	memset(newData, _T('\0'), packSize);
	strcpy_s(newData, packSize, packData);
	strcat_s(newData, packSize, fileData);

	OpenFile(packName, &packFile, _T("wb"));
	WriteFile(&packFile, packSize, newData);
	CloseFile(&packFile);

	free(fileData);
	free(packData);
	free(newData);
	_tprintf(_T("Update Selected File Complete!\n\n"));
}

//=====================================================

void RemoveChar(TCHAR* fileName)
{
	for (; *fileName != '\0'; fileName++)
	{
		if ((_tcscmp(fileName, _T("\\")) == 0) || (_tcscmp(fileName, _T("\"")) == 0) ||
			(_tcscmp(fileName, _T("/")) == 0) || (_tcscmp(fileName, _T(">")) == 0) ||
			(_tcscmp(fileName, _T(":")) == 0) || (_tcscmp(fileName, _T("<")) == 0) ||
			(_tcscmp(fileName, _T("*")) == 0) || (_tcscmp(fileName, _T("?")) == 0))
		{
			_tcscpy_s(fileName, sizeof(fileName + 1), fileName + 1);
			fileName--;
		}
		else if ((_tcscmp(fileName, _T(" ")) == 0) ||
			(_tcscmp(fileName, _T("\t")) == 0))
		{
			_tcscpy_s(fileName, sizeof(_T("_")), _T("_"));
		}
		else if(_tcscmp(fileName, _T("\n")) == 0)
		{
			_tcscpy_s(fileName, sizeof(_T("\0")), _T("\0"));
		}
	}
}

void SetPack(PACKING& pack)
{
	pack.pi.type = 0x99886655;
	pack.pi.fileNum = 0;
}

template <typename T>
void ChangeCharToData(const char* input, const DWORD& idx, T& output)
{
	DWORD tmpIdx = idx;
	DWORD dataSize = sizeof(output);
	DWORD64 flag = 0x000000ff;
	for (DWORD i = 0; i < dataSize; i++)
	{
		output += ((input[tmpIdx] & flag) << (i * 8));
		tmpIdx++;
	}
}

template <typename T>
void ChangeCharToDataArr(const char* input, T& output, const DWORD& idx, const DWORD& cnt)
{
	DWORD tmpIdx = idx;
	DWORD dataSize = sizeof(output[0]);
	DWORD64 flag = 0x000000ff;
	for (DWORD i = 0; i < cnt; i++)
	{
		for (DWORD j = 0; j < dataSize; j++)
		{
			output[i] += ((input[tmpIdx] & flag) << (j * 8));
			tmpIdx++;
		}
	}
}

template <typename T>
void ChangeDataToChar(char* output, const DWORD& idx, const T& input)
{
	DWORD tmpIdx = idx;
	DWORD dataSize = sizeof(input);
	DWORD64 flag = 0x000000ff;

	for (DWORD i = 0; i < dataSize; i++)
	{
		output[tmpIdx] = ((input >> (i * 8)) & flag);
		tmpIdx++;
	}
}

template <typename T>
void ChangeDataArrToChar(char* output, const T& input, const DWORD& idx, const DWORD& cnt)
{
	DWORD tmpIdx = idx;
	DWORD dataSize = sizeof(input[0]);
	DWORD64 flag = 0x000000ff;

	for (DWORD i = 0; i < cnt; i++)
	{
		for (DWORD j = 0; j < dataSize; j++)
		{
			output[tmpIdx] = ((input[i] >> (j * 8)) & flag);
			tmpIdx++;
		}
	}

}
