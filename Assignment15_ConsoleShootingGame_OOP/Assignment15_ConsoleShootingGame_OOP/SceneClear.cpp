#include "SceneClear.h"

#include "ObjectManager.h"
#include "ScreenBuffer.h"
#include "GraphicObject.h"

#include "string.h"
#include <Windows.h>
#include <iostream>

CSceneClear::CSceneClear()
{
	_SceneType = CLEAR;
	_pObjectMgr = CObjectManager::GetInstance();
	_pScreenBuffer = CScreenBuffer::GetInstance();
	SaveAndGetClearData();
}

void CSceneClear::Update()
{
	// input section
	GetKeyInput();

	// login section
	_pObjectMgr->Update();

	// render section
	_pScreenBuffer->BufferClear();
	_pObjectMgr->Render();
	_pScreenBuffer->BufferFlip();
}

void CSceneClear::GetKeyInput()
{
	// Go Back To Title
	if (_bFinal && GetAsyncKeyState(VK_RETURN))
	{
		_SceneType = TITLE;
	}
	// Go To Next Level
	else if (!_bFinal && GetAsyncKeyState(VK_RETURN))
	{
		_SceneType = LOAD;
	}
	// Exit
	else if (GetAsyncKeyState(VK_ESCAPE))
	{
		_bExit = true;
	}
}

void CSceneClear::SaveAndGetClearData()
{
	// Get Current Stage =============================================================

	FILE* pFile;
	fopen_s(&pFile, ".\\Data\\PlayData.txt", "r");
	fseek(pFile, 0, SEEK_END);
	int stageSize = ftell(pFile);
	rewind(pFile);
	char* chStageData = (char*)(malloc(stageSize));
	fread(chStageData, 1, stageSize, pFile);
	fclose(pFile);
	int iStage = atoi(chStageData);

	// Get Final Stage ===============================================================

	fopen_s(&pFile, ".\\Data\\PlayFinalData.txt", "r");
	fseek(pFile, 0, SEEK_END);
	int size = ftell(pFile);
	rewind(pFile);
	char* playFinalData = (char*)(malloc(size));
	fread(playFinalData, 1, size, pFile);
	fclose(pFile);
	int iFinalData = atoi(playFinalData);
	free(playFinalData);

	if (iFinalData == iStage)
		_bFinal = true;
	else
		_bFinal = false;

	// Get Current Stage Clear Info Root from ClearInfo.txt =======================

	fopen_s(&pFile, ".\\Data\\Clear\\ClearInfo.txt", "r");
	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	rewind(pFile);
	char* clearInfo = (char*)(malloc(size));
	fread(clearInfo, 1, size, pFile);
	fclose(pFile);

	char sep[] = { "\n " };
	char* tok = NULL;
	char* next = NULL;

	int cnt = 0;
	char* curStageRoot = (char*)(malloc(size));
	tok = strtok_s(clearInfo, sep, &next);

	while (cnt < iStage && tok != NULL)
	{
		tok = strtok_s(NULL, sep, &next);
		cnt++;
	}

	strcpy_s(curStageRoot, size, tok);
	free(clearInfo);

	// Get Clear Data =============================================================

	fopen_s(&pFile, curStageRoot, "rb");
	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	rewind(pFile);
	char* clearData = (char*)(malloc(size));
	memset(clearData, 0, sizeof(char));
	fread(clearData, 1, size, pFile);
	fclose(pFile);

	int x = 0;
	int y = 0;
	CGraphicObject graphic;

	for (int i = 0; i < size; i++)
	{
		if (clearData[i] == ' ')
		{
			x++;
		}
		else if (clearData[i] == '\n')
		{
			y++;
			x = 0;
		}
		else
		{
			x++;
			_pObjectMgr->CreateObject(&graphic, x, y, clearData[i]);
		}
	}

	free(clearData);

	// Set Current Stage =============================================================

	if (!_bFinal)
	{
		iStage++;
		sprintf_s(chStageData, stageSize, "%d", iStage);
		fopen_s(&pFile, ".\\Data\\PlayData.txt", "w");
		fwrite(chStageData, 1, stageSize, pFile);
		fclose(pFile);
	}
	free(chStageData);
}

CSceneClear::~CSceneClear()
{
	_pObjectMgr->DestroyAllObject();
	_pScreenBuffer->BufferClear();
	_pScreenBuffer->BufferFlip();
}