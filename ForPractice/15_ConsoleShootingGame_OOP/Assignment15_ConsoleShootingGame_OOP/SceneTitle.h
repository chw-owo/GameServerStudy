#pragma once
#include "SceneBase.h"

class CSceneTitle : public CSceneBase
{
	friend CSceneManager;

private:
	CSceneTitle();
	~CSceneTitle();
	void Update();
	void GetKeyInput();
	void GetTitleData();
	void SetStageData(int iStage);

};