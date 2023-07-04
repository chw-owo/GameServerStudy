#pragma once
#include <windows.h>
#include <stdio.h>
#include <set>

#include "Profiler.h"
#include "RedBlackTree.h"
#include "BinaryTree.h"

using namespace std;
#define MOVE 10
class TreeTester
{
public:
	TreeTester();
	~TreeTester();

public:
	void DrawTree(HDC hdc);
	void PrintMenu();

public:
	void MoveRight() { _iXPad -= MOVE; }
	void MoveLeft() { _iXPad += MOVE; }
	void MoveUp() { _iYPad += MOVE; }
	void MoveDown() { _iYPad -= MOVE; }

public:
	void SearchNode();
	void InsertNode();
	void InsertRandomNodeUnder9999();
	void InsertRandomNode();
	void DeleteNode();
	void DeleteRandomNode();
	void PrintPathData();
	void PrintNodeData();

	void TestTree();
	void SetCompareMode();
	void PrintCompareResult();
	void ShiftTreeDraw();

private:
	bool GetTreeDataForTest(set<int>& testSet);
	bool InsertForTest(int count, set<int>& testSet);
	bool DeleteForTest(int count, set<int>& testSet);

private:
	bool _bCompareMode = false;
	bool _bDrawRedBlackTree = true;
	int _iXPad = 0;
	int _iYPad = 0;
	HPEN _hBlackPen;
	HPEN _hRedPen;

	RedBlackTree<int> _RedBlackTree;
	BinaryTree<int> _BinaryTree;
};

