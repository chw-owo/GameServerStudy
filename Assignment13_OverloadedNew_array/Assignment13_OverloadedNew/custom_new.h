#pragma once

void* operator new(size_t size, const char* file, int line);
void* operator new[](size_t size, const char* file, int line);
#define new new(__FILE__, __LINE__)		

void operator delete (void* p, char* file, int line);
void operator delete[](void* p, char* file, int line);
void operator delete (void* p);
void operator delete[](void* p);

