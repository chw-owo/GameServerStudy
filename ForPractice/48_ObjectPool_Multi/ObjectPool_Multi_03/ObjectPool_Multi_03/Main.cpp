#include "PoolTest.h"

int main()
{
    ProfileTlsLockPool();
    ProfileTlsLockPoolUpgrade();
    
    /*
    SetTest();
    SingleNewDelete();
    SingleBasicLockPool();
    SingleLockFreePool();
    SingleTlsLockPool();
    SingleTlsLockPoolUpgrade();

    SetMultiTest();
    MultiNewDelete();
    MultiBasicLockPool();
    MultiLockFreePool();
    MultiTlsLockPool();
    MultiTlsLockPoolUpgrade();
    */

    return 0;
}

