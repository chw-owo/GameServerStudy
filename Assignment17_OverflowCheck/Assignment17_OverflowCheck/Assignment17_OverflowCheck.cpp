#include <iostream>
#include <Windows.h>
#define PAGE 4096

void* Alloc_OverflowCheck(int size, int align)
{
    int allocPageSize = (((size + 1) + 0x0fff) & (~0x0fff));

    char* pReadWrite = (char*)VirtualAlloc(
        NULL, allocPageSize + PAGE, MEM_COMMIT, PAGE_READWRITE);

    char* pNoAccess = (char*)VirtualAlloc(
        pReadWrite + allocPageSize, PAGE, MEM_COMMIT, PAGE_NOACCESS);

    *pReadWrite = 'Z';

    return pReadWrite + allocPageSize - size;
}

bool Free_OverflowCheck(char* p)
{
    unsigned __int64 mask = 0x0fff;
    char* checkPtr = (char*)((unsigned __int64)p & ~mask);

    if (*checkPtr == 'Z')
    {
        VirtualFree(checkPtr, 0, MEM_RELEASE);
        return true;
    }
    
    return false;    
}

int main()
{
    char* p = (char*)Alloc_OverflowCheck(400, 0);
    for (int i = 0; i < 400; i++)
    {
        p[i] = 'a';
    }
    Free_OverflowCheck(p);
}
