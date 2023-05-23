#pragma once
#include "ScreenBuffer.h"

class CBaseObject
{
public:
	enum ObjectType
	{
		PLAYER,
		BULLET,
		ENEMY,
		GRAPHIC
	};

public:
	CBaseObject();
	~CBaseObject();
	virtual void Update(void) = 0;
	virtual void Render(void) = 0;
	virtual void OnCollision(CBaseObject* pTarget) = 0;

public:
	bool GetDead();
	ObjectType GetObjectType(void);
	int GetX();
	int GetY();

protected:
	bool _bDead;
	ObjectType _ObjectType;
	int _iX;
	int _iY;

	CScreenBuffer* _pScreenBuffer;
};

