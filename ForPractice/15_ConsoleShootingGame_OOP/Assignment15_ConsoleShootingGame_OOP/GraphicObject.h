#pragma once
#include "BaseObject.h"
#include "ObjectManager.h"
#include "SceneTitle.h"
#include "SceneOver.h"
#include "SceneClear.h"

class CGraphicObject: public CBaseObject
{
	friend CObjectManager;
	friend CSceneTitle;
	friend CSceneOver;
	friend CSceneClear;

	CGraphicObject(int x, int y, char icon);
	CGraphicObject();
	~CGraphicObject();

	void Update();
	void Render();
	void OnCollision(CBaseObject* pTarget);

private:
	char _chIcon;

};

