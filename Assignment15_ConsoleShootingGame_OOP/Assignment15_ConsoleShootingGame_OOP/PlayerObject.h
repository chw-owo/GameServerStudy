#pragma once
#include "BaseObject.h"
#include "ObjectManager.h"
#include "SceneLoad.h"
#include <time.h>

class CPlayerObject: public CBaseObject
{
	friend CObjectManager;
	friend CSceneLoad;

private:
	CPlayerObject(char icon, int hp, int speed);
	CPlayerObject();
	~CPlayerObject();

	void GetKeyInput();
	void Update();
	void Render();
	void OnCollision(CBaseObject* pTarget);

private:
	static CPlayerObject _Player;
	char _chIcon;
	int _iTotalHp = 0;
	int _iHp = 0;
	float _fSpeed = 0;
	clock_t _moveStart;

};

