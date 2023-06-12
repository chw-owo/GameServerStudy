#pragma once
#include "Node.h"
#include "Map.h"
#include <queue>
using namespace std;

struct cmp
{
	bool operator() (Node* const left, Node* const right) const;
};

class NodeMgr
{
private:
	NodeMgr();

public:
	static NodeMgr* GetInstance();
	void SetStartDestNode();

private:
	static NodeMgr _NodeMgr;

public:
	Map* _pMap;
	Node* _pStart = nullptr;
	Node* _pDest = nullptr;
	priority_queue<Node*, vector<Node*>, cmp> _openSet;
	priority_queue<Node*, vector<Node*>, cmp> _closeSet;
};

