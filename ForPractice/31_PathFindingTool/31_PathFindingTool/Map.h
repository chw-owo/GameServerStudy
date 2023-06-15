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
		NONE = 0,
		OBSTACLE,
		OPEN,
		CLOSE,
		START,
		DEST,
		DIAG_CUZ,
		RANGE_OUT
	};

public:
	void ClearOpenCloseState();
	void SetMapState(int x, int y, STATE state);
	STATE GetMapState(int x, int y);
	void SetMapState(Pos pos, STATE state);
	STATE GetMapState(Pos pos);
	
public:
	Pos _startPos;
	Pos _destPos;
	char _chMap[Y_MAX][X_MAX] = { STATE::NONE, };

private:
	static Map _Map;
};

