#pragma once
#include "Pos.h"

#define X_MAX 100
#define Y_MAX 50

class Map
{
private:
	Map();
	~Map() {}

public:
	static Map* GetInstance();

public:
	enum STATE
	{
		EMPTY = 0,
		OBSTACLE,
		OPEN,
		CLOSE,
		START,
		DEST,
		RANGE_OUT
	};

public:
	void SetMapState(int x, int y, STATE state);
	STATE GetMapState(int x, int y);

public:
	Pos _startPos;
	Pos _destPos;
	char _chMap[Y_MAX][X_MAX] = { STATE::EMPTY, };

private:
	static Map _Map;
};

