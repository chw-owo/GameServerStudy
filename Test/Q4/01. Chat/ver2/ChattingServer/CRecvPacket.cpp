#include "CRecvPacket.h"

CTlsPool<CRecvPacket> CRecvPacket::_pool = CTlsPool<CRecvPacket>(dfRCV_PACKET_DEF, true);