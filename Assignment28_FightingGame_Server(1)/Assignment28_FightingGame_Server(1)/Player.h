#pragma once
#include "NetworkManager.h"
#include "RingBuffer.h"
#include "Typedef.h"

class PlayerManager;

// MOVE UNIT Define
#define dfMOVE_X 3
#define dfMOVE_Y 2

// DIRECTION TYPE Define
#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7

// POSITION RANGE Define
#define dfERROR_RANGE		50
#define dfRANGE_MOVE_TOP	50
#define dfRANGE_MOVE_LEFT	10
#define dfRANGE_MOVE_RIGHT	630
#define dfRANGE_MOVE_BOTTOM	470

// ATTACK RANGE Define
#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100
#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20

// Player Packet Flag
#define dfPLAYER_PACKET_MOVE_START 0b00000001
#define dfPLAYER_PACKET_MOVE_STOP 0b00000010

// Player State Flag
#define dfPLAYER_STATE_ALIVE 0b00000001
#define dfPLAYER_STATE_MOVE 0b00000010

// Init Setting
#define dfINIT_X (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) / 2
#define dfINIT_Y (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) / 2
#define dfMAX_HP 100

class Player
{
	friend PlayerManager;

public:
	Player(Session* pSession, uint32 ID);
	~Player();
	void Update();
	void OnCollision(Player* player);

public:
	uint16 GetX() { return _x; }
	uint16 GetY() { return _y; }

private:
	void DequeueRecvBuf();

	// Packet Flag Setting
private:
	void SetPacketMoveStart() { _packet |= dfPLAYER_PACKET_MOVE_START; }
	void SetPacketMoveStop() { _packet |= dfPLAYER_PACKET_MOVE_STOP; }
	
	void ResetPacketMoveStart() { _packet ^= dfPLAYER_PACKET_MOVE_START; }
	void ResetPacketMoveStop() { _packet ^= dfPLAYER_PACKET_MOVE_STOP; }

public:
	bool GetPacketMoveStart() { return (_packet & dfPLAYER_PACKET_MOVE_START); }
	bool GetPacketMoveStop() { return (_packet & dfPLAYER_PACKET_MOVE_STOP); }


	// State Flag Setting
private:
	void SetStateAlive() { _state |= dfPLAYER_STATE_ALIVE; }
	void SetStateDead() 
	{ 
		_state ^= dfPLAYER_STATE_ALIVE; 
		_pSession->SetSessionDead();
	}
	void SetStateMoveStart() { _state |= dfPLAYER_STATE_MOVE; }
	void SetStateMoveStop() { _state ^= dfPLAYER_STATE_MOVE; }

public:
	bool GetStateAlive() { return (_state & dfPLAYER_STATE_ALIVE); }
	bool GetStateMoving() { return ( _state & dfPLAYER_STATE_MOVE); }

private:
	bool HandlePacketMoveStart(int packetSize);
	bool HandlePacketMoveStop(int packetSize);
	bool HandlePacketAttack1(int packetSize);
	bool HandlePacketAttack2(int packetSize);
	bool HandlePacketAttack3(int packetSize);

private:
	Session* _pSession;

	uint32 _ID;
	uint8 _direction;
	uint8 _moveDirection;
	uint16 _x;
	uint16 _y;
	uint8 _hp;

	uint8 _packet = 0b00000000;
	uint8 _state = 0b00000001;
};

