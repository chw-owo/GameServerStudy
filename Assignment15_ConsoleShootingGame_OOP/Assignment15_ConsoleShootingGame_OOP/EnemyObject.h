#pragma once
#include "BaseObject.h"
#include "ObjectManager.h"
#include "SceneLoad.h"
#include "SceneGame.h"

class CEnemyObject: public CBaseObject
{
	friend CObjectManager;
	friend CSceneLoad;
	friend CSceneGame;

private:
	CEnemyObject(int x, int y, char type, char icon, int hp);
	CEnemyObject();
	~CEnemyObject();

	void Update();
	void Render();
	void OnCollision(CBaseObject* pTarget);

private:
	char _chType;
	char _chIcon;
	int _iHp = 0;
	static int _iCnt;
};

