#pragma once
#include "PathFindAlgorithm.h"

class JumpPointSearch : public PathFindAlgorithm
{
public:
	void FindPath();
	void FindPathStepInto();
	
private:
	void CheckCreateNode(Node* pCurNode);
	void CheckCorner(Pos curPos, DIR dir, Pos& newPos, DIR& searchDir);
	void CreateNode(Node* pCurNode, Pos newPos, DIR dir, DIR searchDir, int newG);
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

