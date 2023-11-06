#pragma once
#define BLOCK 256
void SetMultiTest();

void NewDeletelTest();
void TlsLockPoolTest();
void TlsLockPoolUpgradeTest();
void TlsLockPoolUpgrade_SingleTest();

unsigned __stdcall NewDeletelTestThread(void* arg);
unsigned __stdcall TlsLockPoolTestThread(void* arg);
unsigned __stdcall TlsLockPoolUpgradeTestThread(void* arg);