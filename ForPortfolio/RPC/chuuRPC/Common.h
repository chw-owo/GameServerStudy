#pragma once
#include "SerializePacket.h"

/*TO-DO
������ �ӽ÷� ���� ��. ���Ŀ� �ڵ�ȭ �� �� �ִ� �ڵ�� �����ϱ�
����� ����ڰ� ���鵵�� �ؾ� �ǳ�...?? ��� ����..??
�ϴ� �빮�� SIZE, TYPE�� ���� ����ó�� ������ִ� ���� �ɼ��� �ִ� �� ��������?
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

// fold ���̹Ƿ� c++17 �̻󿡼��� ��� ����
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

Fighting Game ���������� 
�������� �ܰ迡�� UNI/MULTI/BROAD�� ������ �� ��� 
��� �̸� ���ڷ� �޴� ������ �������.

�ƿ� �������� �ܰ迡�� �и��ϴ� �� ������ ���߿� �����غ���
*/

enum TARGET
{
	UNI = 0,
	//MULTI,
	BROAD,
	BROAD_EXP
};