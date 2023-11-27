#pragma once
#define BLOCK 40
void SetMultiTest();

void NewDeletelTest();
void TlsLockPoolTest();
void TlsLockPoolUpgradeTest();
void TlsLockPoolUpgrade_SingleTest();

unsigned __stdcall NewDeletelTestThread(void* arg);
unsigned __stdcall TlsLockPoolTestThread(void* arg);
unsigned __stdcall TlsLockPoolUpgradeTestThread(void* arg);

/*
8~1024 8씩 올리기
*/