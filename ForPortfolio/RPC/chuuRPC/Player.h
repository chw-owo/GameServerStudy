#pragma once

// MOVE UNIT Define
#define dfMOVE_X					3
#define dfMOVE_Y					2

// DIRECTION TYPE Define
#define dfPACKET_MOVE_DIR_LL		0
#define dfPACKET_MOVE_DIR_LU		1
#define dfPACKET_MOVE_DIR_UU		2
#define dfPACKET_MOVE_DIR_RU		3
#define dfPACKET_MOVE_DIR_RR		4
#define dfPACKET_MOVE_DIR_RD		5
#define dfPACKET_MOVE_DIR_DD		6
#define dfPACKET_MOVE_DIR_LD		7

// POSITION RANGE Define
#define dfERROR_RANGE				50
#define dfRANGE_MOVE_TOP			50
#define dfRANGE_MOVE_LEFT			10
#define dfRANGE_MOVE_RIGHT			630
#define dfRANGE_MOVE_BOTTOM			470

// Packet State Flag
#define dfPACKET_STATE_MOVE_START	0b00000001
#define dfPACKET_STATE_MOVE_STOP	0b00000010
#define dfPACKET_STATE_ATTACK1		0b00000100
#define dfPACKET_STATE_ATTACK2		0b00001000
#define dfPACKET_STATE_ATTACK3		0b00010000

// Player State Flag
#define dfPLAYER_STATE_ALIVE		0b00000001
#define dfPLAYER_STATE_MOVE			0b00000010

// Init Setting
#define dfMAX_HP					100
#define dfINIT_X					(dfRANGE_MOVE_RIGHT - dfRANGE_MOVE_LEFT) / 2
#define dfINIT_Y					(dfRANGE_MOVE_BOTTOM - dfRANGE_MOVE_TOP) / 2

class Session;
class GameServer;
class Player
{
	friend GameServer;

public:
	Player(Session* pSession, int ID);
	~Player();
	void Update();

public:
	void Attacked(char damage);

public:
	Session* GetSession() { return _pSession; }
	int GetID() { return _ID; }
	char GetMoveDirection() { return _moveDirection; }
	char GetDirection() { return _direction; }
	short GetX() { return _x; }
	short GetY() { return _y; }
	char GetHp() { return _hp; }

public:
	bool GetPacketState_MoveStart() { return (_packetState & dfPACKET_STATE_MOVE_START); }
	bool GetPacketState_MoveStop() { return (_packetState & dfPACKET_STATE_MOVE_STOP); }
	bool GetPacketState_Attack1() { return (_packetState & dfPACKET_STATE_ATTACK1); }
	bool GetPacketState_Attack2() { return (_packetState & dfPACKET_STATE_ATTACK2); }
	bool GetPacketState_Attack3() { return (_packetState & dfPACKET_STATE_ATTACK3); }

public:
	bool GetPlayerState_Alive() { return (_playerState & dfPLAYER_STATE_ALIVE); }
	bool GetPlayerState_Moving() { return (_playerState & dfPLAYER_STATE_MOVE); }

public:
	void ResetPacketState_MoveStart() { _packetState ^= dfPACKET_STATE_MOVE_START; }
	void ResetPacketState_MoveStop() { _packetState ^= dfPACKET_STATE_MOVE_STOP; }
	void ResetPacketState_Attack1() { _packetState ^= dfPACKET_STATE_ATTACK1; }
	void ResetPacketState_Attack2() { _packetState ^= dfPACKET_STATE_ATTACK2; }
	void ResetPacketState_Attack3() { _packetState ^= dfPACKET_STATE_ATTACK3; }

	// Packet Flag Setting
private:
	void SetPacketState_MoveStart() { _packetState |= dfPACKET_STATE_MOVE_START; }
	void SetPacketState_MoveStop() { _packetState |= dfPACKET_STATE_MOVE_STOP; }
	void SetPacketState_Attack1() { _packetState |= dfPACKET_STATE_ATTACK1; }
	void SetPacketState_Attack2() { _packetState |= dfPACKET_STATE_ATTACK2; }
	void SetPacketState_Attack3() { _packetState |= dfPACKET_STATE_ATTACK3; }

	// State Flag Setting
private:
	void SetPlayerState_Alive() { _playerState |= dfPLAYER_STATE_ALIVE; }
	void SetPlayerState_Dead() { _playerState ^= dfPLAYER_STATE_ALIVE; }
	void SetPlayerState_MoveStart() { _playerState |= dfPLAYER_STATE_MOVE; }
	void SetPlayerState_MoveStop() { _playerState ^= dfPLAYER_STATE_MOVE; }

private:
	void MoveStart(char direction, short x, short y);
	void MoveStop(char direction, short x, short y);
	void Attack1(char direction, short x, short y);
	void Attack2(char direction, short x, short y);
	void Attack3(char direction, short x, short y);

private:
	Session* _pSession;

	int _ID;
	char _direction;
	char _moveDirection;
	short _x;
	short _y;
	char _hp;

	char _packetState;
	char _playerState;
};

