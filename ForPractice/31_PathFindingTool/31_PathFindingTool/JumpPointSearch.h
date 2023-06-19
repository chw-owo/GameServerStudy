#pragma once
#include "PathFindAlgorithm.h"

class JumpPointSearch : public PathFindAlgorithm
{
public:
	void FindPath();
	void FindPathStepInto();
	
private:
	void CheckCreateNode(Node* pCurNode);
	bool CheckCorner(Pos curPos, DIR dir, Pos& newPos, DIR& searchDir);
	bool CheckDiagCorner(Pos diag, DIR diagDir, DIR searchDir, Pos& newPos, DIR& newSearchDir);
	void CreateNode(Node* pCurNode, Pos newPos, DIR dir, DIR searchDir, int newG);
	
private:
	void CorrectPath();
	bool CheckObstacle(Node* pNode1, Node* pNode2);

private:
	void PrintDirForDebug(Node* pCurNode);
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

