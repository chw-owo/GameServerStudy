#include "CNetPacket.h"
#include "CRecvNetPacket.h"

#ifdef NETSERVER
CTlsPool<CNetPacket> CNetPacket::_pool = CTlsPool<CNetPacket>(dfPACKET_DEF, false);

CNetPacket* CNetPacket::Alloc()
{
	CNetPacket* packet = CNetPacket::_pool.Alloc();
	packet->Clear();
	return packet;
}

bool CNetPacket::Free(CNetPacket* packet)
{
	if (InterlockedDecrement(&packet->_usageCount) == 0)
	{
		while (packet->_recvPackets->GetUseSize() > 0)
		{
			CRecvNetPacket* recvPacket = packet->_recvPackets->Pop();
			if (recvPacket == nullptr) break;
			CRecvNetPacket::_pool.Free(recvPacket);
		}
		CNetPacket::_pool.Free(packet);
		return true;
	}
	return false;
}

#endif