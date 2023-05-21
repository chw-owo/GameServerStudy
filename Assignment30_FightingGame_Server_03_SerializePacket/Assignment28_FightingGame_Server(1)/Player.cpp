#include "Player.h"
#include "PacketDefine.h"
#include "NetworkManager.h"
#include <stdio.h>

Player::Player(Session* pSession, int ID)
	: _pSession(pSession), _ID(ID), 
	_direction(dfPACKET_MOVE_DIR_LL), 
	_moveDirection(dfPACKET_MOVE_DIR_LL), 
	_x(dfINIT_X), _y(dfINIT_Y), _hp(dfMAX_HP), 
	_packetState (0), _state(dfPLAYER_STATE_ALIVE)
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
		if (useSize <= HEADER_SIZE)
			break;

		stPACKET_HEADER header;
		int peekRet = _pSession->_recvBuf.Peek((char*)&header, HEADER_SIZE);
		if (peekRet != HEADER_SIZE)
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

		if (useSize < HEADER_SIZE + header.Size)
			break;

		int moveReadRet = _pSession->_recvBuf.MoveReadPos(HEADER_SIZE);
		if (moveReadRet != HEADER_SIZE)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			SetStateDead();
			return;
		}

		_packetBuffer.Clear();

		switch (header.Type)
		{
		case dfPACKET_CS_MOVE_START:
			HandlePacketMoveStart();
			break;

		case dfPACKET_CS_MOVE_STOP:
			HandlePacketMoveStop();
			break;

		case dfPACKET_CS_ATTACK1:
			HandlePacketAttack1();
			break;

		case dfPACKET_CS_ATTACK2:	
			HandlePacketAttack2();
			break;

		case dfPACKET_CS_ATTACK3:
			HandlePacketAttack3();
			break;
		}

		if (_packetBuffer.GetReadPtr() == _packetBuffer.GetWritePtr())
			printf("Good~~ Func %s, Line %d\n", __func__, __LINE__);
		else
			printf("No!!! Func %s, Line %d\n", __func__, __LINE__);

		useSize = _pSession->_recvBuf.GetUseSize();
	}
}

void Player::Attacked(char damage)
{
	_hp -= damage;
	if (_hp <= 0)
	{
		_hp = 0;
		SetStateDead();
	}
}


void Player::HandlePacketMoveStart()
{
	char Direction;
	short X;
	short Y;

	int size = sizeof(Direction) + sizeof(X) + sizeof(Y);
	int dequeueRet = _pSession->_recvBuf.Dequeue(_packetBuffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}
	_packetBuffer.MoveWritePos(size);

	_packetBuffer >> Direction;
	_packetBuffer >> X;
	_packetBuffer >> Y;

	printf("===================================\n\
%d: MOVE START\n\n\
packetMoveStart.Direction: %d\n\
packetMoveStart.X: %d\n\
packetMoveStart.Y: %d\n\n\
now moveDirection: %d\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
	_ID, Direction, X, Y, _moveDirection, _x, _y);

	if (abs(X - _x) > dfERROR_RANGE ||
		abs(Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_moveDirection = Direction;
	switch (Direction)
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

	_x = X;
	_y = Y;
	SetPacketMoveStart();
	SetStateMoveStart();
}

void Player::HandlePacketMoveStop()
{
	char Direction;
	short X;
	short Y;

	int size = sizeof(Direction) + sizeof(X) + sizeof(Y);
	int dequeueRet = _pSession->_recvBuf.Dequeue(_packetBuffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}
	 _packetBuffer.MoveWritePos(size);

	 _packetBuffer >> Direction;
	 _packetBuffer >> X;
	 _packetBuffer >> Y;

	printf("===================================\n\
%d: MOVE STOP\n\n\
packetMoveStop.Direction: %d\n\
packetMoveStop.X: %d\n\
packetMoveStop.Y: %d\n\n\
now moveDirection: %d\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
	_ID, Direction, X, Y, _moveDirection, _x, _y);


	if (abs(X - _x) > dfERROR_RANGE ||
		abs(Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_direction = Direction;
	_x = X;
	_y = Y;

	SetPacketMoveStop();
	SetStateMoveStop();
}

void Player::HandlePacketAttack1()
{
	char Direction;
	short X;
	short Y;

	int size = sizeof(Direction) + sizeof(X) + sizeof(Y);
	int dequeueRet = _pSession->_recvBuf.Dequeue( _packetBuffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}
	 _packetBuffer.MoveWritePos(size);

	 _packetBuffer >> Direction;
	 _packetBuffer >> X;
	 _packetBuffer >> Y;

	printf("===================================\n\
%d: ATTACK 1\n\n\
packetAttack1.Direction: %d\n\
packetAttack1.X: %d\n\
packetAttack1.Y: %d\n\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
	_ID, Direction, X, Y, _x, _y);

	if (abs(X - _x) > dfERROR_RANGE ||
		abs(Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_direction = Direction;
	_x = X;
	_y = Y;
	SetPacketAttack1();
}

void Player::HandlePacketAttack2()
{
	char Direction;
	short X;
	short Y;

	int size = sizeof(Direction) + sizeof(X) + sizeof(Y);
	int dequeueRet = _pSession->_recvBuf.Dequeue( _packetBuffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}
	 _packetBuffer.MoveWritePos(size);

	 _packetBuffer >> Direction;
	 _packetBuffer >> X;
	 _packetBuffer >> Y;

	printf("===================================\n\
%d: ATTACK 2\n\n\
packetAttack2.Direction: %d\n\
packetAttack2.X: %d\n\
packetAttack2.Y: %d\n\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
	_ID, Direction, X, Y, _x, _y);

	if (abs(X - _x) > dfERROR_RANGE ||
		abs(Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_direction = Direction;
	_x = X;
	_y = Y;
	SetPacketAttack2();
}

void Player::HandlePacketAttack3()
{
	char Direction;
	short X;
	short Y;

	int size = sizeof(Direction) + sizeof(X) + sizeof(Y);
	int dequeueRet = _pSession->_recvBuf.Dequeue( _packetBuffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}
	 _packetBuffer.MoveWritePos(size);

	 _packetBuffer >> Direction;
	 _packetBuffer >> X;
	 _packetBuffer >> Y;

	printf("===================================\n\
%d: ATTACK 3\n\n\
packetAttack3.Direction: %d\n\
packetAttack3.X: %d\n\
packetAttack3.Y: %d\n\n\
now X: %d\n\
now Y: %d\n\
====================================\n\n",
	_ID, Direction, X, Y, _x, _y);

	if (abs(X - _x) > dfERROR_RANGE ||
		abs(Y - _y) > dfERROR_RANGE)
	{
		SetStateDead();
		return;
	}

	_direction = Direction;
	_x = X;
	_y = Y;
	SetPacketAttack3();
}