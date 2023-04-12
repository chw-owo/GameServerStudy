#include <iostream>
#include <Windows.h>
void ProcessControl()
{
    int processID;
    printf("프로세스 ID를 입력하세요: ");
    scanf_s("%d", & processID);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);

    int originData;
    printf("찾고자 하는 데이터를 입력하세요: ");
    scanf_s("%d", &originData);

    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    void* pMemoryPos = SystemInfo.lpMinimumApplicationAddress;

    while (pMemoryPos < SystemInfo.lpMaximumApplicationAddress)
    {
        char* buffer = (char*)calloc(4, '0');
        ReadProcessMemory(hProcess, pMemoryPos, buffer, 4, NULL);

        // To-do
        MEMORY_BASIC_INFORMATION MemoryBasicInfo;
        SIZE_T size = VirtualQueryEx(hProcess, pMemoryPos,
            &MemoryBasicInfo, sizeof(MemoryBasicInfo));

        if (MemoryBasicInfo.Type == MEM_PRIVATE &&
            MemoryBasicInfo.State == MEM_COMMIT)
        {
            //To-do
        }
    }

    int newData;
    printf("바꿀 데이터 값을 입력하세요: ");
    scanf_s("%d", &newData);
    WriteProcessMemory(hProcess, pMemoryPos, &newData, 4, NULL);

}
int main()
{
    ProcessControl();
}

