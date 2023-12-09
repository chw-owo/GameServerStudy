#pragma once

// Network Info
#define dfSERVER_IP				L"0.0.0.0"
#define dfLOGIN_PORT			8091
#define dfECHO_PORT				40000
#define dfSERVER_DISCONNECT		false
#define dfTHREAD_MAX			32
#define dfUSER_MAX				20000
#define dfTIMEOUT				40000

// Use in Login Server
#define dfSESSIONKEY_LEN		64


enum en_PACKET_TYPE
{
	//------------------------------------------------------
	// Game Server
	//------------------------------------------------------
	en_PACKET_CS_GAME_SERVER = 1000,

	//------------------------------------------------------------
	// �α��� ��û
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//
	//		int		Version			// 1 
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_GAME_REQ_LOGIN,

	//------------------------------------------------------------
	// �α��� ����
	//
	//	{
	//		WORD	Type
	//
	//		BYTE	Status (0: ���� / 1: ����)
	//		INT64	AccountNo
	//	}
	//
	//	���� ���̴� ������ �������� �Ǵ��ϰ� ����
	//	Status ����� �����Ѵٴ� �̾߱�
	//
	//  en_PACKET_CS_GAME_RES_LOGIN define �� ���.
	//------------------------------------------------------------
	en_PACKET_CS_GAME_RES_LOGIN,



	//------------------------------------------------------------
	// �׽�Ʈ�� ���� ��û
	//
	//	{
	//		WORD		Type
	//
	//		INT64		AccountoNo
	//		LONGLONG	SendTick
	//	}
	//
	//------------------------------------------------------------	
	en_PACKET_CS_GAME_REQ_ECHO = 5000,

	//------------------------------------------------------------
	// �׽�Ʈ�� ���� ���� (REQ �� �״�� ������)
	//
	//	{
	//		WORD		Type
	//
	//		INT64		AccountoNo
	//		LONGLONG	SendTick
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_GAME_RES_ECHO,

	//------------------------------------------------------------
	// ��Ʈ��Ʈ
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// Ŭ���̾�Ʈ�� �̸� 30�ʸ��� ������.
	// ������ 40�� �̻󵿾� �޽��� ������ ���� Ŭ���̾�Ʈ�� ������ ������� ��.
	//------------------------------------------------------------	
	en_PACKET_CS_GAME_REQ_HEARTBEAT,
};
