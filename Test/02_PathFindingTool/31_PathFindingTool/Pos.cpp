#include "Pos.h"
#include <cmath>

Pos& Pos::operator=(const Pos& other)
{
	_x = other._x;
	_y = other._y;
	return *this;
}

bool Pos::operator < (const Pos& other) const
{
	if (_x != other._x)
		return (_x < other._x);
	else
		return (_y < other._y);
}

bool Pos::operator > (const Pos& other) const
{
	if (_x != other._x)
		return (_x > other._x);
	else
		return (_y > other._y);
}

int Pos::GetDistanceToDest(Pos destPos)
{
	return (abs(destPos._x - _x) + abs(destPos._y - _y)) * VERT_DIST;
}


int Pos::GetDistance(Pos pos2, bool vert)
{
	if (vert)
	{
		return (abs(pos2._x - _x) + abs(pos2._y - _y))* VERT_DIST;
	}
	else
	{
		return abs(pos2._x - _x) * DIAG_DIST;
	}
}

