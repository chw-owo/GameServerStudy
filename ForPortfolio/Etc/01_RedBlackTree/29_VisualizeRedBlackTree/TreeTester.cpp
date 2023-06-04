#include "TreeTester.h"
#include <list> 

#define INPUT_LEN 16
#define CHECKPOINT 5000000

TreeTester::TreeTester()
{
    srand(500);
    printf("This is Red Black Tree Tester!\n");
    PrintMenu();
    _hBlackPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    _hRedPen = CreatePen(PS_SOLID, 1, RGB(200, 0, 0));
} 

TreeTester::~TreeTester()
{
}

void TreeTester::SearchNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("Enter Number to Search\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int num = atoi(nodeInput);

    bool ret = false;
    if (_bCompareMode)
    {
        PRO_BEGIN(L"RBT SEARCH");
        ret = _RedBlackTree.SearchNode(num);
        PRO_END(L"RBT SEARCH");

        PRO_BEGIN(L"BST SEARCH");
        _BinaryTree.SearchNode(num);
        PRO_END(L"BST SEARCH");
    }
    else
    {
        ret = _RedBlackTree.SearchNode(num);
    }

    if (ret)
    {
        printf("Success to Search %d\n", num);
    }
    else
    {
        printf("There is no %d\n", num);
    }

    PrintMenu();
}

void TreeTester::PrintPathData()
{
    _RedBlackTree.PrintAllPath();
    PrintMenu();
}

void TreeTester::PrintNodeData()
{
    int max = _RedBlackTree.GetTreeSize();
    int* data = new int[max];
    _RedBlackTree.GetAllNode(data);
    printf("\ntotal size: %d\n", max);
    for (int i = 0; i < max; i++)
        printf("%d ", data[i]);
    printf("\n");

    PrintMenu();
}

void TreeTester::InsertNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("Enter Start Number to Insert\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int startNum = atoi(nodeInput);

    memset(nodeInput, '\0', INPUT_LEN);
    printf("Enter End Number to Insert\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int endNum = atoi(nodeInput);

    if (startNum < 0 || startNum > INT_MAX) startNum = 0;
    if (endNum < 0 || endNum > INT_MAX) endNum = INT_MAX;

    printf("Requested Range: %d~%d\n", startNum, endNum);

    list<int> dupList;
    INSERT_NODE_RETURN ret;
    for (int i = startNum; i <= endNum; i++)
    {
        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT INSERT");
            ret = _RedBlackTree.InsertNode(i);
            PRO_END(L"RBT INSERT");

            PRO_BEGIN(L"BST INSERT");
            _BinaryTree.InsertNode(i);
            PRO_END(L"BST INSERT");
        }
        else
        {
            ret = _RedBlackTree.InsertNode(i);
        }
        
        if (ret == INSERT_NODE_RETURN::TREE_IS_FULL)
        {
            printf("Tree is full!\n" 
                "Cur Data: %d, Success Count: %d, Fail Count: %d\n", 
                i, i - startNum, endNum - i + 1);
            break;
        }
        else if (ret == INSERT_NODE_RETURN::DUPLICATE_VALUE)
        {
            dupList.push_back(i);
        }
        else if (ret == INSERT_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Unknown Error!\n"
                "Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                i, i - startNum, endNum - i + 1);
            break;
        }
    }
    
    printf("Duplicate Data Count: %llu\n", dupList.size());
    /*
    list<int>::iterator iter = dupList.begin();
    for (; iter != dupList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */

    PrintMenu();
}

