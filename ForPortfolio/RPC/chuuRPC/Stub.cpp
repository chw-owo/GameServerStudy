#include "Stub.h"
#include "Common.h"
#include "SerializePacket.h"
#include "Network.h"
#include <stdio.h>

void IStub::ProcessReceivedMessage(Session* pSession)
{
	// TO-DO: Get HostID
	// 일단은 인자로 받고 있는데 이거 말고~
	
	// Get Packet ID from Serialize Packet
	int useSize = pSession->_recvBuf.GetUseSize();
	while (useSize > 0)
	{
		if (useSize <= df_HEADER_SIZE)
			break;

		stHEADER header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, df_HEADER_SIZE);
		if (peekRet != df_HEADER_SIZE)
		{
			// TO-DO: Handle Exception (세션을 끊던가.. 로그를 남기던가..)
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			//SetStateDead();
			return;
		}

		if ((char)header.code != df_HEADER_CODE)
		{
			// TO-DO: Handle Exception (세션을 끊던가.. 로그를 남기던가..)
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			//SetStateDead();
			return;
		}

		if (useSize < df_HEADER_SIZE + header.size)
			break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(df_HEADER_SIZE);
		if (moveReadRet != df_HEADER_SIZE)
		{
			// TO-DO: Handle Exception (세션을 끊던가.. 로그를 남기던가..)
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			//SetStateDead();
			return;
		}

		switch (header.type)
		{
		case df_CS_MOVE_START:
			Handle_CS_MoveStart(pSession);
			break;

		case df_CS_MOVE_STOP:
			Handle_CS_MoveStop(pSession);
			break;

		case df_CS_ATTACK1:
			Handle_CS_Attack1(pSession);
			break;

		case df_CS_ATTACK2:
			Handle_CS_Attack2(pSession);
			break;

		case df_CS_ATTACK3:
			Handle_CS_Attack3(pSession);
			break;
		}
	}
}

void IStub::Handle_CS_MoveStart(Session* pSession)
{
	char direction;
	short x;
	short y;

	SerializePacket packet;
	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = pSession->_recvBuf.Dequeue(packet.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		// TO-DO: Handle Exception (세션을 끊던가.. 로그를 남기던가..)
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		//SetStateDead();
		return;
	}
	packet.MoveWritePos(size);

	packet >> direction >> x >> y;
	CS_MoveStart(pSession->GetSessionID(), direction, x, y);
}

void IStub::Handle_CS_MoveStop(Session* pSession)
{
}

void IStub::Handle_CS_Attack1(Session* pSession)
{
}

void IStub::Handle_CS_Attack2(Session* pSession)
{
}

void IStub::Handle_CS_Attack3(Session* pSession)
{
}
