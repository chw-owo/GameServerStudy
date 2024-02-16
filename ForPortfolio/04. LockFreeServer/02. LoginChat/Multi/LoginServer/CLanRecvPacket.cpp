#include "CLanRecvPacket.h"
#include "CLanMsg.h"

#ifdef LANSERVER
CTlsPool<CLanRecvPacket> CLanRecvPacket::_pool = CTlsPool<CLanRecvPacket>(dfPACKET_DEF, false);

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
		while (packet->_msgs->GetUseSize() > 0)
		{
			CLanMsg* msg = packet->_msgs->Pop();
			if (msg == nullptr) break;
			CLanMsg::_pool.Free(msg);
		}
		CLanRecvPacket::_pool.Free(packet);
		return true;
	}
	return false;
}

#endif