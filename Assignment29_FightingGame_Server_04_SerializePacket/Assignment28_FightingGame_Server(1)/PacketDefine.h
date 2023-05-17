#pragma once
#include "Typedef.h"

#define	dfPACKET_SC_CREATE_MY_CHARACTER			0
#define	dfPACKET_SC_CREATE_OTHER_CHARACTER		1
#define	dfPACKET_SC_DELETE_CHARACTER			2

#define	dfPACKET_CS_MOVE_START					10
#define	dfPACKET_SC_MOVE_START					11
#define	dfPACKET_CS_MOVE_STOP					12
#define	dfPACKET_SC_MOVE_STOP					13

#define	dfPACKET_CS_ATTACK1						20
#define	dfPACKET_SC_ATTACK1						21
#define	dfPACKET_CS_ATTACK2						22
#define	dfPACKET_SC_ATTACK2						23
#define	dfPACKET_CS_ATTACK3						24
#define	dfPACKET_SC_ATTACK3						25

#define	dfPACKET_SC_DAMAGE						30

#define	dfPACKET_CS_SYNC						250
#define	dfPACKET_SC_SYNC						251

#define dfPACKET_HEADER_CODE					0x89
#pragma pack (push, 1)
#define HEADER_SIZE sizeof(stPACKET_HEADER)

struct stPACKET_HEADER
{
	uint8	Code = dfPACKET_HEADER_CODE;
	uint8	Size;
	uint8	Type;
};
#pragma pack (pop)