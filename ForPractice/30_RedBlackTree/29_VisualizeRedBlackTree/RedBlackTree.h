#pragma once
#include <stdio.h>
#include <windows.h>
#include <cmath>
#include "TreeSetting.h"

template<typename DATA>
class RedBlackTree
{
	enum COLOR
	{
		BLACK = 0,
		RED
	};

public:
	class Node
	{
		friend RedBlackTree;

	private:
		Node(DATA data, Node* pChild) 
			: _data(data), _color(RED), _pParent(nullptr), 
			_pLeft(pChild), _pRight(pChild){}

		~Node() {}

	public:
		DATA _data;

		COLOR _color;
		Node* _pParent;
		Node* _pLeft;
		Node* _pRight;
	};

public:
	RedBlackTree() 
	{
		_pNil = new Node(0, nullptr);
		_pNil->_color = BLACK;
		_pRoot = _pNil;
	}
	~RedBlackTree() {}

public:
	INSERT_NODE_RETURN InsertNode(DATA data)
	{
		if (_size >= TREE_MAX)
		{
			//printf("Tree is full. MAX: %d\n", _size);
			return INSERT_NODE_RETURN::TREE_IS_FULL;
		}

		if (_pRoot == _pNil)
		{
			_pRoot = new Node(data, _pNil);
			_pRoot->_color = BLACK;
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
				if (pNode->_pLeft == _pNil)
				{
					Node* pNewNode = new Node(data, _pNil);
					pNode->_pLeft = pNewNode;
					pNewNode->_pParent = pNode;
					InsertBalanceCheck(pNewNode);
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
				if (pNode->_pRight == _pNil)
				{
					Node* pNewNode = new Node(data, _pNil);
					pNode->_pRight = pNewNode;
					pNewNode->_pParent = pNode;
					InsertBalanceCheck(pNewNode);
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

private:
	void InsertBalanceCheck(Node* pNode)
	{
		while (pNode->_pParent->_color != BLACK)
		{
			bool bParentLeft = false;
			if (pNode->_pParent == pNode->_pParent->_pParent->_pLeft)
				bParentLeft = true;

			if (bParentLeft
				&& pNode->_pParent->_pParent->_pRight->_color == RED)
			{
				pNode->_pParent->_color = BLACK;
				pNode->_pParent->_pParent->_pRight->_color = BLACK;

				if (pNode->_pParent->_pParent == _pRoot)
				{
					break;
				}
				else
				{
					pNode->_pParent->_pParent->_color = RED;
					pNode = pNode->_pParent->_pParent;
					continue;
				}
			}

			else if (!bParentLeft
				&& pNode->_pParent->_pParent->_pLeft->_color == RED)
			{
				pNode->_pParent->_color = BLACK;
				pNode->_pParent->_pParent->_pLeft->_color = BLACK;

				if (pNode->_pParent->_pParent == _pRoot)
				{
					break;
				}
				else
				{
					pNode->_pParent->_pParent->_color = RED;
					pNode = pNode->_pParent->_pParent;
					continue;
				}
			}
			else if (bParentLeft
				&& pNode->_pParent->_pParent->_pRight->_color == BLACK)
			{
				if (pNode == pNode->_pParent->_pRight)
				{
					RotateLeft(pNode->_pParent);
					pNode = pNode->_pLeft;
				}

				pNode->_pParent->_color = BLACK;
				pNode->_pParent->_pParent->_color = RED;
				RotateRight(pNode->_pParent->_pParent);
				break;
			}
			else if (!bParentLeft
				&& pNode->_pParent->_pParent->_pLeft->_color == BLACK)
			{
				if (pNode == pNode->_pParent->_pLeft)
				{
					RotateRight(pNode->_pParent);
					pNode = pNode->_pRight;
				}

				pNode->_pParent->_color = BLACK;
				pNode->_pParent->_pParent->_color = RED;
				RotateLeft(pNode->_pParent->_pParent);
				break;
			}
		}
	}

public:
	DELETE_NODE_RETURN DeleteNode(DATA data)
	{
		Node* pNode = nullptr;
		bool bLeft = false;
		if (!SearchNode(data, pNode, bLeft))
			return DELETE_NODE_RETURN::CANT_FIND;

		// Delete Input Data Node 
		if (pNode->_pLeft == _pNil &&
			pNode->_pRight == _pNil)
		{
			//printf("(0) ");

			if (pNode == _pRoot)
			{
				_pRoot = _pNil;
				delete(pNode);
			}
			else
			{
				if (bLeft)
				{
					pNode->_pParent->_pLeft = _pNil;
				}
				else
				{
					pNode->_pParent->_pRight = _pNil;
				}

				COLOR color = pNode->_color;
				Node* pParent = pNode->_pParent;
				delete(pNode);

				if (color == BLACK)
					DeleteBalanceCheck(_pNil, pParent);
			}

			//printf("Success to Delete %d!\n", data);
			_size--;
			return DELETE_NODE_RETURN::SUCCESS;
		}
		else if (pNode->_pLeft == _pNil)
		{
			//printf("(1) ");

			if (pNode == _pRoot)
			{
				_pRoot = pNode->_pRight;
				delete(pNode);
			}
			else
			{
				if (bLeft)
				{
					pNode->_pRight->_pParent = pNode->_pParent;
					pNode->_pParent->_pLeft = pNode->_pRight;
				}
				else
				{
					pNode->_pRight->_pParent = pNode->_pParent;
					pNode->_pParent->_pRight = pNode->_pRight;
				}

				Node* pChild = pNode->_pRight;
				COLOR color = pNode->_color;
				delete(pNode);

				if (color == BLACK)
					DeleteBalanceCheck(pChild, pChild->_pParent);
			}

			//printf("Success to Delete %d!\n", data);
			_size--;
			return DELETE_NODE_RETURN::SUCCESS;
		}
		else if (pNode->_pRight == _pNil)
		{
			//printf("(2) ");

			if (pNode == _pRoot)
			{ 
				_pRoot = pNode->_pLeft;
				delete(pNode);
			}
			else
			{
				if (bLeft)
				{
					pNode->_pLeft->_pParent = pNode->_pParent;
					pNode->_pParent->_pLeft = pNode->_pLeft;
				}
				else
				{
					pNode->_pLeft->_pParent = pNode->_pParent;
					pNode->_pParent->_pRight = pNode->_pLeft;
				}

				Node* pChild = pNode->_pLeft;
				COLOR color = pNode->_color;
				delete(pNode);

				if (color == BLACK)
					DeleteBalanceCheck(pChild, pChild->_pParent);
			}

			//printf("Success to Delete %d!\n", data);
			_size--;
			return DELETE_NODE_RETURN::SUCCESS;
		}

		// Delete Alternate Node 
		//printf("(3) ");

		Node* pAlt = pNode->_pLeft;
		bool bAltLeft = true;

		
		while (pAlt->_pRight != _pNil)
		{
			pAlt = pAlt->_pRight;
			bAltLeft = false;
		}
	
		pNode->_data = pAlt->_data;

		if (bAltLeft)
		{
			pAlt->_pLeft->_pParent = pAlt->_pParent;
			pAlt->_pParent->_pLeft = pAlt->_pLeft;
		}
		else if (!bAltLeft)
		{
			pAlt->_pLeft->_pParent = pAlt->_pParent;
			pAlt->_pParent->_pRight = pAlt->_pLeft;
		}

		COLOR color = pAlt->_color;
		Node* pChild = pAlt->_pLeft;
		Node* pParent = pAlt->_pParent;
		delete(pAlt);

		if (color == BLACK)
			DeleteBalanceCheck(pChild, pParent);

		//printf("Success to Delete %d!\n", data);
		_size--;
		return DELETE_NODE_RETURN::SUCCESS;
	}

	void DeleteAllNode()
	{
		PostOrderDeleteNode(_pRoot);
		_pRoot = _pNil;
		_size = 0;
	}

	void PostOrderDeleteNode(Node* pNode)
	{
		if (pNode == _pNil) return;

		PostOrderDeleteNode(pNode->_pLeft);
		PostOrderDeleteNode(pNode->_pRight);
		delete(pNode);
	}

private:

	void DeleteBalanceCheck(Node* pNode, Node* pParent)
	{
		while (pNode->_color != RED)
		{
			bool bLeft = false;
			Node* pSibling = nullptr;
			if (pNode == pParent->_pLeft)
			{
				bLeft = true;
				pSibling = pParent->_pRight;
			}
			else
			{
				pSibling = pParent->_pLeft;
			}

			if (pSibling->_color == RED)
			{
				pSibling->_color = BLACK;
				if (bLeft)
					RotateLeft(pParent);
				else
					RotateRight(pParent);
				pParent->_color = RED;
				continue;
			}
			else if (pSibling->_color == BLACK &&
				pSibling->_pLeft->_color == BLACK &&
				pSibling->_pRight->_color == BLACK)
			{
				pSibling->_color = RED;
				if (pParent == _pRoot)
					return;
				pNode = pParent;
				pParent = pParent->_pParent;
				continue;
			}

			if (bLeft &&
				pSibling->_color == BLACK &&
				pSibling->_pLeft->_color == RED &&
				pSibling->_pRight->_color == BLACK)
			{
				pSibling->_pLeft->_color = BLACK;
				pSibling->_color = RED;
				RotateRight(pSibling);
			}
			else if (!bLeft &&
				pSibling->_color == BLACK &&
				pSibling->_pLeft->_color == BLACK &&
				pSibling->_pRight->_color == RED)
			{
				pSibling->_pRight->_color = BLACK;
				pSibling->_color = RED;
				RotateLeft(pSibling);
			}

			if (bLeft &&
				pSibling->_color == BLACK &&
				pSibling->_pRight->_color == RED)
			{
				pSibling->_color = pParent->_color;
				pParent->_color = BLACK;
				pSibling->_pRight->_color = BLACK;
				RotateLeft(pParent);
				return;
			}

			else if (!bLeft &&
				pSibling->_color == BLACK &&
				pSibling->_pLeft->_color == RED)
			{
				pSibling->_color = pParent->_color;
				pParent->_color = BLACK;
				pSibling->_pLeft->_color = BLACK;
				RotateRight(pParent);
				return;
			}
		}

		pNode->_color = BLACK;
		return;
	}

public:
	bool SearchNode(DATA data)
	{
		if (_pRoot == _pNil)
		{
			//printf("현재 비어있는 트리입니다.\n");
			return false;
		}

		Node* pNode = _pRoot;

		while (pNode->_data != data)
		{
			if (pNode->_data > data)
			{
				if (pNode->_pLeft == _pNil)
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
				if (pNode->_pRight == _pNil)
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
	bool SearchNode(DATA data, Node*& pNode, bool& bNodeLeft)
	{
		if (_pRoot == _pNil)
		{
			//printf("현재 비어있는 트리입니다.\n");
			pNode = nullptr;
			return false;
		}

		pNode = _pRoot;

		while (pNode->_data != data)
		{
			if (pNode->_data > data)
			{
				if (pNode->_pLeft == _pNil)
				{
					//printf("존재하지 않는 값입니다: %d\n", data);
					pNode = nullptr;
					return false;
				}
				else
				{
					pNode = pNode->_pLeft;
					bNodeLeft = true;
					continue;
				}
			}

			if (pNode->_data < data)
			{
				if (pNode->_pRight == _pNil)
				{
					//printf("존재하지 않는 값입니다: %d\n", data);
					pNode = nullptr;
					return false;
				}
				else
				{
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

	void PrintAllPath()
	{
		int leafIdx = 0;
		int black = 0;
		int red = 0;
		
		printf("\n");
		PreOrderPrintPath(_pRoot, leafIdx, black, red);
		printf("\n");
	}

private:
	void InOrderPrintNode(Node* pNode)
	{
		if (pNode == _pNil) return;

		InOrderPrintNode(pNode->_pLeft);
		printf("%d", pNode->_data);
		InOrderPrintNode(pNode->_pRight);
	}

	void PreOrderPrintPath(Node* pNode, int& leafIdx, int black, int red)
	{
		if (pNode == _pNil)
		{
			black++;
			printf("Leaf %d: black-%d, red-%d\n", leafIdx, black, red);
			leafIdx++;
			return;
		}

		if (pNode->_color == BLACK) black++;
		else if (pNode->_color == RED) red++;

		PreOrderPrintPath(pNode->_pLeft, leafIdx, black, red);
		PreOrderPrintPath(pNode->_pRight, leafIdx, black, red);
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
		if (pNode == _pNil) return;

		InOrderGetNode(pNode->_pLeft, dataArray, index);
		dataArray[*index] = pNode->_data;
		(*index)++;
		InOrderGetNode(pNode->_pRight, dataArray, index);
	}

private:
	void RotateLeft(Node* pNode)
	{
		if (pNode->_pRight == _pNil)
		{
			printf("Left Error: %d\n", pNode->_data);
			return;
		}

		if (pNode == _pRoot)
		{
			_pRoot = pNode->_pRight;
			pNode->_pRight->_color = BLACK;
		}
		else
		{
			if (pNode == pNode->_pParent->_pRight)
				pNode->_pParent->_pRight = pNode->_pRight;
			else if (pNode == pNode->_pParent->_pLeft)
				pNode->_pParent->_pLeft = pNode->_pRight;
		}

		Node* pGrandChild = pNode->_pRight->_pLeft;
		pNode->_pRight->_pLeft = pNode;
		pNode->_pRight->_pParent = pNode->_pParent;
		pNode->_pParent = pNode->_pRight;
		pNode->_pRight = pGrandChild;
		pGrandChild->_pParent = pNode;
	}

	void RotateRight(Node* pNode)
	{
		if (pNode->_pLeft == _pNil)
		{
			printf("Right Error: %d\n", pNode->_data);
			return;
		}

		if (pNode == _pRoot)
		{
			_pRoot = pNode->_pLeft;
			pNode->_pLeft->_color = BLACK;
		}
		else
		{
			if (pNode == pNode->_pParent->_pRight)
				pNode->_pParent->_pRight = pNode->_pLeft;
			else if (pNode == pNode->_pParent->_pLeft)
				pNode->_pParent->_pLeft = pNode->_pLeft;
		}

		Node* pGrandChild = pNode->_pLeft->_pRight;
		pNode->_pLeft->_pRight = pNode;
		pNode->_pLeft->_pParent = pNode->_pParent;
		pNode->_pParent = pNode->_pLeft;
		pNode->_pLeft = pGrandChild;
		pGrandChild->_pParent = pNode;
	}

public:
	TEST_RETURN TestAllNode()
	{
		int leafIdx = 0;
		int black = 0;
		int red = 0;

		TEST_RETURN ret = PreOrderTestNode(_pRoot, leafIdx, black, red, BLACK);
		_PrevBlack = 0;

		return ret;
	}

private:
	TEST_RETURN PreOrderTestNode(Node* pNode, int& leafIdx, int black, int red, COLOR prevColor)
	{
		if (pNode == _pNil)
		{
			black++;
			if (black != _PrevBlack && _PrevBlack != 0)
			{
				printf("Unbalanced! leaf idx %d, data %d\n", leafIdx, pNode->_data);
				return TEST_RETURN::UNBALANCD;
			}
			_PrevBlack = black;
			leafIdx++;
			return TEST_RETURN::SUCCESS;
		}

		if (pNode->_color == BLACK)
		{
			black++;
		}
		else if (pNode->_color == RED)
		{
			red++;
			if (prevColor == RED)
			{
				printf("Double Red! leaf idx %d, data %d\n", leafIdx, pNode->_data);
				return TEST_RETURN::DOUBLE_RED;
			}
		}

		TEST_RETURN ret;
		ret = PreOrderTestNode(pNode->_pLeft, leafIdx, black, red, pNode->_color);
		if (ret != TEST_RETURN::SUCCESS) return ret;
		ret = PreOrderTestNode(pNode->_pRight, leafIdx, black, red, pNode->_color);
		if (ret != TEST_RETURN::SUCCESS) return ret;

		return TEST_RETURN::SUCCESS;
	}

public:
	void DrawAllNode(HDC hdc, HPEN black, HPEN red, int xPad, int yPad)
	{
		HPEN hPen = (HPEN)SelectObject(hdc, black);

		DrawNode(hdc, red, _pRoot,
			X_PIVOT, Y_PIVOT, X_PIVOT, Y_PIVOT,
			xPad, yPad, 0, 0, 0);
		SelectObject(hdc, hPen);
	}

private:
	void DrawNode(HDC hdc, HPEN red, 
		Node* pNode, int prevXPos, int prevYPos,
		int xPos, int yPos, int xPad, int yPad,
		int left, int right, int depth)
	{
		MoveToEx(hdc, prevXPos + xPad, prevYPos + yPad, NULL);
		LineTo(hdc, xPos + xPad, yPos + yPad);

		WCHAR text[TEXT_LEN];
		if (pNode == _pNil)
		{
			wcscpy_s(text, TEXT_LEN, L"nil");
		}
		else
		{
			_itow_s(pNode->_data, text, TEXT_LEN, 10);
		}

		if (pNode->_color == RED)
		{
			HPEN hPen = (HPEN)SelectObject(hdc, red);

			Ellipse(hdc,
				xPos - RADIUS + xPad,
				yPos - RADIUS + yPad,
				xPos + RADIUS + xPad,
				yPos + RADIUS + yPad);

			SelectObject(hdc, hPen);
		}
		else
		{
			Ellipse(hdc,
				xPos - RADIUS + xPad,
				yPos - RADIUS + yPad,
				xPos + RADIUS + xPad,
				yPos + RADIUS + yPad);
		}

		TextOutW(hdc,
			xPos - TEXT_PAD + xPad,
			yPos - TEXT_PAD + yPad,
			text, wcslen(text));

		if (pNode == _pNil) return;

		
		depth++;

		int prevXGap = abs(prevXPos - xPos);
		if (pNode == _pRoot)
			prevXGap = xPos;

		left++;
		DrawNode(hdc, red, pNode->_pLeft, xPos, yPos,
			xPos - (prevXGap / 2), yPos + Y_GAP,
			xPad, yPad, left, right, depth);
		left--;

		right++;
		DrawNode(hdc, red, pNode->_pRight, xPos, yPos,
			xPos + (prevXGap / 2), yPos + Y_GAP,
			xPad, yPad, left, right, depth);
		right--;

		depth--;
		return;
	}

public:
	Node* _pNil;
	Node* _pRoot;
	int _size = 0;

private:
	int _PrevBlack = 0;
};

