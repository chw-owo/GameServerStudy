#pragma once
#include "SceneBase.h"

class CSceneGame : public CSceneBase
{
	friend CSceneManager;

private:
	CSceneGame();
	~CSceneGame();
	void Update();
	void GetKeyInput();
};


