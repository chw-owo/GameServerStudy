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

class DiagCuz
{
public:
	DiagCuz(int x, int y, Pos parentPos) : _x(x), _y(y), _parentPos(parentPos) {}

public:
	int _x;
	int _y;
	Pos _parentPos;
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
	vector<Node*> _correctedList;

public:
	vector<DiagCuz> _diagCuzList;
	vector<Pos> _checkedList;
	vector<Pos> _checkedDiagList;
};

