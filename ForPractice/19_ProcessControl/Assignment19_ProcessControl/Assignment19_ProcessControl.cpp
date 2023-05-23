#include <iostream>
#include <Windows.h>

void ProcessControl()
{
    int processID;
    printf("프로세스 ID를 입력하세요: ");
    scanf_s("%d", & processID);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    
    int iOriginData;
    printf("\n찾고자 하는 데이터를 입력하세요: ");
    scanf_s("%d", &iOriginData);
    char chOriginData[4];
    int flag = 0x000000ff;

    for (int i = 0; i < 4; i++)
    {
        chOriginData[3 - i] = ((iOriginData >> (i * 8)) & flag);
    }

    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    char* pMemoryPos = (char*)SystemInfo.lpMinimumApplicationAddress;
    char* pMemoryPosOld = 0;
    char* pTargetPos = nullptr;

    printf("\n데이터를 탐색합니다");
    while (pMemoryPos > pMemoryPosOld)
    {
        printf("\n%u ~ %u", pMemoryPosOld, pMemoryPos);
        MEMORY_BASIC_INFORMATION MemoryBasicInfo;
        SIZE_T ret = VirtualQueryEx(hProcess, pMemoryPos,
            &MemoryBasicInfo, sizeof(MemoryBasicInfo));

        int size = MemoryBasicInfo.RegionSize;

        if (MemoryBasicInfo.Type == MEM_PRIVATE &&
            MemoryBasicInfo.State == MEM_COMMIT)
        {
            char* buffer = (char*) malloc(size + 1);
            memset(buffer, 0, size + 1);
            ReadProcessMemory(hProcess, pMemoryPos, buffer, size, NULL);
            buffer[size] = '\0';
            pTargetPos = strstr(buffer, chOriginData);

            if (pTargetPos != nullptr)
            {
                int newData;
                printf("\n데이터를 찾았습니다! 바꿀 값을 입력하세요: ");
                scanf_s("%d", &newData);
                WriteProcessMemory(hProcess, pTargetPos, &newData, 4, NULL);
                printf("\n데이터를 바꿨습니다! 탐색을 더 진행합니다.");
            }  

            free(buffer);
        }
        pMemoryPosOld = pMemoryPos;
        pMemoryPos += size;
    }

    if(pTargetPos == nullptr)
        printf("\n해당 데이터를 찾지 못했습니다!");
    printf("\n데이터 탐색이 끝났습니다.\n");
}

int main()
{
    ProcessControl();
}

