#include "CNetSendPacket.h"

#ifdef NETSERVER
CObjectPool<CNetSendPacket> CNetSendPacket::_pool = CObjectPool<CNetSendPacket>(dfPACKET_DEF, false);
#endif