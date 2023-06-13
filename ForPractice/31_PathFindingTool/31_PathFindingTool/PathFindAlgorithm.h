#pragma once
#include "NodeMgr.h"

enum class DIR
{
	UP = 0,
	R,
	DOWN,
	L,
	UP_R = 4,
	DOWN_R,
	DOWN_L,
	UP_L,
	END = 8
};

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
	Pos _direction[(int)DIR::END] =
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