#pragma once

#define VERT_DIST 10
#define DIAG_DIST 14

// << GetDistance >> 
// 
// 목적지 설정과 길 찾기 모두 유클리드 방법의 변형을 사용
// ( x^2 + y^2 ) * 10 을 int로 cast 하여 사용한다
// 이를 적용하여 직선 거리는 + 10을, 대각선 거리는 +14를 한다

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