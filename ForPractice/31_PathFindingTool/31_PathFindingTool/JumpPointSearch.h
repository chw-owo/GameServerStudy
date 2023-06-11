#pragma once
#include "PathFindAlgorithm.h"

class JumpPointSearch : public PathFindAlgorithm
{
public:
	void SetStartDestNode(Pos startPos, Pos destPos);
	void FindPath();
};

