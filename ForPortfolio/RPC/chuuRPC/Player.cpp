#include "Player.h"
#include <stdlib.h>
#include <stdio.h>

Player::Player()
	: _pSession(nullptr), _ID(-1),
	_direction(dfPACKET_MOVE_DIR_LL),
	_moveDirection(dfPACKET_MOVE_DIR_LL),
	_x(dfINIT_X), _y(dfINIT_Y), _hp(dfMAX_HP),
	_packetState(0), _playerState(dfPLAYER_STATE_ALIVE)
{
	
}

Player::Player(Session* pSession, int ID)
	: _pSession(pSession), _ID(ID),
	_direction(dfPACKET_MOVE_DIR_LL),
	_moveDirection(dfPACKET_MOVE_DIR_LL),
	_x(dfINIT_X), _y(dfINIT_Y), _hp(dfMAX_HP),
	_packetState(0), _playerState(dfPLAYER_STATE_ALIVE)
{
	printf("Success to Create Player!\n");
}

Player::~Player()
{
	SetPlayerState_Dead();
}

void Player::Update()
{
	if (_x < dfRANGE_MOVE_LEFT || _x > dfRANGE_MOVE_RIGHT ||
		_y > dfRANGE_MOVE_BOTTOM || _y < dfRANGE_MOVE_TOP)
	{

	}
	else if (GetPlayerState_Moving())
	{
		switch (_moveDirection)
		{
		case dfPACKET_MOVE_DIR_LL:
			_x -= dfMOVE_X;
			break;

		case dfPACKET_MOVE_DIR_LU:
			_x -= dfMOVE_X;
			_y -= dfMOVE_Y;
			break;

		case dfPACKET_MOVE_DIR_UU:
			_y -= dfMOVE_Y;
			break;

		case dfPACKET_MOVE_DIR_RU:
			_x += dfMOVE_X;
			_y -= dfMOVE_Y;
			break;

		case dfPACKET_MOVE_DIR_RR:
			_x += dfMOVE_X;
			break;

		case dfPACKET_MOVE_DIR_RD:
			_x += dfMOVE_X;
			_y += dfMOVE_Y;
			break;

		case dfPACKET_MOVE_DIR_DD:
			_y += dfMOVE_Y;
			break;

		case dfPACKET_MOVE_DIR_LD:
			_x -= dfMOVE_X;
			_y += dfMOVE_Y;
			break;
		}
	}
}

void Player::Attacked(char damage)
{
	_hp -= damage;
	if (_hp <= 0)
	{
		_hp = 0;
		
		SetPlayerState_Dead();
	}
}


void Player::MoveStart(char direction, short x, short y)
{
	if (abs(x - _x) > dfERROR_RANGE ||
		abs(y - _y) > dfERROR_RANGE)
	{
		
		SetPlayerState_Dead();
		return;
	}

	_moveDirection = direction;

	switch (direction)
	{
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		_direction = dfPACKET_MOVE_DIR_LL;
		break;

	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		_direction = dfPACKET_MOVE_DIR_RR;
		break;
	}

	_x = x;
	_y = y;
	SetPacketState_MoveStart();
	SetPlayerState_MoveStart();
}

void Player::MoveStop(char direction, short x, short y)
{
	if (abs(x - _x) > dfERROR_RANGE ||
		abs(y - _y) > dfERROR_RANGE)
	{
		
		SetPlayerState_Dead();
		return;
	}

	_direction = direction;
	_x = x;
	_y = y;

	SetPacketState_MoveStop();
	SetPlayerState_MoveStop();
}

void Player::Attack1(char direction, short x, short y)
{
	if (abs(x - _x) > dfERROR_RANGE ||
		abs(y - _y) > dfERROR_RANGE)
	{
		
		SetPlayerState_Dead();
		return;
	}

	_direction = direction;
	_x = x;
	_y = y;

	SetPacketState_Attack1();
}

void Player::Attack2(char direction, short x, short y)
{
	if (abs(x - _x) > dfERROR_RANGE ||
		abs(y - _y) > dfERROR_RANGE)
	{
		
		SetPlayerState_Dead();
		return;
	}

	_direction = direction;
	_x = x;
	_y = y;

	SetPacketState_Attack2();
}

void Player::Attack3(char direction, short x, short y)
{
	if (abs(x - _x) > dfERROR_RANGE ||
		abs(y - _y) > dfERROR_RANGE)
	{
		
		SetPlayerState_Dead();
		return;
	}

	_direction = direction;
	_x = x;
	_y = y;

	SetPacketState_Attack3();
}