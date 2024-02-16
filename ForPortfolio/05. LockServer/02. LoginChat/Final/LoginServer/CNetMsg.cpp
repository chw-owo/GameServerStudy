#include "CNetMsg.h"

#ifdef NETSERVER
CObjectPool<CNetMsg> CNetMsg::_pool = CObjectPool<CNetMsg>(dfRCV_PACKET_DEF, true);
#endif