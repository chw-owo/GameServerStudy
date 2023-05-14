#pragma once
#include "NetworkManager.h"
#include "RingBuffer.h"
#include "Typedef.h"

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

// Player Packet Flag
#define dfPLAYER_PACKET_MOVE_START	0b00000001
#define dfPLAYER_PACKET_MOVE_STOP	0b00000010
#define dfPLAYER_PACKET_ATTACK1		0b00000100
#define dfPLAYER_PACKET_ATTACK2		0b00001000
#define dfPLAYER_PACKET_ATTACK3		0b00010000

// Player State Flag
#define dfPLAYER_STATE_ALIVE		0b00000001
#define dfPLAYER_STATE_MOVE			0b00000010

// Init Setting
#define dfINIT_X (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) / 2
#define dfINIT_Y (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) / 2
#define dfMAX_HP 100

class Player
{

public:
	Player(Session* pSession, uint32 ID);
	~Player();
	void Update();
	void DequeueRecvBuf();

public:
	void Attacked(uint8 damage);

public:
	Session* GetSession()		{ return _pSession; }
	uint32 GetID()				{ return _ID; }
	uint8 GetMoveDirection()	{ return _moveDirection; }
	uint8 GetDirection()		{ return _direction; }
	uint16 GetX()				{ return _x; }
	uint16 GetY()				{ return _y; }
	uint8 GetHp()				{ return _hp; }

public:
	bool GetPacketMoveStart()	{ return (_packet & dfPLAYER_PACKET_MOVE_START); }
	bool GetPacketMoveStop()	{ return (_packet & dfPLAYER_PACKET_MOVE_STOP); }
	bool GetPacketAttack1()		{ return (_packet & dfPLAYER_PACKET_ATTACK1); }
	bool GetPacketAttack2()		{ return (_packet & dfPLAYER_PACKET_ATTACK2); }
	bool GetPacketAttack3()		{ return (_packet & dfPLAYER_PACKET_ATTACK3); }

public:
	bool GetStateAlive()		{ return (_state & dfPLAYER_STATE_ALIVE); }
	bool GetStateMoving()		{ return (_state & dfPLAYER_STATE_MOVE); }

public:
	void ResetPacketMoveStart() { _packet ^= dfPLAYER_PACKET_MOVE_START; }
	void ResetPacketMoveStop()	{ _packet ^= dfPLAYER_PACKET_MOVE_STOP; }
	void ResetPacketAttack1()	{ _packet ^= dfPLAYER_PACKET_ATTACK1; }
	void ResetPacketAttack2()	{ _packet ^= dfPLAYER_PACKET_ATTACK2; }
	void ResetPacketAttack3()	{ _packet ^= dfPLAYER_PACKET_ATTACK3; }

	// Packet Flag Setting
private:
	void SetPacketMoveStart()	{ _packet |= dfPLAYER_PACKET_MOVE_START; }
	void SetPacketMoveStop()	{ _packet |= dfPLAYER_PACKET_MOVE_STOP; }
	void SetPacketAttack1()		{ _packet |= dfPLAYER_PACKET_ATTACK1; }
	void SetPacketAttack2()		{ _packet |= dfPLAYER_PACKET_ATTACK2; }
	void SetPacketAttack3()		{ _packet |= dfPLAYER_PACKET_ATTACK3; }

	// State Flag Setting
private:
	void SetStateAlive()		{ _state |= dfPLAYER_STATE_ALIVE; }
	void SetStateDead()			{ _state ^= dfPLAYER_STATE_ALIVE; }
	void SetStateMoveStart()	{ _state |= dfPLAYER_STATE_MOVE;  }
	void SetStateMoveStop()		{ _state ^= dfPLAYER_STATE_MOVE;  }	

private:
	void HandlePacketMoveStart(int packetSize);
	void HandlePacketMoveStop(int packetSize);
	void HandlePacketAttack1(int packetSize);
	void HandlePacketAttack2(int packetSize);
	void HandlePacketAttack3(int packetSize);

private:
	Session* _pSession;
	
	uint32 _ID;
	uint8 _direction;
	uint8 _moveDirection;
	uint16 _x;
	uint16 _y;
	uint8 _hp;

	uint8 _packet;
	uint8 _state;
};

