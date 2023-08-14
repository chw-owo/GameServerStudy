#pragma once

#define dfSERVER_IP L"127.0.0.1"
#define dfLANSERVER_PORT 6000
#define dfNETSERVER_PORT 3000

#define dfLAN_HEADER_LEN 2
struct stLanHeader
{
	short _shLen;
};

#define dfNET_HEADER_LEN 2
struct stNetHeader
{
	short _shLen;
};

//-----------------------------
//  Echo
//-----------------------------

#define dfECHO_LEN 8
