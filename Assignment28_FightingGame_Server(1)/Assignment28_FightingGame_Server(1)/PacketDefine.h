#ifndef __PACKET_DEFINE__
#define __PACKET_DEFINE__
#include "Typedef.h"

// PACKET TYPE Define
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

#pragma pack (push, 1)
#define HEADER_LEN sizeof(stPACKET_HEADER)

struct stPACKET_HEADER
{
	uint8	Code = 0x89;
	uint8	Size;
	uint8	Type;
};

struct stPACKET_SC_CREATE_MY_CHARACTER
{
	uint32 ID;
	uint8 Direction; 
	uint16 X;
	uint16 Y;
	uint8 HP;
};

struct stPACKET_SC_CREATE_OTHER_CHARACTER
{
	uint32 ID;
	uint8 Direction;
	uint16 X;
	uint16 Y;
	uint8 HP;
};

struct stPACKET_SC_DELETE_CHARACTER
{
	uint32 ID;
};

struct stPACKET_CS_MOVE_START
{
	uint8 Direction;
	uint16 X;
	uint16 Y;
};

struct stPACKET_SC_MOVE_START
{
	uint32 ID;
	uint8 Direction; 
	uint16 X;
	uint16 Y;
};

struct stPACKET_CS_MOVE_STOP
{
	uint8 Direction;
	uint16 X;
	uint16 Y;
};

struct stPACKET_SC_MOVE_STOP
{
	uint32 ID;
	uint8 Direction;
	uint16 X;
	uint16 Y;
};

struct stPACKET_CS_ATTACK1
{
	uint8 Direction;
	uint16 X;
	uint16 Y;
};

struct stPACKET_SC_ATTACK1
{
	uint32 ID;
	uint8 Direction;
	uint16 X;
	uint16 Y;
};

struct stPACKET_CS_ATTACK2
{
	uint8 Direction;
	uint16 X;
	uint16 Y;
};

struct stPACKET_SC_ATTACK2
{
	uint32 ID;
	uint8 Direction;
	uint16 X;
	uint16 Y;
};

struct stPACKET_CS_ATTACK3
{
	uint8 Direction;
	uint16 X;
	uint16 Y;
};

struct stPACKET_SC_ATTACK3
{
	uint32 ID;
	uint8 Direction;
	uint16 X;
	uint16 Y;
};

struct stPACKET_SC_DAMAGE
{
	uint32 AttackID;
	uint32 DamageID;
	uint8 DamageHP;
};

struct stPACKET_CS_SYNC
{
	uint16 X;
	uint16 Y;
};

struct stPACKET_SC_SYNC
{
	uint16 X;
	uint16 Y;
};
#pragma pack(pop)
#endif


