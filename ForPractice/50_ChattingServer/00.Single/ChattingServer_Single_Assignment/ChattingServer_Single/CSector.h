#pragma once
#include <vector>
#include "Protocol.h"
using namespace std;

class CPlayer;
class CSector
{
public:
	CSector* _around[dfAROUND_SECTOR_NUM];
	vector<CPlayer*> _players;
};