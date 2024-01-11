#pragma once

#define LANSERVER
#define NETSERVER

#ifdef LANSERVER
#pragma pack (push, 1)
struct stLanHeader
{
	unsigned short	_len;
};
#pragma pack (pop)
#define dfLANHEADER_LEN				sizeof(stLanHeader)
#endif

#ifdef NETSERVER

#define dfPACKET_CODE		0x77
#define dfPACKET_KEY		0x32

#pragma pack (push, 1)
struct stNetHeader
{
	unsigned char _code = dfPACKET_CODE;
	unsigned short _len;
	unsigned char _randKey;
	unsigned char _checkSum;
};
#pragma pack (pop)
#define dfNETHEADER_LEN					sizeof(stNetHeader)
#endif

#define dfSESSION_MAX					12000
#define dfADDRESS_LEN					50
#define dfWSARECVBUF_CNT				1
#define dfWSASENDBUF_CNT				200

#define dfRCVPACKET_SIZE_DEF			3000
#define dfSNDPACKET_SIZE_DEF			64
#define dfPACKET_DEF_SIZE				512
#define dfPACKET_MAX_SIZE				6000

#define dfRCVPACKET_CNT_DEF				12000
#define dfSNDPACKET_CNT_DEF				24000
#define dfMSG_CNT_DEF					24000

#define dfRCVPACKETPOOL_BUCKET_SIZE		3000
#define dfSNDPACKETPOOL_BUCKET_SIZE		6000
#define dfMSGPOOL_BUCEKT_SIZE			6000
#define dfPOOL_THREAD_MAX				20

#define dfSTACKNODE_CNT_DEF				100
#define dfQUEUENODE_CNT_DEF				100
#define dfSTACKNODE_BUCKET_SIZE			200
#define dfQUEUENODE_BUCKET_SIZE			200

#define dfJOB_CNT_DEF					0
#define dfJOBQ_CNT						0

#define dfERR_MAX						256


