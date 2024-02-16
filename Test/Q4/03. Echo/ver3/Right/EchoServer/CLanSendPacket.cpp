#include "CLanSendPacket.h"

#ifdef LANSERVER
CTlsPool<CLanSendPacket> CLanSendPacket::_pool = CTlsPool<CLanSendPacket>(dfSNDPACKET_CNT_DEF, dfSNDPACKETPOOL_BUCKET_SIZE, false);
#endif