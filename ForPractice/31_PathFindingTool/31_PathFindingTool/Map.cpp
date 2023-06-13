#include "Map.h"
#include <stdio.h>

Map* Map::GetInstance()
{
	static Map _Map;
	return &_Map;
}

Map::Map()
{
	_startPos = Pos(0,0);
	_destPos = Pos(X_MAX - 1, Y_MAX - 1);
	_chMap[_startPos._y][_startPos._x] = START;
	_chMap[_destPos._y][_destPos._x] = DEST;
}

void Map::SetMapState(int x, int y, STATE state)
{
	if (x < 0 || y < 0 || x >= X_MAX || y >= Y_MAX)
		return;

	switch (state)
	{
	case EMPTY:
	case OBSTACLE:
		_chMap[y][x] = state;
		break;

	case OPEN:
	case CLOSE:
		if(_chMap[y][x] != START && _chMap[y][x] != DEST)
			_chMap[y][x] = state;
		break;

	case START:
		_chMap[_startPos._y][_startPos._x] = EMPTY;
		_startPos._x = x;
		_startPos._y = y;
		_chMap[_startPos._y][_startPos._x] = START;
		break;

	case DEST:
		_chMap[_destPos._y][_destPos._x] = EMPTY;
		_destPos._x = x;
		_destPos._y = y;
		_chMap[_destPos._y][_destPos._x] = DEST;
		break;

	}

}

Map::STATE Map::GetMapState(int x, int y)
{
	if (x < 0 || y < 0 || x >= X_MAX || y >= Y_MAX)
		return RANGE_OUT;

	return (STATE)_chMap[y][x]; 
}

void Map::ClearOpenCloseState()
{
	for (int y = 0; y < Y_MAX; y++)
	{
		for (int x = 0; x < X_MAX; x++)
		{
			if ((STATE)_chMap[y][x] == OPEN || (STATE)_chMap[y][x] == CLOSE)
			{
				_chMap[y][x] = EMPTY;
			}
		}
	}
}

