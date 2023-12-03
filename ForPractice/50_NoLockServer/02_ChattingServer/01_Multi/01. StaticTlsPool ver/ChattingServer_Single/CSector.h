#pragma once
#include <vector>
#include "Protocol.h"
using namespace std;

class CPlayer;
class CSector
{
public:
	int _x; 
	int _y; 

	vector<CSector*> _around;
	vector<CPlayer*> _players;
	SRWLOCK _lock;
};