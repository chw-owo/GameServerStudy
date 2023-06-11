#include "Map.h"

Map* Map::GetInstance()
{
	static Map _Map;
	return &_Map;
}

void Map::Initialize()
{
	_startPos._x = 0;
	_startPos._y = 0;
	_destPos._x = X_MAX - 1;
	_destPos._y = Y_MAX - 1;
	_chMap[_startPos._y][_startPos._x] = START;
	_chMap[_destPos._y][_destPos._x] = DEST;
}

void Map::SetMapState(int x, int y, STATE state)
{
	switch (state)
	{
	case NONE:
	case OBSTACLE:
		_chMap[y][x] = state;
		break;

	case OPEN:
	case CLOSE:
		if(_chMap[y][x] != START && _chMap[y][x] != DEST)
			_chMap[y][x] = state;
		break;

	case START:
		_chMap[_startPos._y][_startPos._x] = NONE;
		_startPos._x = x;
		_startPos._y = y;
		_chMap[_startPos._y][_startPos._x] = START;
		break;

	case DEST:
		_chMap[_destPos._y][_destPos._x] = NONE;
		_destPos._x = x;
		_destPos._y = y;
		_chMap[_destPos._y][_destPos._x] = DEST;
		break;

	}

}

bool Map::CanGo(Pos pos)
{
	if (pos._x < 0 || pos._y < 0 || pos._x >= X_MAX || pos._y >= Y_MAX ||
		_chMap[pos._y][pos._x] == OBSTACLE)
	{
		return false;
	}

	return true;
}
