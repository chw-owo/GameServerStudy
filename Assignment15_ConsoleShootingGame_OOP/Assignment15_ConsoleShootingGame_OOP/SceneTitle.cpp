#include "SceneTitle.h"

#include "ObjectManager.h"
#include "ScreenBuffer.h"
#include "GraphicObject.h"

#include "string.h"
#include <Windows.h>
#include <iostream>

#define BUFFER_SIZE 8
CSceneTitle::CSceneTitle()
{
	_SceneType = TITLE;
	_pObjectMgr = CObjectManager::GetInstance();
	_pScreenBuffer = CScreenBuffer::GetInstance();
	GetTitleData();
}

void CSceneTitle::Update()
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

void CSceneTitle::GetKeyInput()
{
	// Continue Prev Play
	if (GetAsyncKeyState(VK_NUMPAD1)
		|| GetAsyncKeyState(0x31))
	{
		_SceneType = LOAD;
	}
	// Start New Play
	else if (GetAsyncKeyState(VK_NUMPAD2)
		|| GetAsyncKeyState(0x32))
	{
		SetStageData(0);
		_SceneType = LOAD;
	}
	// Exit
	else if (GetAsyncKeyState(VK_ESCAPE))
	{
		_bExit = true;
	}
}

void CSceneTitle::SetStageData(int iStage)
{
	char chStageData[BUFFER_SIZE] = { '\0' };
	sprintf_s(chStageData, BUFFER_SIZE, "%d", iStage);

	FILE* pFile;
	fopen_s(&pFile, ".\\Data\\PlayData.txt", "w");
	fwrite(chStageData, 1, BUFFER_SIZE, pFile);
	fclose(pFile);
}

void CSceneTitle::GetTitleData()
{
	FILE* pFile;
	fopen_s(&pFile, ".\\Data\\Title\\Title.txt", "rb");
	fseek(pFile, 0, SEEK_END);
	int size = ftell(pFile);
	rewind(pFile);
	char* titleData = (char*)(malloc(size));
	memset(titleData, 0, sizeof(char));
	fread(titleData, 1, size, pFile);
	fclose(pFile);

	int x = 0;
	int y = 0;
	CGraphicObject graphic;

	for (int i = 0; i < size; i++)
	{
		if (titleData[i] == ' ')
		{
			x++;
		}
		else if (titleData[i] == '\n')
		{
			y++;
			x = 0;
		}
		else
		{
			x++;
			_pObjectMgr->CreateObject(&graphic, x, y, titleData[i]);
		}
	}

	free(titleData);
}

CSceneTitle::~CSceneTitle()
{
	_pObjectMgr->DestroyAllObject();
	_pScreenBuffer->BufferClear();
	_pScreenBuffer->BufferFlip();
}