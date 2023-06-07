#pragma once
#include <list>
using namespace std;

// Map =========================================

#define X_MAX 100
#define Y_MAX 50

enum MAP_STATE
{
	NONE = 0,
	OBSTACLE,
	OPEN,
	CLOSE,
	START,
	DEST
};

extern char g_chMap[Y_MAX][X_MAX];

// Position ====================================

struct Pos
{
public:
	int _x = 0;
	int _y = 0;

public:
	Pos operator+(const Pos& other) { return Pos{ _x + other._x, _y + other._y}; }
	bool operator==(const Pos& other) { return (_x == other._x &&  _y == other._y); }
	bool operator!=(const Pos& other) { return (_x != other._x || _y != other._y); }
	bool operator < (const Pos& other) const 
	{ 
		if (_x != other._x)
			return (_x < other._x);
		else
			return (_y < other._y);	
	}
	bool operator > (const Pos& other) const 
	{ 
		if (_x != other._x)
			return (_x > other._x);
		else
			return (_y > other._y);
	}
};

// PathFinder =================================

class PathFinder
{
public:
	virtual void SetStartDestPoint(Pos startPos, Pos destPos) = 0;
	virtual void FindPath() = 0;

public:
	list<Pos> _resultPath;
	bool _bComplete = false;
};

