#pragma once
#include "PathFindAlgorithm.h"

class AStar : public PathFindAlgorithm
{
public:
	void FindPath();
	void FindPathStepInto();
	void CorrectPath();

private:
	void CreateNode(Node* pCurNode);

private:
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

