#include <iostream>
#include "Parser.h"
#include <tchar.h>
int main()
{
    // Get txt Data
    txtParser txtP;
    INT version, serverId;
    txtP.LoadFile(_T("Server.txt"));
    txtP.GetValue(_T("Version"), version);
    txtP.GetValue(_T("ServerID"), serverId);
    _tprintf(_T("Version: %d, ServerID: %d"), version, serverId);


    // Get csv Data
    csvParser csvP;
    INT value;
    csvP.LoadFile(_T("Player.csv"));
    for (int i = 0; i < 10; i++)
    {
        csvP.GetColumn(value);
        _tprintf(_T("col%d's value: %d"), i, value);
        csvP.NextColumn();
    }
}
