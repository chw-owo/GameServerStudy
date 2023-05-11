#pragma once
#include "RingBuffer.h"
#include "Typedef.h"

class Session;
class PlayerManager;

class Player
{
	friend PlayerManager;

public:
	Player(Session* pSession, uint32 ID,
		uint8 direction, uint16 x, uint16 y, uint8 hp);
	~Player();
	void Update();
	void OnCollision(Player* player);

public:
	bool GetDead() { return _dead; }
	uint16 GetX() { return _x; }
	uint16 GetY() { return _y; }

private:
	void DequeueRecvBuf();

private:

	bool _dead = false;
	bool _move = false;
	Session* _pSession;

	uint32 _ID;
	uint8 _direction;
	uint16 _x;
	uint16 _y;
	uint8 _hp;
};

