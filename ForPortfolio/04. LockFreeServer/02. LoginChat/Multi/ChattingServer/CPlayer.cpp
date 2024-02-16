#include "CPlayer.h"
CTlsPool<ID> g_IDPool = CTlsPool<ID>(dfPLAYER_MAX, false);
CTlsPool<Nickname> g_NicknamePool = CTlsPool<Nickname>(dfPLAYER_MAX, false);
CTlsPool<SessionKey> g_SessionKey = CTlsPool<SessionKey>(dfPLAYER_MAX, false);