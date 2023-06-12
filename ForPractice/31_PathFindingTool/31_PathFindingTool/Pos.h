#pragma once
struct Pos
{
public:
	Pos() : _x(0), _y(0) {}
	Pos(int x, int y) : _x(x), _y(y) {}

public:
	int _x = 0;
	int _y = 0;

public:
	Pos operator+(const Pos& other) { return Pos(_x + other._x, _y + other._y); }
	bool operator==(const Pos& other) { return (_x == other._x && _y == other._y); }
	bool operator!=(const Pos& other) { return (_x != other._x || _y != other._y); }
	bool operator < (const Pos& other) const;
	bool operator > (const Pos& other) const;

public:
	double GetDistance(Pos destPos);
};