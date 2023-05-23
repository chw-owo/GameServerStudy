#pragma once
#include "SerializePacket.h"

/*TO-DO
지금은 임시로 만든 것. 이후에 자동화 할 수 있는 코드로 수정하기
헤더는 사용자가 만들도록 해야 되나...?? 어떻게 하지..??
일단 대문자 SIZE, TYPE에 한해 지금처럼 만들어주는 등의 옵션을 주는 게 나으려나?
*/

#define df_HEADER_CODE	(char)0x89
#define df_HEADER_SIZE	sizeof(stHEADER)

#pragma pack (push, 1)
struct stHEADER
{
	char code = df_HEADER_CODE;
	char size; 
	char type;
};
#pragma pack (pop)

// fold 식이므로 c++17 이상에서만 사용 가능
template <typename... DATA>
int GetTotalSize(DATA... data)
{
	return (... + sizeof(data));
}

template <typename... DATA>
void CreateHeader(SerializePacket* packet, int type, DATA ... data)
{
	stHEADER header;
	header.code = df_HEADER_CODE;
	header.size =  GetTotalSize(data...);
	header.type = type;

	*packet << header.code << header.size << header.type;
}

#define	df_SC_CREATE_MY_CHARACTER			0
#define	df_SC_CREATE_OTHER_CHARACTER		1
#define	df_SC_DELETE_CHARACTER				2

#define	df_CS_MOVE_START					10
#define	df_SC_MOVE_START					11
#define	df_CS_MOVE_STOP						12
#define	df_SC_MOVE_STOP						13

#define	df_CS_ATTACK1						20
#define	df_SC_ATTACK1						21
#define	df_CS_ATTACK2						22
#define	df_SC_ATTACK2						23
#define	df_CS_ATTACK3						24
#define	df_SC_ATTACK3						25

#define	df_SC_DAMAGE						30

/* TO-DO

Fighting Game 예제에서는 
프로토콜 단계에서 UNI/MULTI/BROAD를 구분할 수 없어서 
대신 이를 인자로 받는 것으로 만들었다.

아예 프로토콜 단계에서 분리하는 게 좋을지 나중에 생각해보기
*/

enum TARGET
{
	UNI = 0,
	//MULTI,
	BROAD,
	BROAD_EXP
};