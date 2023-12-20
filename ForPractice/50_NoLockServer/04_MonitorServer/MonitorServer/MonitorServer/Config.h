#pragma once

#define LANSERVER
// #define NETSERVER

#ifdef LANSERVER
#pragma pack (push, 1)
struct stHeader
{
	unsigned short	_len;
};
#pragma pack (pop)
#define dfHEADER_LEN				sizeof(stHeader)
#endif

#ifdef NETSERVER

#define dfPACKET_CODE		0x77
#define dfPACKET_KEY		0x32

#pragma pack (push, 1)
struct stHeader
{
	unsigned char _code = dfPACKET_CODE;
	unsigned short _len;
	unsigned char _randKey;
	unsigned char _checkSum;
};
#pragma pack (pop)
#define dfHEADER_LEN				sizeof(stHeader)
#endif

#define dfSESSION_MAX				20000
#define dfPACKET_DEF				30000
#define dfRCV_PACKET_DEF			10000
#define dfJOB_DEF					10000

#define dfJOB_QUEUE_CNT				1
#define dfWSARECVBUF_CNT			1
#define dfWSASENDBUF_CNT			200

#define dfRBUFFER_DEF_SIZE			1024
#define dfRBUFFER_MAX_SIZE			16384

#define dfSPACKET_DEF_SIZE			256
#define dfSPACKET_MAX_SIZE			2048

#define dfERR_MAX					256