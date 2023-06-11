#pragma once
#include <set>
#include <map>
using namespace std;

struct Pos
{
public:
	Pos() : _x(0), _y(0) {}
	Pos(int x, int y) : _x(x), _y(y) {}

public:
	int _x = 0;
	int _y = 0;

public:
	Pos operator+(const Pos& other) { return Pos (_x + other._x, _y + other._y ); }
	bool operator==(const Pos& other) { return (_x == other._x && _y == other._y); }
	bool operator!=(const Pos& other) { return (_x != other._x || _y != other._y); }
	bool operator < (const Pos& other) const;
	bool operator > (const Pos& other) const;

};

struct Node
{
public:
	Node(Pos pos, int g, int h, Node* pParent = nullptr)
		: _pos(pos), _g(g), _h(h), _f(g + h), _pParent(pParent) {}
	~Node() {}

public:
	void SetData(Pos pos, int g, int h, Node* pParent = nullptr);
	void ResetParent(int g, Node* pParent);

public:
	bool operator==(const Node& other) { return _f == other._f; }
	bool operator!=(const Node& other) { return _f != other._f; }
	bool operator < (const Node& other) const { return _f < other._f; }
	bool operator > (const Node& other) const { return _f > other._f; }
	bool operator <= (const Node& other) const { return _f <= other._f; }
	bool operator >= (const Node& other) const { return _f >= other._f; }

public:
	Pos _pos;
	int _g;
	int _h;
	int _f;
	Node* _pParent;
};

struct cmp
{
	bool operator() (const Node* left, const Node* right) const;
};

class NodeMgr
{
private:
	NodeMgr() {}

public:
	static NodeMgr* GetInstance();

public:
	void InsertOpenNode(Node* node);
	void InsertCloseNode(Node* node);
	void SetStartDestNode(Pos startPos, Pos destPos);

private:
	static NodeMgr _NodeMgr;

public:
	Node* _pStart = nullptr;
	Node* _pDest = nullptr;
	multiset<Node*, cmp> _openSet;
	map<Pos, Node*> _closeMap;
};
