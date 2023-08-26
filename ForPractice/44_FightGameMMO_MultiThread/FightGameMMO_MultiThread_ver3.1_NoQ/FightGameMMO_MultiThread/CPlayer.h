#pragma once
#include "CSystemLog.h"
#include "CSector.h"
#include <tchar.h>

enum PLAYER
{
	ALIVE,
	DEAD,
	DELETED
};

// Packet State Flag
#define dfPACKET_MOVE_START		0b00000001
#define dfPACKET_MOVE_STOP		0b00000010
#define dfPACKET_ATTACK1		0b00000100
#define dfPACKET_ATTACK2		0b00001000
#define dfPACKET_ATTACK3		0b00010000
#define dfPACKET_DAMAGED		0b00100000
#define dfPACKET_ECHO			0b01000000
#define dfPACKET_SYNC			0b10000000

// Player State Flag
#define dfPLAYER_ALIVE			0b00000001
#define dfPLAYER_MOVE			0b00000010
#define dfPLAYER_DELETED		0b00000100

class CPlayer
{
public:
	inline CPlayer(int sessionID, int playerID)
		: _playerState(0), _packetState(0),
		_playerID(playerID), _sessionID(sessionID), _pSector(nullptr),
		_hp(dfMAX_HP), _direction(dfMOVE_DIR_LL), _moveDirection(dfMOVE_DIR_LL)
	{
		_x = rand() % dfRANGE_MOVE_RIGHT;
		_y = rand() % dfRANGE_MOVE_BOTTOM;
		_lastRecvTime = GetTickCount64();
	}

	inline CPlayer()
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
	}

public:
	char _packetState;
	char _playerState;
	int _playerID;
	__int64 _sessionID;
		
	short _x;
	short _y;
	char _hp;
	bool _move;	
	unsigned char _direction;
	unsigned char _moveDirection;

	int _time;
	int _attackerID;
	CSector* _pSector;
	DWORD _lastRecvTime;

public:
	bool GetPacketMoveStart()	{ return (_packetState & dfPACKET_MOVE_START); }
	bool GetPacketMoveStop()	{ return (_packetState & dfPACKET_MOVE_STOP); }
	bool GetPacketAttack1()		{ return (_packetState & dfPACKET_ATTACK1); }
	bool GetPacketAttack2()		{ return (_packetState & dfPACKET_ATTACK2); }
	bool GetPacketAttack3()		{ return (_packetState & dfPACKET_ATTACK3); }
	bool GetStateDamaged()		{ return (_packetState & dfPACKET_DAMAGED); }
	bool GetPacketSync()		{ return (_packetState & dfPACKET_SYNC); }
	bool GetPacketEcho()		{ return (_packetState & dfPACKET_ECHO); }

	bool GetStateAlive()		{ return (_playerState & dfPLAYER_ALIVE); }
	bool GetStateDead()			{ return !(_playerState & dfPLAYER_ALIVE); }
	bool GetStateMove()			{ return (_playerState & dfPLAYER_MOVE); }
	bool GetStateDeleted()		{ return (_playerState & dfPLAYER_DELETED); }

	void ResetPacketMoveStart() { _packetState ^= dfPACKET_MOVE_START; }
	void ResetPacketMoveStop()	{ _packetState ^= dfPACKET_MOVE_STOP; }
	void ResetPacketAttack1()	{ _packetState ^= dfPACKET_ATTACK1; }
	void ResetPacketAttack2()	{ _packetState ^= dfPACKET_ATTACK2; }
	void ResetPacketAttack3()	{ _packetState ^= dfPACKET_ATTACK3; }
	void ResetPacketDamaged()	{ _packetState ^= dfPACKET_DAMAGED; }
	void ResetPacketSync()		{ _packetState ^= dfPACKET_SYNC; }
	void ResetPacketEcho()		{ _packetState ^= dfPACKET_ECHO; }

	void SetPacketMoveStart()	{ _packetState |= dfPACKET_MOVE_START; }
	void SetPacketMoveStop()	{ _packetState |= dfPACKET_MOVE_STOP; }
	void SetPacketAttack1()		{ _packetState |= dfPACKET_ATTACK1; }
	void SetPacketAttack2()		{ _packetState |= dfPACKET_ATTACK2; }
	void SetPacketAttack3()		{ _packetState |= dfPACKET_ATTACK3; }
	void SetPacketDamaged()		{ _packetState |= dfPACKET_DAMAGED; }
	void SetPacketSync()		{ _packetState |= dfPACKET_SYNC; }
	void SetPacketEcho()		{ _packetState |= dfPACKET_ECHO; }

	void SetStateAlive()		{ _playerState |= dfPLAYER_ALIVE; }
	void SetStateDead()			{ _playerState ^= dfPLAYER_ALIVE; }
	void SetStateMoveStart()	{ _playerState |= dfPLAYER_MOVE; }
	void SetStateMoveStop()		{ _playerState ^= dfPLAYER_MOVE; }
	void SetStateDeleted()		{ _playerState |= dfPLAYER_DELETED; }
};
