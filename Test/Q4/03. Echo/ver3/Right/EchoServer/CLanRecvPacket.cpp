#include "CLanRecvPacket.h"

#ifdef LANSERVER
CTlsPool<CLanRecvPacket> CLanRecvPacket::_pool = CTlsPool<CLanRecvPacket>(dfRCVPACKET_CNT_DEF, dfRCVPACKETPOOL_BUCKET_SIZE, false);
#endif