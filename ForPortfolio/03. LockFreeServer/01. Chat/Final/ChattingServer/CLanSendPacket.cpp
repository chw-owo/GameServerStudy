#include "CLanSendPacket.h"

#ifdef LANSERVER
CTlsPool<CLanSendPacket> CLanSendPacket::_pool = CTlsPool<CLanSendPacket>(dfPACKET_DEF, false);
#endif