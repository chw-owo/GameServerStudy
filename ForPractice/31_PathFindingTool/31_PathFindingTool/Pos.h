#pragma once

#define VERT_DIST 10
#define DIAG_DIST 14

// << GetDistance >> 
// 
// ������ ������ �� ã�� ��� ��Ŭ���� ����� ������ ���
// ( x^2 + y^2 ) * 10 �� int�� cast �Ͽ� ����Ѵ�
// �̸� �����Ͽ� ���� �Ÿ��� + 10��, �밢�� �Ÿ��� +14�� �Ѵ�

struct Pos
{
public:
	Pos() : _x(0), _y(0) {}
	Pos(int x, int y) : _x(x), _y(y) {}

public:
	int _x = 0;
	int _y = 0;

public:
	Pos& operator=(const Pos& other);
	Pos operator+(const Pos& other) { return Pos(_x + other._x, _y + other._y); }
	bool operator==(const Pos& other) { return (_x == other._x && _y == other._y); }
	bool operator!=(const Pos& other) { return (_x != other._x || _y != other._y); }
	bool operator < (const Pos& other) const;
	bool operator > (const Pos& other) const;

public:
	int GetDistance(Pos destPos);
};