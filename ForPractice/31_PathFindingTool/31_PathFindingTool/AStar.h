#pragma once
#include "PathFindAlgorithm.h"

class AStar : public PathFindAlgorithm
{
public:
	void FindPath();
	void CreateNode(Node* pCurNode);

};

