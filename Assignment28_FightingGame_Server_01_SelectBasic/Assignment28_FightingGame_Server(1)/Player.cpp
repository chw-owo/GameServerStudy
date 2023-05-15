#include "Player.h"
#include "PacketDefine.h"
#include "CreatePacket.h"
#include <iostream>
#include <stdlib.h>

Player::Player(Session* pSession, uint32 ID)
	: _pSession(pSession), _ID(ID), 
	_direction(dfPACKET_MOVE_DIR_LL), 
	_moveDirection(dfPACKET_MOVE_DIR_LL), 
	_x(dfINIT_X), _y(dfINIT_Y), _hp(dfMAX_HP), 
	_packet (0), _state(dfPLAYER_STATE_ALIVE)
{

}

Player::~Player()
{
	SetStateDead();
}

void Player::Update()
{
	if (_x < dfRANGE_MOVE_LEFT || _x > dfRANGE_MOVE_RIGHT ||
		_y > dfRANGE_MOVE_BOTTOM || _y < dfRANGE_MOVE_TOP)
	{

	}
	else if (GetStateMoving())
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

void Player::DequeueRecvBuf()
{
	int useSize = _pSession->_recvBuf.GetUseSize();
	while (useSize > 0)
	{
		if (useSize <= HEADER_LEN)
			break;

		stPACKET_HEADER header;
		int peekRet = _pSession->_recvBuf.Peek((char*)&header, HEADER_LEN);
		if (peekRet != HEADER_LEN)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			SetStateDead();
			return;
		}

		if (header.Code != dfPACKET_HEADER_CODE)
		{
			printf("Error! Wrong Header Code! - Func %s Line %d\n", __func__, __LINE__);
			SetStateDead();
			return;
		}

		if (useSize < HEADER_LEN + header.Size)
			break;

		int moveReadRet = _pSession->_recvBuf.MoveReadPos(HEADER_LEN);
		if (moveReadRet != HEADER_LEN)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			SetStateDead();
			return;
		}

		switch (header.Type)
		{
		case dfPACKET_CS_MOVE_START:
			HandlePacketMoveStart(header.Size);
			break;

		case dfPACKET_CS_MOVE_STOP:
			HandlePacketMoveStop(header.Size);
			break;

		case dfPACKET_CS_ATTACK1:
			HandlePacketAttack1(header.Size);
			break;

		case dfPACKET_CS_ATTACK2:	
			HandlePacketAttack2(header.Size);
			break;

		case dfPACKET_CS_ATTACK3:
			HandlePacketAttack3(header.Size);
			break;
		}

		useSize = _pSession->_recvBuf.GetUseSize();
	}
}

void Player::Attacked(uint8 damage)
{
	_hp -= damage;
	if (_hp <= 0)
	{
		_hp = 0;
		SetStateDead();
	}
}

void Player::HandlePacketMoveStart(int packetSize)
{
	stPACKET_CS_MOVE_START packetMoveStart;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetMoveStart, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}
	
	printf("===================================\n\
%d: MOVE START\n\n\
packetMoveStart.Direction: %d\n\
packetMoveStart.X: %d\n\
packetMoveStart.Y: %d\n\n\
now moveDirection: %d\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
		_ID, packetMoveStart.Direction, packetMoveStart.X, packetMoveStart.Y,
		_moveDirection, _x, _y);
		

	if (abs(packetMoveStart.X - _x) > dfERROR_RANGE ||
		abs(packetMoveStart.Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_moveDirection = packetMoveStart.Direction;
	switch (packetMoveStart.Direction)
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

	_x = packetMoveStart.X;
	_y = packetMoveStart.Y;
	SetPacketMoveStart();
	SetStateMoveStart();
}

void Player::HandlePacketMoveStop(int packetSize)
{
	stPACKET_CS_MOVE_STOP packetMoveStop;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetMoveStop, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}
	
	printf("===================================\n\
%d: MOVE STOP\n\n\
packetMoveStop.Direction: %d\n\
packetMoveStop.X: %d\n\
packetMoveStop.Y: %d\n\n\
now moveDirection: %d\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
		_ID, packetMoveStop.Direction, packetMoveStop.X, packetMoveStop.Y,
		_moveDirection, _x, _y);


	if (abs(packetMoveStop.X - _x) > dfERROR_RANGE ||
		abs(packetMoveStop.Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_direction = packetMoveStop.Direction;
	_x = packetMoveStop.X;
	_y = packetMoveStop.Y;
	SetPacketMoveStop();
	SetStateMoveStop();
}

void Player::HandlePacketAttack1(int packetSize)
{
	stPACKET_CS_ATTACK1 packetAttack1;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetAttack1, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}

	printf("===================================\n\
%d: ATTACK 1\n\n\
packetAttack1.Direction: %d\n\
packetAttack1.X: %d\n\
packetAttack1.Y: %d\n\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
	_ID, packetAttack1.Direction, packetAttack1.X, packetAttack1.Y, _x, _y);

	if (abs(packetAttack1.X - _x) > dfERROR_RANGE ||
		abs(packetAttack1.Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_direction = packetAttack1.Direction;
	_x = packetAttack1.X;
	_y = packetAttack1.Y;
	SetPacketAttack1();
}

void Player::HandlePacketAttack2(int packetSize)
{
	stPACKET_CS_ATTACK2 packetAttack2;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetAttack2, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}

	printf("===================================\n\
%d: ATTACK 2\n\n\
packetAttack2.Direction: %d\n\
packetAttack2.X: %d\n\
packetAttack2.Y: %d\n\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
	_ID, packetAttack2.Direction, packetAttack2.X, packetAttack2.Y, _x, _y);

	if (abs(packetAttack2.X - _x) > dfERROR_RANGE ||
		abs(packetAttack2.Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_direction = packetAttack2.Direction;
	_x = packetAttack2.X;
	_y = packetAttack2.Y;
	SetPacketAttack2();
}

void Player::HandlePacketAttack3(int packetSize)
{
	stPACKET_CS_ATTACK3 packetAttack3;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetAttack3, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}

	printf("===================================\n\
%d: ATTACK 3\n\n\
packetAttack3.Direction: %d\n\
packetAttack3.X: %d\n\
packetAttack3.Y: %d\n\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
	_ID, packetAttack3.Direction, packetAttack3.X, packetAttack3.Y, _x, _y);

	if (abs(packetAttack3.X - _x) > dfERROR_RANGE ||
		abs(packetAttack3.Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_direction = packetAttack3.Direction;
	_x = packetAttack3.X;
	_y = packetAttack3.Y;
	SetPacketAttack3();
}