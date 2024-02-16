#include "CNetRecvPacket.h"
#include "CNetMsg.h"

#ifdef NETSERVER
CObjectPool<CNetRecvPacket> CNetRecvPacket::_pool = CObjectPool<CNetRecvPacket>(dfPACKET_DEF, false);

CNetRecvPacket* CNetRecvPacket::Alloc()
{
	CNetRecvPacket* packet = CNetRecvPacket::_pool.Alloc();
	packet->Clear();
	return packet;
}

bool CNetRecvPacket::Free(CNetRecvPacket* packet)
{
	if (InterlockedDecrement(&packet->_usageCount) == 0)
	{
		AcquireSRWLockExclusive(&packet->_lock);
		while (packet->_msgs.size() > 0)
		{
			CNetMsg* msg = packet->_msgs.top();
			packet->_msgs.pop();
			CNetMsg::_pool.Free(msg);
		}
		ReleaseSRWLockExclusive(&packet->_lock);

		CNetRecvPacket::_pool.Free(packet);
		return true;
	}
	return false;
}

#endif