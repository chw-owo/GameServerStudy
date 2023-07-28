#include "SystemLog.h"

int main()
{
    SYSLOG_DIRECTORY(L"Log_MatchmakingServer");
    SYSLOG_LEVEL(SystemLog::ERROR_LEVEL);
}
