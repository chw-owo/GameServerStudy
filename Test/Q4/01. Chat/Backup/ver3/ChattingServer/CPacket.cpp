#include "CPacket.h"

CTlsPool<CPacket> CPacket::_pool = CTlsPool<CPacket>(dfPACKET_DEF, false);