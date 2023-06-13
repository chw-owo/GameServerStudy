#pragma once
#include "Pos.h"

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

