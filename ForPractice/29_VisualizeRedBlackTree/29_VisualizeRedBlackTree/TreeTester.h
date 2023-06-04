#pragma once
#include <windows.h>
#include <stdio.h>
#include <set>
#include "RedBlackTree.h"
//#include "BinaryTree.h"

using namespace std;
#define MOVE 10
class TreeTester
{
public:
	TreeTester();
	~TreeTester();

	void GetLeafData();

public:
	void DrawTree(HDC hdc);

public:
	void MoveRight() { _iXPad -= MOVE; }
	void MoveLeft() { _iXPad += MOVE; }
	void MoveUp() { _iYPad += MOVE; }
	void MoveDown() { _iYPad -= MOVE; }

public:
	void GetTreeData();
	void InsertNode();
	void InsertRandomNodeUnder9999();
	void InsertRandomNode();
	void DeleteNode();
	void DeleteRandomNode();
	void TestTree();

public:
	void PrintMenu();

private:
	bool GetTreeDataForTest(set<int>& testSet);
	bool InsertForTest(int count, set<int>& testSet);
	bool DeleteForTest(int count, set<int>& testSet);

private:
	int _iXPad = 0;
	int _iYPad = 0;
	HPEN _hBlackPen;
	HPEN _hRedPen;

	RedBlackTree<int> _RedBlackTree;
	//BinaryTree<int> _BinaryTree;
};

