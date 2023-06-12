#pragma once
#include "Pos.h"

struct Node
{
public:
	Node(Pos pos, double g, double h, Node* pParent = nullptr)
		: _pos(pos), _g(g), _h(h), _f(g + h), _pParent(pParent) {}
	~Node() {}

public:
	void SetData(Pos pos, double g, double h, Node* pParent = nullptr);
	void ResetParent(double g, Node* pParent);

public:
	bool operator==(const Node& other) { return _f == other._f; }
	bool operator!=(const Node& other) { return _f != other._f; }
	bool operator < (const Node& other) const { return _f < other._f; }
	bool operator > (const Node& other) const { return _f > other._f; }
	bool operator <= (const Node& other) const { return _f <= other._f; }
	bool operator >= (const Node& other) const { return _f >= other._f; }

public:
	Pos _pos;
	double _g;
	double _h;
	double _f;
	Node* _pParent;
};

