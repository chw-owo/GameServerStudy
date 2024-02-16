#include "CLanMsg.h"

#ifdef LANSERVER
CTlsPool<CLanMsg> CLanMsg::_pool = CTlsPool<CLanMsg>(dfMSG_CNT_DEF, dfMSGPOOL_BUCEKT_SIZE, true);
#endif