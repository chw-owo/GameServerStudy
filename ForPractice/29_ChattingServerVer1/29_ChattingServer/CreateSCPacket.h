#pragma once
#include "Protocol.h"

void CreatePacket_HEADER(st_PACKET_HEADER& header, BYTE checkSum, WORD Type, WORD Size);