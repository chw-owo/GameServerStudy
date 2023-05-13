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
	_packet (0b00000000), _state(0b00000001)
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

void Player::OnCollision(Player* player)
{

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
			if (!HandlePacketMoveStart(header.Size)) return;
			break;

		case dfPACKET_CS_MOVE_STOP:
			if (!HandlePacketMoveStop(header.Size)) return;
			break;

		case dfPACKET_CS_ATTACK1:
			if (!HandlePacketAttack1(header.Size)) return;
			break;

		case dfPACKET_CS_ATTACK2:	
			if (!HandlePacketAttack2(header.Size)) return;
			break;

		case dfPACKET_CS_ATTACK3:
			if (!HandlePacketAttack3(header.Size)) return;
			break;
		}

		useSize = _pSession->_recvBuf.GetUseSize();
	}
}

bool Player::HandlePacketMoveStart(int packetSize)
{
	stPACKET_CS_MOVE_START packetMoveStart;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetMoveStart, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return false;
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
		return false;
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

	return true;
}

bool Player::HandlePacketMoveStop(int packetSize)
{
	stPACKET_CS_MOVE_STOP packetMoveStop;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetMoveStop, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return false;
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
		return false;
	}

	_direction = packetMoveStop.Direction;
	_x = packetMoveStop.X;
	_y = packetMoveStop.Y;
	SetPacketMoveStop();
	SetStateMoveStop();

	return true;
}

bool Player::HandlePacketAttack1(int packetSize)
{
	stPACKET_CS_ATTACK1 packetAttack1;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetAttack1, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return false;
	}

	if (abs(packetAttack1.X - _x) > dfERROR_RANGE ||
		abs(packetAttack1.Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return false;
	}
	return true;
}

bool Player::HandlePacketAttack2(int packetSize)
{
	stPACKET_CS_ATTACK2 packetAttack2;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetAttack2, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return false;
	}

	if (abs(packetAttack2.X - _x) > dfERROR_RANGE ||
		abs(packetAttack2.Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return false;
	}

	return true;
}

bool Player::HandlePacketAttack3(int packetSize)
{
	stPACKET_CS_ATTACK3 packetAttack3;
	int dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetAttack3, packetSize);
	if (dequeueRet != packetSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return false;
	}

	if (abs(packetAttack3.X - _x) > dfERROR_RANGE ||
		abs(packetAttack3.Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return false;
	}

	return true;
}