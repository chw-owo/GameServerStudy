#pragma once
#include "SerializePacket.h"
class Session;

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
#define dfPLAYER_packetState_MOVE_START	0b00000001
#define dfPLAYER_packetState_MOVE_STOP	0b00000010
#define dfPLAYER_packetState_ATTACK1		0b00000100
#define dfPLAYER_packetState_ATTACK2		0b00001000
#define dfPLAYER_packetState_ATTACK3		0b00010000

// Player State Flag
#define dfPLAYER_playerState_ALIVE		0b00000001
#define dfPLAYER_playerState_MOVE			0b00000010

// Init Setting
#define dfINIT_X (dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) / 2
#define dfINIT_Y (dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) / 2
#define dfMAX_HP 100

class Player
{

public:
	Player(Session* pSession, int ID);
	~Player();
	void Update();
	void DequeueRecvBuf();

public:
	void Attacked(char damage);

public:
	Session* GetSession()		{ return _pSession; }
	int GetID()				{ return _ID; }
	char GetMoveDirection()	{ return _moveDirection; }
	char GetDirection()		{ return _direction; }
	short GetX()				{ return _x; }
	short GetY()				{ return _y; }
	char GetHp()				{ return _hp; }

public:
	bool GetPacketMoveStart()	{ return (_packetState & dfPLAYER_packetState_MOVE_START); }
	bool GetPacketMoveStop()	{ return (_packetState & dfPLAYER_packetState_MOVE_STOP); }
	bool GetPacketAttack1()		{ return (_packetState & dfPLAYER_packetState_ATTACK1); }
	bool GetPacketAttack2()		{ return (_packetState & dfPLAYER_packetState_ATTACK2); }
	bool GetPacketAttack3()		{ return (_packetState & dfPLAYER_packetState_ATTACK3); }

public:
	bool GetStateAlive()		{ return (_playerState & dfPLAYER_playerState_ALIVE); }
	bool GetStateMoving()		{ return (_playerState & dfPLAYER_playerState_MOVE); }

public:
	void ResetPacketMoveStart() { _packetState ^= dfPLAYER_packetState_MOVE_START; }
	void ResetPacketMoveStop()	{ _packetState ^= dfPLAYER_packetState_MOVE_STOP; }
	void ResetPacketAttack1()	{ _packetState ^= dfPLAYER_packetState_ATTACK1; }
	void ResetPacketAttack2()	{ _packetState ^= dfPLAYER_packetState_ATTACK2; }
	void ResetPacketAttack3()	{ _packetState ^= dfPLAYER_packetState_ATTACK3; }

	// Packet Flag Setting
private:
	void SetPacketMoveStart()	{ _packetState |= dfPLAYER_packetState_MOVE_START; }
	void SetPacketMoveStop()	{ _packetState |= dfPLAYER_packetState_MOVE_STOP; }
	void SetPacketAttack1()		{ _packetState |= dfPLAYER_packetState_ATTACK1; }
	void SetPacketAttack2()		{ _packetState |= dfPLAYER_packetState_ATTACK2; }
	void SetPacketAttack3()		{ _packetState |= dfPLAYER_packetState_ATTACK3; }

	// State Flag Setting
private:
	void SetStateAlive()		{ _playerState |= dfPLAYER_playerState_ALIVE; }
	void SetStateDead()			{ _playerState ^= dfPLAYER_playerState_ALIVE; }
	void SetStateMoveStart()	{ _playerState |= dfPLAYER_playerState_MOVE;  }
	void SetStateMoveStop()		{ _playerState ^= dfPLAYER_playerState_MOVE;  }	

private:
	void HandlePacketMoveStart();
	void HandlePacketMoveStop();
	void HandlePacketAttack1();
	void HandlePacketAttack2();
	void HandlePacketAttack3();

private:
	Session* _pSession;
	SerializePacket _packetBuffer;

	int _ID;
	char _direction;
	char _moveDirection;
	short _x;
	short _y;
	char _hp;

	char _packetState;
	char _playerState;
};

