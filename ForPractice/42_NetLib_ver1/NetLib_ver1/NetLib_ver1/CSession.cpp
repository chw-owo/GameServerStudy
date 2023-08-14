#include "CSession.h"
unordered_map<int, CSession*> g_SessionMap;
SRWLOCK g_SessionMapLock;