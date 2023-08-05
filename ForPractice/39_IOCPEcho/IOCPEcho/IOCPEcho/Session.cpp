#include "Session.h"
unordered_map<int, Session*> g_SessionMap;
SRWLOCK g_SessionMapLock;