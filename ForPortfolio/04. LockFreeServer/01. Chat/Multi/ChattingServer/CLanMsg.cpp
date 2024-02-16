#include "CLanMsg.h"

#ifdef LANSERVER
CTlsPool<CLanMsg> CLanMsg::_pool = CTlsPool<CLanMsg>(dfRCV_PACKET_DEF, true);
#endif