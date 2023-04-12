#pragma once
#include "SceneBase.h"

class CSceneOver : public CSceneBase
{
	friend CSceneManager;
		
private:
	CSceneOver();
	~CSceneOver();
	void Update(void);
	void GetKeyInput();
	void GetOverData();
};