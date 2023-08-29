#include "stdio.h"
#include "CCrashDump.h"

CCrashDump dump;
int main()
{
    int a = 100;
    while(1)
    {
        a--;
        if (a == 0) dump.Crash();
        printf("%d\n", 100000 / a);
    }
}
