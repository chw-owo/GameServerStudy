#include "CLanRecvPacket.h"
#include "CLanMsg.h"

#ifdef LANSERVER
CObjectPool<CLanRecvPacket> CLanRecvPacket::_pool = CObjectPool<CLanRecvPacket>(dfPACKET_DEF, false);

CLanRecvPacket* CLanRecvPacket::Alloc()
{
	CLanRecvPacket* packet = CLanRecvPacket::_pool.Alloc();
	packet->Clear();
	return packet;
}

bool CLanRecvPacket::Free(CLanRecvPacket* packet)
{
	if (InterlockedDecrement(&packet->_usageCount) == 0)
	{

		AcquireSRWLockExclusive(&packet->_lock);
		while (packet->_msgs.size() > 0)
		{
			CLanMsg* msg = packet->_msgs.top();
			packet->_msgs.pop();
			CLanMsg::_pool.Free(msg);
		}
		ReleaseSRWLockExclusive(&packet->_lock);

		CLanRecvPacket::_pool.Free(packet);
		return true;
	}
	return false;
}

#endif