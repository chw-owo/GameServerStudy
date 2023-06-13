#pragma once
#include "PathFindAlgorithm.h"

class AStar : public PathFindAlgorithm
{
public:
	void FindPath();
	void CreateNode(Node* pCurNode);
	void PrintOpenListForDebug();

private:
	class CompareG
	{
	public:
		Pos pos;
		int g = 0;
		bool operator()(Node*& pNode) const; 
	};

private:
	CompareF compareF;
	CompareG compareG;


};

