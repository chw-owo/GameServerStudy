#pragma once

// #define LANSERVER
#define NETSERVER

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

#define dfSESSION_MAX				30000
#define dfPACKET_DEF				10000
#define dfPACKET_MAX				2048

#define dfWSARECVBUF_CNT			2
#define dfWSASENDBUF_CNT			100

#define dfRBUFFER_DEF_SIZE			8192
#define dfRBUFFER_MAX_SIZE			32768

#define dfSPACKET_DEF_SIZE			2048
#define dfSPACKET_MAX_SIZE			4096

#define dfMSG_MAX					256