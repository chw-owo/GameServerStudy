#pragma once
#include "SceneBase.h"

class CSceneClear: public CSceneBase
{
	friend CSceneManager;

private:
	CSceneClear();
	~CSceneClear();
	void Update();
	void GetKeyInput();
	void SaveAndGetClearData();

private:
	bool _bFinal = false;
};

