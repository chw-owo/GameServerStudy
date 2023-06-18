#pragma once
#include "Node.h"
#include "Map.h"
#include <vector>
using namespace std;

class CompareF
{
public:
	bool operator() (Node* const left, Node* const right) const;
};

class NodeMgr
{
private:
	NodeMgr();

public:
	static NodeMgr* GetInstance();
	void SetData();
	
private:
	static NodeMgr _NodeMgr;

public:
	Map* _pMap;
	Node* _pCurNode = nullptr;
	Node* _pStart = nullptr;
	Node* _pDest = nullptr;

public:
	vector<Node*> _openList;
	vector<Node*> _closeList;

public:
	vector<Pos> _diagCuzList;
	vector<Pos> _checkedList;
	vector<Pos> _checkedDiagList;
};

