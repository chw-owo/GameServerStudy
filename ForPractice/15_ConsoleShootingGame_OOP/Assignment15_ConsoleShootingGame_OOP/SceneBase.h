#pragma once
#include "ObjectManager.h"
#include "ScreenBuffer.h"

class CSceneManager;

class CSceneBase
{
	friend CSceneManager;

public:

	enum SceneType
	{
		TITLE,
		LOAD,
		GAME,
		OVER,
		CLEAR
	};

	virtual void Update() {};
	virtual ~CSceneBase() {};

protected:
	CObjectManager* _pObjectMgr;
	CScreenBuffer* _pScreenBuffer;
	SceneType _SceneType;
	bool _bExit = false;
};

