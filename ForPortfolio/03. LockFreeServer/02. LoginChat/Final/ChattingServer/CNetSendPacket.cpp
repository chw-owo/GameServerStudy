#include "CNetSendPacket.h"

#ifdef NETSERVER
CTlsPool<CNetSendPacket> CNetSendPacket::_pool = CTlsPool<CNetSendPacket>(dfPACKET_DEF, false);
#endif