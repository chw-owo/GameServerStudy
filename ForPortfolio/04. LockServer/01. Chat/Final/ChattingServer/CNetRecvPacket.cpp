#include "CNetRecvPacket.h"
#include "CNetMsg.h"

#ifdef NETSERVER
CTlsPool<CNetRecvPacket> CNetRecvPacket::_pool = CTlsPool<CNetRecvPacket>(dfPACKET_DEF, false);

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
		while (packet->_msgs->GetUseSize() > 0)
		{
			CNetMsg* msg = packet->_msgs->Pop();
			if (msg == nullptr) break;
			CNetMsg::_pool.Free(msg);
		}
		CNetRecvPacket::_pool.Free(packet);
		return true;
	}
	return false;
}

#endif