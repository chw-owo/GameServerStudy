#include "SceneOver.h"

#include "ObjectManager.h"
#include "ScreenBuffer.h"
#include "GraphicObject.h"

#include "string.h"
#include <Windows.h>
#include <iostream>

CSceneOver::CSceneOver()
{
	_SceneType = OVER;
	_pObjectMgr = CObjectManager::GetInstance();
	_pScreenBuffer = CScreenBuffer::GetInstance();
	GetOverData();
}

void CSceneOver::Update()
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

void CSceneOver::GetKeyInput()
{
	// Go Back To Title
	if (GetAsyncKeyState(VK_RETURN))
	{
		_SceneType = TITLE;
	}
	// Exit
	else if (GetAsyncKeyState(VK_ESCAPE))
	{
		_bExit = true;
	}
}

void CSceneOver::GetOverData()
{
	FILE* pFile;
	fopen_s(&pFile, ".\\Data\\Over\\Over.txt", "rb");
	fseek(pFile, 0, SEEK_END);
	int size = ftell(pFile);
	rewind(pFile);
	char* overData = (char*)(malloc(size));
	memset(overData, 0, sizeof(char));
	fread(overData, 1, size, pFile);
	fclose(pFile);

	int x = 0;
	int y = 0;
	CGraphicObject graphic;

	for (int i = 0; i < size; i++)
	{
		if (overData[i] == ' ')
		{
			x++;
		}
		else if (overData[i] == '\n')
		{
			y++;
			x = 0;
		}
		else
		{
			x++;
			_pObjectMgr->CreateObject(&graphic, x, y, overData[i]);
		}
	}

	free(overData);
}

CSceneOver::~CSceneOver()
{
	_pObjectMgr->DestroyAllObject();
	_pScreenBuffer->BufferClear();
	_pScreenBuffer->BufferFlip();
}