#include "CLanPacket.h"

#ifdef LANSERVER
CTlsPool<CLanPacket> CLanPacket::_pool = CTlsPool<CLanPacket>(dfPACKET_DEF, false);
#endif