#pragma once

void* operator new(size_t size, const char* File, int Line);
void* operator new[](size_t size, const char* File, int Line);
#define new new(__FILE__, __LINE__)		

void operator delete (void* p, char* File, int Line);
void operator delete[](void* p, char* File, int Line);
void operator delete (void* p);
void operator delete[](void* p);

