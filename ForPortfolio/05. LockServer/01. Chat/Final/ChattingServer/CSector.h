#pragma once
#include <vector>
#include "CommonProtocol.h"
using namespace std;

class CPlayer;
class CSector
{
public:
	int _x; // For Debug
	int _y; // For Debug

	vector<CSector*> _around;
	vector<CPlayer*> _players;
};