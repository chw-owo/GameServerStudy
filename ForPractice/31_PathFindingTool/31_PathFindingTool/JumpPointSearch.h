#pragma once
#include "PathFindAlgorithm.h"

class JumpPointSearch : public PathFindAlgorithm
{
public:
	void FindPath();
	void CreateNode(Node* pCurNode);
};

