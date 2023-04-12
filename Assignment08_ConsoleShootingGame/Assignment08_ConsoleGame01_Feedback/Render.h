#pragma once
#include "Console.h"

#define DATA_SIZE 2048

void rd_BufferFlip(void);
void rd_BufferClear(void);
void rd_SpriteDraw(int iX, int iY, char chSprite);
void rd_DataToBuffer(const char* data);