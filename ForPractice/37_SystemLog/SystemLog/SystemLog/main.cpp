#include "SystemLog.h"

int main()
{
    SYSLOG_DIRECTORY(L"LogTest");
    SYSLOG_LEVEL(SystemLog::ERROR_LEVEL);
    char test[5] = { 1,2,3,4,5 };
    LOGHEX(L"loghex", SystemLog::ERROR_LEVEL, L"aaa", (BYTE*)test, 5);
    LOG(L"debug", SystemLog::DEBUG_LEVEL, L"%d, %d, %d: %s", 1, 2, 3, L"Yeah~~");
    LOG(L"error", SystemLog::ERROR_LEVEL, L"%d, %d, %d: %s", 4, 5, 6, L"Yeah!!");
    LOG(L"system", SystemLog::SYSTEM_LEVEL, L"%d, %d, %d: %s", 7, 8, 9, L"Yeah..");
}
