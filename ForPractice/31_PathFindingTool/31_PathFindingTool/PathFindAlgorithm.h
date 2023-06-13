#pragma once
#include "NodeMgr.h"

class PathFindAlgorithm
{
public:
	PathFindAlgorithm();
	~PathFindAlgorithm() {}

public:
	bool GetOn() { return _bOn; }
	void StartFindPath();
	virtual void FindPath() = 0;

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
	bool _bOn = false;

protected:
	Node* _pCurNode = nullptr;
	NodeMgr* _pNodeMgr = nullptr;
};