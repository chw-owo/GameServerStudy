#pragma once
#include "PathFindAlgorithm.h"

class JumpPointSearch : public PathFindAlgorithm
{
public:
	void FindPath();
	
private:
	void CheckCreateNode(Node* pCurNode, DIR dir);
	void CheckCorner(Pos curPos, DIR dir, Pos& newPos);
	void CreateNode(Node* pCurNode, Pos newPos, DIR dir, int newG);
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