void TreeTester::InsertRandomNodeUnder9999()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("Enter Node Count to Insert (MAX: %d)\n", 9999);
    fgets(nodeInput, INPUT_LEN, stdin);
    int node = atoi(nodeInput);

    if (node > 9999 || node < 0)
        node = 9999;

    printf("Requested Count: %d\n", node);

    list<int> dupList;
    INSERT_NODE_RETURN ret;
    for (int i = 0; i < node; i++)
    {
        int data = rand() % 9999;
        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT INSERT");
            ret = _RedBlackTree.InsertNode(data);
            PRO_END(L"RBT INSERT");

            PRO_BEGIN(L"BST INSERT");
            _BinaryTree.InsertNode(data);
            PRO_END(L"BST INSERT");
        }
        else
        {
            ret = _RedBlackTree.InsertNode(data);
        }

        if (ret == INSERT_NODE_RETURN::TREE_IS_FULL)
        {
            printf("Tree is full!\n"
                "Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                data, i, node - i + 1);
            break;
        }
        else if (ret == INSERT_NODE_RETURN::DUPLICATE_VALUE)
        {
            dupList.push_back(data);
        }
        else if (ret == INSERT_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Unknown Error!\n"
                "Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                data, i, node - i + 1);
            break;
        }
    }

    printf("Duplicate Data Count: %llu\n", dupList.size());
    /*
    list<int>::iterator iter = dupList.begin();
    for (; iter != dupList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */

    PrintMenu();
}

void TreeTester::InsertRandomNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("Enter Node Count to Insert (MAX: %d)\n", INT_MAX);
    fgets(nodeInput, INPUT_LEN, stdin);
    int node = atoi(nodeInput);

    if (node > INT_MAX|| node < 0)
        node = INT_MAX;

    printf("Requested Count: %d\n", node);

    list<int> dupList;
    INSERT_NODE_RETURN ret;
    for (int i = 0; i < node; i++)
    {
        int rand1 = rand();
        int rand2 = rand();
        rand1 = rand1 << 7;
        rand1 |= rand2;

        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT INSERT");
            ret = _RedBlackTree.InsertNode(rand1);
            PRO_END(L"RBT INSERT");

            PRO_BEGIN(L"BST INSERT");
            _BinaryTree.InsertNode(rand1);
            PRO_END(L"BST INSERT");
        }
        else
        {
            ret = _RedBlackTree.InsertNode(rand1);
        }

        if (ret == INSERT_NODE_RETURN::TREE_IS_FULL)
        {
            printf("Tree is full!\n"
                "Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                rand1, i, node - i + 1);
            break;
        }
        else if (ret == INSERT_NODE_RETURN::DUPLICATE_VALUE)
        {
            dupList.push_back(rand1);
        }
        else if (ret == INSERT_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Unknown Error!\n"
                "Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                rand1, i, node - i + 1);
            break;
        }
    }

    printf("Duplicate Data Count: %llu\n", dupList.size());
    /*
    list<int>::iterator iter = dupList.begin();
    for (; iter != dupList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */
    PrintMenu();
}

void TreeTester::DeleteNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("Enter Start Number to Delete\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int startNum = atoi(nodeInput);

    memset(nodeInput, '\0', INPUT_LEN);
    printf("Enter End Number to Delete\n");
    fgets(nodeInput, INPUT_LEN, stdin);
    int endNum = atoi(nodeInput);

    if (startNum < 0 || startNum > INT_MAX) startNum = 0;
    if (endNum < 0 || endNum > INT_MAX) endNum = INT_MAX;
    
    list<int> cantFindList;
    DELETE_NODE_RETURN ret;
    for(int i = startNum; i <= endNum; i++)
    {
        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT DELETE");
            ret = _RedBlackTree.DeleteNode(i);
            PRO_END(L"RBT DELETE");

            PRO_BEGIN(L"BST DELETE");
            _BinaryTree.DeleteNode(i);
            PRO_END(L"BST DELETE");
        }
        else
        {
            ret = _RedBlackTree.DeleteNode(i);
        }

        if (ret == DELETE_NODE_RETURN::CANT_FIND)
        {
            cantFindList.push_back(i);
        }
        else if (ret == DELETE_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Unknown Error!\n");
            printf("Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                i, i - startNum, endNum - i + 1);
            break;
        }
    }

    printf("Can't Find Data Count: %llu\n", cantFindList.size());
    printf("Success Count: %llu\n", (unsigned __int64)(endNum - startNum + 1) - cantFindList.size());
    /*list<int>::iterator iter = cantFindList.begin();
    for (; iter != cantFindList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */
    

    PrintMenu();
}

