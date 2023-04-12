#pragma once
#include "BaseObject.h"
#include "PlayerObject.h"
#include "ObjectManager.h"
#include "SceneLoad.h"
#include <time.h>

class CBulletObject : public CBaseObject
{
	friend CObjectManager;
	friend CSceneLoad;

public:
	int GetAttack();

private:
	CBulletObject(char icon, int attack, int speed, CPlayerObject* player);
	CBulletObject();
	~CBulletObject();

	void Update();
	void Render();
	void OnCollision(CBaseObject* pTarget);

	void Deactivate();

private:
	char _chIcon;
	int _iAttack;
	float _fSpeed;
	CPlayerObject* _pPlayer;
	clock_t _moveStart;
	static clock_t _coolTimeCheck;
};


