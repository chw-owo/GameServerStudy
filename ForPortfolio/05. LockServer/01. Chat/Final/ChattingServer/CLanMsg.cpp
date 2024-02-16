#include "CLanMsg.h"

#ifdef LANSERVER
CObjectPool<CLanMsg> CLanMsg::_pool = CObjectPool<CLanMsg>(dfRCV_PACKET_DEF, true);
#endif