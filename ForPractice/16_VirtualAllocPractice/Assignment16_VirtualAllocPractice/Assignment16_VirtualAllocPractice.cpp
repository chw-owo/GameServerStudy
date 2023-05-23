#include <iostream>
#include <Windows.h>

void GetMinMaxAddress()
{
    SYSTEM_INFO sys;
    GetSystemInfo(&sys);

    printf("min: %08x\n max: %08x\n",
        sys.lpMinimumApplicationAddress,
        sys.lpMaximumApplicationAddress);
}

void NullCheck(char* p)
{
    if (p == nullptr)
        printf("Error!!\n");
    else
        printf("Ok!!\n");
}

void VirtualAllocTest()
{
    // Ok
    char* p = (char*)VirtualAlloc(
        NULL, 4096 * 2, MEM_COMMIT, PAGE_READWRITE);
    NullCheck(p);
    
    /*
    // Error!!
    char* p = (char*)VirtualAlloc(
        (VOID*)0x00a00000, 4096 * 2, MEM_COMMIT, PAGE_READWRITE);
    NullCheck(p);

    // Error!!
    char* p1 = (char*)VirtualAlloc(
        p + 4096 * 2, 4096, MEM_RESERVE, PAGE_READWRITE);
    NullCheck(p1);

    // Error!!
    char* p2 = (char*)VirtualAlloc(
        p + 4096 * 2, 4096, MEM_COMMIT, PAGE_READWRITE);
    NullCheck(p2);*/

    VirtualFree(p, 0, MEM_RELEASE);
 
}

int main()
{    
    GetMinMaxAddress();
    VirtualAllocTest();
}
