#pragma once
#include <stdio.h>
#include <windows.h>
#include <cmath>
#include "TreeSetting.h"
#include "RedBlackTree.h"

template<typename DATA>
class BinaryTree
{
private:
	class Node
	{
		friend BinaryTree;

	private:
		Node(DATA data) : _data(data) {}
		~Node() {}

	private:
		DATA _data;
		Node* _pLeft = nullptr;
		Node* _pRight = nullptr;
	};

public:
	BinaryTree() {}
	~BinaryTree() {}

public:
	INSERT_NODE_RETURN InsertNode(DATA data)
	{
		if (_size >= TREE_MAX)
		{
			//printf("Tree is full. MAX: %d\n", _size);
			return INSERT_NODE_RETURN::TREE_IS_FULL;
		}

		if (_pRoot == nullptr)
		{
			_pRoot = new Node(data);
			//printf("Success to Insert %d!\n", data);
			_size++;
			return INSERT_NODE_RETURN::SUCCESS;
		}

		Node* pNode = _pRoot;

		for (;;)
		{
			if (pNode->_data == data)
			{
				//printf("중복되는 값을 넣을 수 없습니다: %d\n", data);
				return INSERT_NODE_RETURN::DUPLICATE_VALUE;
			}

			if (pNode->_data > data)
			{
				if (pNode->_pLeft == nullptr)
				{
					Node* pNewNode = new Node(data);
					pNode->_pLeft = pNewNode;
					//printf("Success to Insert %d!\n", data);
					_size++;
					return INSERT_NODE_RETURN::SUCCESS;
				}
				else
				{
					pNode = pNode->_pLeft;
					continue;
				}
			}

			if (pNode->_data < data)
			{
				if (pNode->_pRight == nullptr)
				{
					Node* pNewNode = new Node(data);
					pNode->_pRight = pNewNode;
					//printf("Success to Insert %d!\n", data);
					_size++;
					return INSERT_NODE_RETURN::SUCCESS;
				}
				else
				{
					pNode = pNode->_pRight;
					continue;
				}
			}
		}
		return INSERT_NODE_RETURN::UNKNOWN_ERROR;
	}

public:
	DELETE_NODE_RETURN DeleteNode(DATA data)
	{
		Node* pNode = nullptr;
		Node* pParent = nullptr;
		bool bNodeLeft = false;
		if (!SearchNode(data, pNode, pParent, bNodeLeft))
			return DELETE_NODE_RETURN::CANT_FIND;

		// Delete Input Data Node 
		if (pNode->_pLeft == nullptr &&
			pNode->_pRight == nullptr)
		{
			//printf("(0) ");

			if (pNode == _pRoot)
				_pRoot = nullptr;
			else if (bNodeLeft)
				pParent->_pLeft = nullptr;
			else if (!bNodeLeft)
				pParent->_pRight = nullptr;

			delete(pNode);
			//printf("Success to Delete %d!\n", data);
			_size--;
			return DELETE_NODE_RETURN::SUCCESS;
		}
		else if (pNode->_pLeft == nullptr)
		{
			//printf("(1) ");

			if (pNode == _pRoot)
				_pRoot = pNode->_pRight;
			else if (bNodeLeft)
				pParent->_pLeft = pNode->_pRight;
			else if (!bNodeLeft)
				pParent->_pRight = pNode->_pRight;

			delete(pNode);
			//printf("Success to Delete %d!\n", data);
			_size--;
			return DELETE_NODE_RETURN::SUCCESS;
		}
		else if (pNode->_pRight == nullptr)
		{
			//printf("(2) ");

			if (pNode == _pRoot)
				_pRoot = pNode->_pLeft;
			else if (bNodeLeft)
				pParent->_pLeft = pNode->_pLeft;
			else if (!bNodeLeft)
				pParent->_pRight = pNode->_pLeft;

			delete(pNode);
			//printf("Success to Delete %d!\n", data);
			_size--;
			return DELETE_NODE_RETURN::SUCCESS;
		}

		// Delete Alternate Node 

		//printf("(3) ");

		Node* pAlt = pNode->_pLeft;
		Node* pAltParent = pNode;
		bool bAltLeft = true;

		while (pAlt->_pRight != nullptr)
		{
			pAltParent = pAlt;
			pAlt = pAlt->_pRight;
			bAltLeft = false;
		}

		pNode->_data = pAlt->_data;

		if (bAltLeft && pAlt->_pLeft != nullptr)
			pAltParent->_pLeft = pAlt->_pLeft;
		else if (bAltLeft && pAlt->_pLeft == nullptr)
			pAltParent->_pLeft = nullptr;
		else if (!bAltLeft && pAlt->_pLeft != nullptr)
			pAltParent->_pRight = pAlt->_pLeft;
		else if (!bAltLeft && pAlt->_pLeft == nullptr)
			pAltParent->_pRight = nullptr;

		delete(pAlt);

		//printf("Success to Delete %d!\n", data);
		_size--;
		return DELETE_NODE_RETURN::SUCCESS;
	}

