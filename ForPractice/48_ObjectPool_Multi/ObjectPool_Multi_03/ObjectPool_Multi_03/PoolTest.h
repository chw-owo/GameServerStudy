#pragma once
#define BLOCK 8

void ProfileTlsLockPool();
void ProfileTlsLockPoolUpgrade();

void SetTest();
void SingleNewDelete();
void SingleBasicLockPool();
void SingleLockFreePool();
void SingleTlsLockPool();
void SingleTlsLockPoolUpgrade();

void SetMultiTest();
void MultiNewDelete();
void MultiBasicLockPool();
void MultiLockFreePool();
void MultiTlsLockPool();
void MultiTlsLockPoolUpgrade();