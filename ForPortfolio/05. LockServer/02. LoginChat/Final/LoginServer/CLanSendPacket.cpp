#include "CLanSendPacket.h"

#ifdef LANSERVER
CObjectPool<CLanSendPacket> CLanSendPacket::_pool = CObjectPool<CLanSendPacket>(dfPACKET_DEF, false);
#endif