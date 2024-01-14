#include "CNetMsg.h"

#ifdef NETSERVER
CTlsPool<CNetMsg> CNetMsg::_pool = CTlsPool<CNetMsg>(dfRCV_PACKET_DEF, true);
#endif