void TreeTester::DeleteRandomNode()
{
    char nodeInput[INPUT_LEN] = { '\0', };
    printf("Enter Node Count to Delete (MAX: %d)\n", 
        _RedBlackTree.GetTreeSize());
    fgets(nodeInput, INPUT_LEN, stdin);
    int node = atoi(nodeInput);

    if (node > _RedBlackTree.GetTreeSize() || node < 0)
        node = _RedBlackTree.GetTreeSize();

    printf("Requested Count: %d\n", node);

    int max = _RedBlackTree.GetTreeSize();
    int* data = new int[max];
    _RedBlackTree.GetAllNode(data);

    list<int> cantFindList;
    DELETE_NODE_RETURN ret;

    if (node <= RAND_MAX)
    {
        for (int i = 0; i < node; i++)
        {
            int rand1 = (rand() % max);
            while (data[rand1] == -1)
            {
                rand1 = (rand() % max);
            }

            if (_bCompareMode)
            {
                PRO_BEGIN(L"RBT DELETE");
                ret = _RedBlackTree.DeleteNode(data[rand1]);
                PRO_END(L"RBT DELETE");

                PRO_BEGIN(L"BST DELETE");
                _BinaryTree.DeleteNode(data[rand1]);
                PRO_END(L"BST DELETE");
            }
            else
            {
                ret = _RedBlackTree.DeleteNode(data[rand1]);
            }

            data[rand1] = -1;

            if (ret == DELETE_NODE_RETURN::CANT_FIND)
            {
                cantFindList.push_back(rand1);
            }
            else if (ret == DELETE_NODE_RETURN::UNKNOWN_ERROR)
            {
                printf("Unknown Error!\n"
                    "Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                    rand1, i, node - i + 1);
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < node; i++)
        {
            int rand1 = rand();
            int rand2 = rand();
            rand1 = rand1 << 7;
            rand1 |= rand2;
            rand1 %= max;

            while (data[rand1] == -1)
            {
                rand1 = rand();
                rand2 = rand();
                rand1 = rand1 << 7;
                rand1 |= rand2;
                rand1 %= max;
            }

            if (_bCompareMode)
            {
                PRO_BEGIN(L"RBT DELETE");
                ret = _RedBlackTree.DeleteNode(data[rand1]);
                PRO_END(L"RBT DELETE");

                PRO_BEGIN(L"BST DELETE");
                _BinaryTree.DeleteNode(data[rand1]);
                PRO_END(L"BST DELETE");
            }
            else
            {
                ret = _RedBlackTree.DeleteNode(data[rand1]);
            }

            data[rand1] = -1;

            if (ret == DELETE_NODE_RETURN::CANT_FIND)
            {
                cantFindList.push_back(rand1);
            }
            else if (ret == DELETE_NODE_RETURN::UNKNOWN_ERROR)
            {
                printf("Unknown Error!\n"
                    "Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                    rand1, i, node - i + 1);
                break;
            }
        }
    }

    delete[] data;

    printf("Can't Find Data Count: %llu\n", cantFindList.size());

    /*
    list<int>::iterator iter = cantFindList.begin();
    for (; iter != cantFindList.end(); iter++)
    {
        printf("%d ", *iter);
    }
    printf("\n");
    */
    PrintMenu();
}

void TreeTester::TestTree()
{
    printf("Press Enter to Stop Test\n");
    _RedBlackTree.DeleteAllNode();
    set<int> testSet;

    int insertRand;
    int insertDataAcc = 0;
    int insertDataCnt = 0;

    int deleteRand;
    int deleteDataAcc = 0;
    int deleteDataCnt = 0;

    int idx = 0;

    while (1)
    {
        idx++;

        if (GetAsyncKeyState(VK_RETURN))
            break;

        insertRand = rand();
        if (!InsertForTest(insertRand, testSet)) break;
        if (!GetTreeDataForTest(testSet))  break;
        insertDataCnt += insertRand;

        deleteRand = rand();
        if (!DeleteForTest(deleteRand, testSet)) break;
        if (!GetTreeDataForTest(testSet))  break;
        deleteDataCnt += deleteRand;

        if (insertDataCnt > CHECKPOINT)
        {
            insertDataCnt -= CHECKPOINT;
            insertDataAcc++;
            printf("Insert Success: %d\n", 
                insertDataAcc * CHECKPOINT + insertDataCnt);
        }

        if (deleteDataCnt > CHECKPOINT)
        {
            deleteDataCnt -= CHECKPOINT;
            deleteDataAcc++;
            printf("Delete Success: %d\n",
                deleteDataAcc * CHECKPOINT + deleteDataCnt);
        }
    }

    printf( "\n<Test Result>\n"
            "Loop Count: %d\n"
            "Insert Success: %d\n"
            "Delete Success: %d\n\n",
            idx,
            insertDataAcc * CHECKPOINT + insertDataCnt,
            deleteDataAcc * CHECKPOINT + deleteDataCnt
    );

    PrintMenu();
}

void TreeTester::SetCompareMode()
{
    char input[INPUT_LEN] = { '\0', };
    printf("Press 0 to Compare Mode [OFF], 1 to Compare Mode [ON]\n");
    fgets(input, INPUT_LEN, stdin);
    int num = atoi(input);

    if (_bCompareMode && num == 0)
    {
        _bCompareMode = false;
        _bDrawRedBlackTree = true;
        printf("Now Compare Mode [OFF]\n");
    }
    else if (!_bCompareMode && num == 1)
    {
        _bCompareMode = true;
        _BinaryTree.DeleteAllNode();
        _BinaryTree.CopyRedBlackTree(&_RedBlackTree);
        printf("Now Compare Mode [ON]\n");
    }
    else if (!_bCompareMode && num == 0)
    {
        printf("Already Compare Mode [OFF]\n");
    }
    else if (_bCompareMode && num == 1)
    {
        printf("Already Compare Mode [ON]\n");

    }
    else
    {
        printf("Wrong Input!\n");
    }

    PrintMenu();
}

void TreeTester::PrintCompareResult()
{
    if (!_bCompareMode) return;

    PRO_PRINT_OUT();
    PRO_OUT(L"output.txt");
    PrintMenu();
}

void TreeTester::ShiftTreeDraw()
{
    if (!_bCompareMode) return;

    if (_bDrawRedBlackTree)
    {
        _bDrawRedBlackTree = false;
    }
    else
    {
        _bDrawRedBlackTree = true;
    }
}

void TreeTester::DrawTree(HDC hdc)
{
    if(_bDrawRedBlackTree)
    {
        _RedBlackTree.DrawAllNode(
            hdc, _hBlackPen, _hRedPen, _iXPad, _iYPad);
    }
    else
    {
        _BinaryTree.DrawAllNode(hdc, _iXPad, _iYPad);
    }
}

void TreeTester::PrintMenu()
{
    if(_bCompareMode)
    {
        printf("\n1. Search Node\n"
            "2. Insert Node\n"
            "3. Insert Random Node (under %d)\n"
            "4. Insert Random Node (under %d)\n"
            "5. Delete Node\n"
            "6. Delete Random Node\n"
            "7. Print Node Data\n"
            "8. Print Path Data\n"
            "9. Test Tree\n"
            "0. Set Compare Mode with Basic Tree [now: ON]\n"
            "Q. Print Compare Result with Basic Tree\n"
            "W. Shift Tree Drawing\n"
            "ESC. Clear Console\n\n"
            "Choose number\n",
            9999, INT_MAX);
    }
    else
    {
        printf("\n1. Search Node\n"
            "2. Insert Node\n"
            "3. Insert Random Node (under %d)\n"
            "4. Insert Random Node (under %d)\n"
            "5. Delete Node\n"
            "6. Delete Random Node\n"
            "7. Print Node Data\n"
            "8. Print Path Data\n"
            "9. Test Tree\n"
            "0. Set Compare Mode with Basic Tree [now: OFF]\n"        
            "ESC. Clear Console\n\n"
            "Choose number\n",
            9999, INT_MAX);
    }

}

bool TreeTester::GetTreeDataForTest(set<int>& testSet)
{
    if(_RedBlackTree.TestAllNode() != TEST_RETURN::SUCCESS)
    {
        return false;
    }

    int treeSize = _RedBlackTree.GetTreeSize();
    int* data = new int[treeSize];
    _RedBlackTree.GetAllNode(data);

    if (treeSize != testSet.size())
    {
        printf("data count is different! tree: %d, test set: %llu\n",
            treeSize, testSet.size());
    }

    int idx = 0;
    set<int>::iterator iter = testSet.begin();
    for (; iter != testSet.end(); iter++) {
        
        if (data[idx] != *iter)
        {
            printf("data is different! tree: %d, test set: %d\n",
                data[idx], *iter);
            return false;
        }

        idx++;
    }

    return true;
}

bool TreeTester::InsertForTest(int count, set<int> &testSet)
{
    INSERT_NODE_RETURN ret;
    for (int i = 0; i < count; i++)
    {
        int rand1 = rand();
        int rand2 = rand();
        rand1 = rand1 << 7;
        rand1 |= rand2;

        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT INSERT");
            ret = _RedBlackTree.InsertNode(rand1);
            PRO_END(L"RBT INSERT");

            PRO_BEGIN(L"BST DELETE");
            _BinaryTree.InsertNode(rand1);
            PRO_END(L"BST DELETE");
        }
        else
        {
            ret = _RedBlackTree.InsertNode(rand1);
        }

        if (ret == INSERT_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Insert Unknown Error! Cur Data: %d\n", rand1);
            return false;
        }
        else if (ret == INSERT_NODE_RETURN::TREE_IS_FULL)
        {
            break;
        }

        testSet.insert(rand1);
    }
    return true;
}

bool TreeTester::DeleteForTest(int count, set<int>& testSet)
{
    int max = _RedBlackTree.GetTreeSize();
    int* data = new int[max];
    _RedBlackTree.GetAllNode(data);

    if (count > max) count = max;
    DELETE_NODE_RETURN ret;
    for (int i = 0; i < count; i++)
    {
        int rand1 = rand();
        int rand2 = rand();
        rand1 = rand1 << 7;
        rand1 |= rand2;
        rand1 %= max;

        while (data[rand1] == -1)
        {
            rand1 = rand();
            rand2 = rand();
            rand1 = rand1 << 7;
            rand1 |= rand2;
            rand1 %= max;
        }

        if (_bCompareMode)
        {
            PRO_BEGIN(L"RBT DELETE");
            ret = _RedBlackTree.DeleteNode(data[rand1]);
            PRO_END(L"RBT DELETE");

            PRO_BEGIN(L"BST DELETE");
            _BinaryTree.DeleteNode(data[rand1]);
            PRO_END(L"BST DELETE");
        }
        else
        {
            ret = _RedBlackTree.DeleteNode(data[rand1]);
        }

        int deleted = data[rand1];
        data[rand1] = -1;

        if (ret == DELETE_NODE_RETURN::UNKNOWN_ERROR)
        {
            printf("Delete Unknown Error!: %d\n", rand1);
            delete[] data;
            return false;
        }

        testSet.erase(deleted);
    }

    delete[] data;
    return true;
}
