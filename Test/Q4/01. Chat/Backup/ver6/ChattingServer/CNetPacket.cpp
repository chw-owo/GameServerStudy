#include "CNetPacket.h"

#ifdef NETSERVER
CTlsPool<CNetPacket> CNetPacket::_pool = CTlsPool<CNetPacket>(dfPACKET_DEF, false);
#endif