#include "Player.h"
#include "NetworkManager.h"
#include "PacketDefine.h"
#include <iostream>

Player::Player(Session* pSession, uint32 ID,
	uint8 direction, uint16 x, uint16 y, uint8 hp)
	: _pSession(pSession), _ID(ID), _direction(direction),_x(x), _y(y), _hp(hp)
{

}

Player::~Player()
{

}

void Player::Update()
{
	if (_move)
	{
		// ¿òÁ÷¿©¾ßµÊ
		// º®¿¡ ´êÀ¸¸é ¸ØÃç¾ß µÊ
		// BroadCastµµ ÇØ¾ß µÊ
		// ³»ÀÏ ...
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
			_pSession->_disconnect = true;
			return;
		}

		if (useSize < HEADER_LEN + header.Size)
			break;

		int moveReadRet = _pSession->_recvBuf.MoveReadPos(HEADER_LEN);
		if (moveReadRet != HEADER_LEN)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			_pSession->_disconnect = true;
			return;
		}

		int dequeueRet;

		switch (header.Type)
		{
		case dfPACKET_CS_MOVE_START:

			stPACKET_CS_MOVE_START packetMoveStart;
			dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetMoveStart, header.Size);
			if (dequeueRet != header.Size)
			{
				printf("Error! Func %s Line %d\n", __func__, __LINE__);
				_pSession->_disconnect = true;
				return;
			}

			_direction = packetMoveStart.Direction;
			_x = packetMoveStart.X;
			_y = packetMoveStart.Y;
			_move = true;

			break;

		case dfPACKET_CS_MOVE_STOP:

			stPACKET_CS_MOVE_STOP packetMoveStop;
			dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetMoveStop, header.Size);
			if (dequeueRet != header.Size)
			{
				printf("Error! Func %s Line %d\n", __func__, __LINE__);
				_pSession->_disconnect = true;
				return;
			}

			_direction = packetMoveStop.Direction;
			_x = packetMoveStop.X;
			_y = packetMoveStop.Y;
			_move = false;

			break;

		case dfPACKET_CS_ATTACK1:
			stPACKET_CS_ATTACK1 packetAttack1;
			dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetAttack1, header.Size);
			if (dequeueRet != header.Size)
			{
				printf("Error! Func %s Line %d\n", __func__, __LINE__);
				_pSession->_disconnect = true;
				return;
			}
			break;

		case dfPACKET_CS_ATTACK2:
			stPACKET_CS_ATTACK2 packetAttack2;
			dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetAttack2, header.Size);
			if (dequeueRet != header.Size)
			{
				printf("Error! Func %s Line %d\n", __func__, __LINE__);
				_pSession->_disconnect = true;
				return;
			}
			break;

		case dfPACKET_CS_ATTACK3:
			stPACKET_CS_ATTACK3 packetAttack3;
			dequeueRet = _pSession->_recvBuf.Dequeue((char*)&packetAttack3, header.Size);
			if (dequeueRet != header.Size)
			{
				printf("Error! Func %s Line %d\n", __func__, __LINE__);
				_pSession->_disconnect = true;
				return;
			}
			break;
		}

		useSize = _pSession->_recvBuf.GetUseSize();
	}
}

