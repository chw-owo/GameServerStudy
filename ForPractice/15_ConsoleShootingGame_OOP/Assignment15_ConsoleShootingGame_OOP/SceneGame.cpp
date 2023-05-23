#include "SceneGame.h"

#include "ObjectManager.h"
#include "ScreenBuffer.h"
#include "EnemyObject.h"

#include "string.h"
#include <Windows.h>
#include <iostream>

#define BUFFER_SIZE 8
CSceneGame::CSceneGame()
{
	_SceneType = GAME;
	_pObjectMgr = CObjectManager::GetInstance();
	_pScreenBuffer = CScreenBuffer::GetInstance(); 
}

void CSceneGame::Update()
{
	// input section
	GetKeyInput();

	// login section
	_pObjectMgr->Update();
	if (CEnemyObject::_iCnt == 0)
		_SceneType = CLEAR;

	// render section
	_pScreenBuffer->BufferClear();
	_pObjectMgr->Render();
	_pScreenBuffer->BufferFlip();
}

void CSceneGame::GetKeyInput()
{
	// Exit
	if (GetAsyncKeyState(VK_ESCAPE))
	{
		_bExit = true;
	}
}

CSceneGame::~CSceneGame()
{
	_pObjectMgr->DestroyAllObject();
	_pScreenBuffer->BufferClear();
	_pScreenBuffer->BufferFlip();
}