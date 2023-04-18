#include <iostream>
#include <Windows.h>

void ProcessControl()
{
    int processID;
    printf("프로세스 ID를 입력하세요: ");
    scanf_s("%d", & processID);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    
    int originData;
    printf("\n찾고자 하는 데이터를 입력하세요: ");
    scanf_s("%d", &originData);

    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    char* pMemoryPos = (char*)SystemInfo.lpMinimumApplicationAddress;
    char* pMemoryPosOld = pMemoryPos;
    char* pTargetPos = nullptr;

    while (pMemoryPos >= pMemoryPosOld)
    {
        printf("%u\n", pMemoryPos);

        MEMORY_BASIC_INFORMATION MemoryBasicInfo;
        SIZE_T ret = VirtualQueryEx(hProcess, pMemoryPos,
            &MemoryBasicInfo, sizeof(MemoryBasicInfo));

        int size = MemoryBasicInfo.RegionSize;
        if (MemoryBasicInfo.Type == MEM_PRIVATE &&
            MemoryBasicInfo.State == MEM_COMMIT)
        {
            int counted = 0;
            char buf[5];
            char* ptr = pMemoryPos;
           
            while (counted < size)
            {
                memset(buf, 0, 4);
                ReadProcessMemory(hProcess, ptr, buf, 4, NULL);
                buf[4] = '\0';
                if (*buf == (char)originData) // To-do !!!!!!
                {
                    int newData;
                    pTargetPos = (char*)((int)MemoryBasicInfo.BaseAddress + counted);
                    printf("\n데이터를 찾았습니다! 바꿀 값을 입력하세요: ");
                    scanf_s("%d", &newData);
                    WriteProcessMemory(hProcess, pTargetPos, &newData, 4, NULL);
                    printf("\n데이터를 바꿨습니다! 탐색을 더 진행합니다.\n");
                }
                ptr += 4;
                counted += 4;
            }          
        }
        pMemoryPosOld = pMemoryPos;
        pMemoryPos += size;
    }
    if(pTargetPos == nullptr)
        printf("해당 데이터를 찾지 못했습니다!\n");
    printf("데이터 탐색이 끝났습니다.\n");
}

int main()
{
    ProcessControl();
}