	void DeleteAllNode()
	{
		PostOrderDeleteNode(_pRoot);
		_pRoot = nullptr;
		_size = 0;
	}

private:
	void PostOrderDeleteNode(Node* pNode)
	{
		if (pNode == nullptr) return;

		PostOrderDeleteNode(pNode->_pLeft);
		PostOrderDeleteNode(pNode->_pRight);
		delete(pNode);
	}

public:
	bool SearchNode(DATA data)
	{
		if (_pRoot == nullptr)
		{
			//printf("현재 비어있는 트리입니다.\n");
			return false;
		}

		Node* pNode = _pRoot;

		while (pNode->_data != data)
		{
			if (pNode->_data > data)
			{
				if (pNode->_pLeft == nullptr)
				{
					//printf("존재하지 않는 값입니다: %d\n", data);
					return false;
				}
				else
				{
					pNode = pNode->_pLeft;
					continue;
				}
			}

			if (pNode->_data < data)
			{
				if (pNode->_pRight == nullptr)
				{
					//printf("존재하지 않는 값입니다: %d\n", data);
					return false;
				}
				else
				{
					pNode = pNode->_pRight;
					continue;
				}
			}
		}

		//printf("Success to Search %d!\n", data);
		return true;
	}

private:
	bool SearchNode(DATA data, Node*& pNode, Node*& pParent, bool& bNodeLeft)
	{
		if (_pRoot == nullptr)
		{
			//printf("현재 비어있는 트리입니다.\n");
			pNode = nullptr;
			pParent = nullptr;
			return false;
		}

		pParent = nullptr;
		pNode = _pRoot;

		while (pNode->_data != data)
		{
			if (pNode->_data > data)
			{
				if (pNode->_pLeft == nullptr)
				{
					//printf("존재하지 않는 값입니다: %d\n", data);
					pNode = nullptr;
					pParent = nullptr;
					return false;
				}
				else
				{
					pParent = pNode;
					pNode = pNode->_pLeft;
					bNodeLeft = true;
					continue;
				}
			}

			if (pNode->_data < data)
			{
				if (pNode->_pRight == nullptr)
				{
					//printf("존재하지 않는 값입니다: %d\n", data);
					pNode = nullptr;
					pParent = nullptr;
					return false;
				}
				else
				{
					pParent = pNode;
					pNode = pNode->_pRight;
					bNodeLeft = false;
					continue;
				}
			}
		}

		//printf("Success to Search %d!\n", data);
		return true;
	}

public:
	void PrintAllNode()
	{
		InOrderPrintNode(_pRoot);
		printf("\n");
	}

private:
	void InOrderPrintNode(Node* pNode)
	{
		if (pNode == nullptr) return;

		InOrderPrintNode(pNode->_pLeft);
		printf("%d", pNode->_data);
		InOrderPrintNode(pNode->_pRight);
	}

public:
	int GetTreeSize()
	{
		return _size;
	}

	void GetAllNode(DATA* dataArray)
	{
		int index = 0;
		InOrderGetNode(_pRoot, dataArray, &index);
	}

private:
	void InOrderGetNode(Node* pNode, DATA* dataArray, int* index)
	{
		if (pNode == nullptr) return;

		InOrderGetNode(pNode->_pLeft, dataArray, index);
		dataArray[*index] = pNode->_data;
		(*index)++;
		InOrderGetNode(pNode->_pRight, dataArray, index);
	}

public:
	void CopyRedBlackTree(RedBlackTree<int>* redBlackTree)
	{
		PreOrderCopyNode(redBlackTree->_pRoot, redBlackTree->_pNil);
	}

	void PreOrderCopyNode(RedBlackTree<int>::Node* pNode, RedBlackTree<int>::Node* pNil)
	{
		if (pNode == pNil) return;

		InsertNode(pNode->_data);
		PreOrderCopyNode(pNode->_pLeft, pNil);
		PreOrderCopyNode(pNode->_pRight, pNil);
	}

public:
	void DrawAllNode(HDC hdc, int xPad, int yPad)
	{
		DrawNode(hdc, _pRoot,
			X_PIVOT, Y_PIVOT, X_PIVOT, Y_PIVOT,
			xPad, yPad, 0, 0, 0);
	}

private:
	void DrawNode(HDC hdc, Node* pNode, int prevXPos, int prevYPos,
		int xPos, int yPos, int xPad, int yPad,
		int left, int right, int depth)
	{
		if (pNode == nullptr) return;

		MoveToEx(hdc, prevXPos + xPad, prevYPos + yPad, NULL);
		LineTo(hdc, xPos + xPad, yPos + yPad);

		WCHAR text[TEXT_LEN] = { 0 };
		_itow_s(pNode->_data, text, TEXT_LEN, 10);

		Ellipse(hdc,
			xPos - RADIUS + xPad,
			yPos - RADIUS + yPad,
			xPos + RADIUS + xPad,
			yPos + RADIUS + yPad);

		TextOutW(hdc,
			xPos - TEXT_PAD + xPad,
			yPos - TEXT_PAD + yPad,
			text, wcslen(text));

		depth++;

		int prevXGap = abs(prevXPos - xPos);

		if (pNode == _pRoot)
			prevXGap = xPos;

		left++;
		DrawNode(hdc, pNode->_pLeft, xPos, yPos,
			xPos - (prevXGap / 2), yPos + Y_GAP,
			xPad, yPad, left, right, depth);
		left--;

		right++;
		DrawNode(hdc, pNode->_pRight, xPos, yPos,
			xPos + (prevXGap / 2), yPos + Y_GAP,
			xPad, yPad, left, right, depth);
		right--;

		depth--;
		return;
	}

private:
	Node* _pRoot = nullptr;
	int _size = 0;
};

