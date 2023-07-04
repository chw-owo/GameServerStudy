#pragma once
#include "NodeMgr.h"

class PathFindAlgorithm
{
public:
	PathFindAlgorithm();
	~PathFindAlgorithm() {}

public:
	bool GetFindPathOn() { return _bFindPathOn; }
	void StartFindPath();

public:
	virtual void FindPath() = 0;
	virtual void FindPathStepInto() = 0;

protected:
	void CorrectPath();
	void PrintOpenListForDebug();

private:
	bool CheckObstacle(Node* pNode1, Node* pNode2);

public:

	void SetDebugCreateNode(bool debugCreateNode)
	{
		if (debugCreateNode) printf("\nDebug CreateNode ON\n");
		else printf("\nDebug CreateNode OFF\n");

		_debugCreateNode = debugCreateNode;
	}

	void SetDebugOpenList(bool debugOpenList)
	{
		if (debugOpenList) printf("\nDebug OpenList ON\n");
		else printf("\nDebug OpenList OFF\n");

		_debugOpenList = debugOpenList;
	}

	void SetDebugFindCorner(bool debugFindCorner) 
	{ 
		if (debugFindCorner) printf("\nDebug FindCorner ON\n");
		else printf("\nDebug FindCorner OFF\n");

		_debugFindCorner = debugFindCorner; 
	}

	void SetCorrectPath(bool correctPath)
	{
		if (correctPath) printf("\nCorrectPath ON\n");
		else printf("\nCorrectPath OFF\n");

		_correctPath = correctPath;
	}

protected:
	Pos _direction[(int)DIR::NONE] =
	{
		Pos (0, -1),
		Pos (1, 0),
		Pos (0, 1),
		Pos (-1, 0),

		Pos (1, -1),
		Pos (1, 1),
		Pos (-1, 1),
		Pos (-1, -1)
	};

public:
	bool _bFindPathOn = false;
	bool _bFindPathStepOn = false;

protected:
	NodeMgr* _pNodeMgr = nullptr;

protected:
	bool _debugCreateNode = false;
	bool _debugOpenList = false;
	bool _debugFindCorner = true;
	bool _correctPath = true;

};