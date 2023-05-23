#pragma once
#include "SceneBase.h"

class CSceneManager
{
public:
	static CSceneManager* GetInstance(void);
	void Run(void);

private:
	CSceneManager();
	~CSceneManager();
	void LoadScene(CSceneBase::SceneType scene);

public:
	bool _bExit = false;

private:
	static CSceneManager _SceneMgr;
	CSceneBase::SceneType _SceneType = CSceneBase::TITLE;
	CSceneBase* _pScene = nullptr;
};
