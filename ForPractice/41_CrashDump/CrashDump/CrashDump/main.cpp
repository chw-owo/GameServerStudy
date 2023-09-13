#include "stdio.h"
#include "CCrashDump.h"

CCrashDump dump;
int main()
{
    int a = 100;
    for(;;)
    {
        a--;
        printf("%d\n", 100000 / a);
    }
}
