#include "Node.h"

bool Pos::operator < (const Pos& other) const
{
	if (_x != other._x)
		return (_x < other._x);
	else
		return (_y < other._y);
}

bool Pos::operator > (const Pos& other) const
{
	if (_x != other._x)
		return (_x > other._x);
	else
		return (_y > other._y);
}

void Node::SetData(Pos pos, int g, int h, Node* pParent)
{
	_pos = pos;
	_g = g;
	_h = h;
	_f = g + h;
	_pParent = pParent;
}

void Node::ResetParent(int g, Node* pParent)
{
	_g = g;
	_f = _h + _g;
	_pParent = pParent;
}


NodeMgr* NodeMgr::GetInstance()
{
	static NodeMgr _NodeMgr;
	return &_NodeMgr;
}

void NodeMgr::InsertOpenNode(Node* pNode)
{
	_openSet.insert(pNode);
}

void NodeMgr::InsertCloseNode(Node* pNode)
{
	_openSet.erase(pNode);
	_closeMap.insert(make_pair(pNode->_pos, pNode));
}

void NodeMgr::SetStartDestNode(Pos startPos, Pos destPos)
{
	_pDest = nullptr;

	if (_pStart == nullptr)
	{
		printf("create start node (%d, %d)\n", startPos._x, startPos._y);
		_pStart = new Node(startPos, 0,
			abs(destPos._x - startPos._x) + abs(destPos._y - startPos._y));
	}
	else
	{
		printf("reset start node (%d, %d)\n", startPos._x, startPos._y);
		_pStart->SetData(startPos, 0,
			abs(destPos._x - startPos._x) + abs(destPos._y - startPos._y));
	}

	InsertOpenNode(_pStart);
}

bool cmp::operator() (const Node* left, const Node* right) const
{
	if (left->_f == right->_f)
	{
		return left < right;
	}
	else
	{
		return left->_f < right->_f;
	}
}