#pragma once
#include "NodeMgr.h"

class PathFindAlgorithm
{
public:
	PathFindAlgorithm();
	~PathFindAlgorithm() {}

public:
	bool GetFindPathOn() { return _bFindPathOn; }
	void StartFindPath();
	virtual void FindPath() = 0;
	virtual void FindPathStepInto() = 0;

protected:
	Pos _direction[(int)DIR::NONE] =
	{
		Pos (0, -1),
		Pos (1, 0),
		Pos (0, 1),
		Pos (-1, 0),

		Pos (1, -1),
		Pos (1, 1),
		Pos (-1, 1),
		Pos (-1, -1)
	};

public:
	bool _bFindPathOn = false;
	bool _bFindPathStepOn = false;

protected:
	NodeMgr* _pNodeMgr = nullptr;
};