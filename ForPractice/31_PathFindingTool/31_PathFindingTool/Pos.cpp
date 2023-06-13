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

int Pos::GetDistance(Pos destPos)
{
	return (int)((pow(destPos._x - _x, 2) + pow(destPos._y - _y, 2)) * 10); 	
}

