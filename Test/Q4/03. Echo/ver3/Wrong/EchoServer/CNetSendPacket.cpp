#include "CNetSendPacket.h"

#ifdef NETSERVER
CTlsPool<CNetSendPacket> CNetSendPacket::_pool = CTlsPool<CNetSendPacket>(dfSNDPACKET_CNT_DEF, dfSNDPACKETPOOL_BUCKET_SIZE, false);
#endif