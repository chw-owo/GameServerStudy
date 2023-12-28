#include "CPlayer.h"
CObjectPool<ID> g_IDPool = CObjectPool<ID>(dfPLAYER_MAX, false);
CObjectPool<Nickname> g_NicknamePool = CObjectPool<Nickname>(dfPLAYER_MAX, false);
CObjectPool<SessionKey> g_SessionKey = CObjectPool<SessionKey>(dfPLAYER_MAX, false);