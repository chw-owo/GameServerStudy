#include "CNetMsg.h"

#ifdef NETSERVER
CTlsPool<CNetMsg> CNetMsg::_pool = CTlsPool<CNetMsg>(dfMSG_CNT_DEF, dfMSGPOOL_BUCEKT_SIZE, true);
#endif