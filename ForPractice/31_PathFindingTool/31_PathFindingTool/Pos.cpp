#include "Pos.h"
#include <cmath>

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

double Pos::GetDistance(Pos destPos)
{
	return sqrt(pow(destPos._x - _x, 2) + pow(destPos._y - _y, 2));
}
