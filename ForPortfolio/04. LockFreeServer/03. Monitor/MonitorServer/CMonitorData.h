#pragma once
#include "CChatData.h"
#include "CGameData.h"
#include "CLoginData.h"
#include "CServerData.h"

class CMonitorData
{
public:
	CChatData _chat;
	CGameData _game;
	CLoginData _login;
	CServerData _server;
};

extern CMonitorData g_monitor;