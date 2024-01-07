#include "CRecvLanPacket.h"

#ifdef LANSERVER
CTlsPool<CRecvLanPacket> CRecvLanPacket::_pool = CTlsPool<CRecvLanPacket>(dfRCV_PACKET_DEF, true);
#endif