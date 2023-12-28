#include "CRecvNetPacket.h"

#ifdef NETSERVER
CTlsPool<CRecvNetPacket> CRecvNetPacket::_pool = CTlsPool<CRecvNetPacket>(dfRCV_PACKET_DEF, true);
#endif