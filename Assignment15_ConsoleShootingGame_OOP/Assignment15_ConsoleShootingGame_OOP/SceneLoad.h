#pragma once
#include "SceneBase.h"
class CSceneLoad : public CSceneBase
{
	friend CSceneManager;

private:
	CSceneLoad();
	~CSceneLoad();
	void Update(void);
	void GetKeyInput();
	void GetGameData();

private:
	int _iPlayData;
	int _iPlayFinalData;
	
};
