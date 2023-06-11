#pragma once
#include "Map.h"

enum class DIR
{
	UP = 0,
	UP_R,
	R,
	DOWN_R,
	DOWN,
	DOWN_L,
	L,
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
		Pos (1, -1),
		Pos (1, 0),
		Pos (1, 1),
		Pos (0, 1),
		Pos (-1, 1),
		Pos (-1, 0),
		Pos (-1, -1)
	};

protected:
	bool _bOn = false;
	Node* _pCurNode = nullptr;
	Map* _pMap = nullptr;
	NodeMgr* _pNodeMgr = nullptr;
};