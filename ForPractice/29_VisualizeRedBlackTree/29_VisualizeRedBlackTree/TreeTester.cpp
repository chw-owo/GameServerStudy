#include "TreeTester.h"
#include <list> 

#define INPUT_LEN 16
#define CHECKPOINT 5000000
TreeTester::TreeTester()
{
    srand(500);
    PrintMenu();
    _hBlackPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    _hRedPen = CreatePen(PS_SOLID, 1, RGB(200, 0, 0));
} 

TreeTester::~TreeTester()
{
}

void TreeTester::GetLeafData()
{
    _RedBlackTree.PrintAllNodeData();
    PrintMenu();
}

void TreeTester::GetTreeData()
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
    RedBlackTree<int>::INSERT_NODE ret;
    for (int i = startNum; i <= endNum; i++)
    {
        ret = _RedBlackTree.InsertNode(i);

        if (ret == RedBlackTree<int>::INSERT_NODE::TREE_IS_FULL)
        {
            printf("Tree is full!\n" 
                "Cur Data: %d, Success Count: %d, Fail Count: %d\n", 
                i, i - startNum, endNum - i + 1);
            break;
        }
        else if (ret == RedBlackTree<int>::INSERT_NODE::DUPLICATE_VALUE)
        {
            dupList.push_back(i);
        }
        else if (ret == RedBlackTree<int>::INSERT_NODE::UNKNOWN_ERROR)
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
    RedBlackTree<int>::INSERT_NODE ret;
    for (int i = 0; i < node; i++)
    {
        int data = rand() % 9999;
        ret = _RedBlackTree.InsertNode(data);

        if (ret == RedBlackTree<int>::INSERT_NODE::TREE_IS_FULL)
        {
            printf("Tree is full!\n"
                "Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                data, i, node - i + 1);
            break;
        }
        else if (ret == RedBlackTree<int>::INSERT_NODE::DUPLICATE_VALUE)
        {
            dupList.push_back(data);
        }
        else if (ret == RedBlackTree<int>::INSERT_NODE::UNKNOWN_ERROR)
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
    RedBlackTree<int>::INSERT_NODE ret;
    for (int i = 0; i < node; i++)
    {
        int rand1 = rand();
        int rand2 = rand();
        rand1 = rand1 << 7;
        rand1 |= rand2;
        ret = _RedBlackTree.InsertNode(rand1);

        if (ret == RedBlackTree<int>::INSERT_NODE::TREE_IS_FULL)
        {
            printf("Tree is full!\n"
                "Cur Data: %d, Success Count: %d, Fail Count: %d\n",
                rand1, i, node - i + 1);
            break;
        }
        else if (ret == RedBlackTree<int>::INSERT_NODE::DUPLICATE_VALUE)
        {
            dupList.push_back(rand1);
        }
        else if (ret == RedBlackTree<int>::INSERT_NODE::UNKNOWN_ERROR)
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
    RedBlackTree<int>::DELETE_NODE ret;
    for(int i = startNum; i <= endNum; i++)
    {
        ret = _RedBlackTree.DeleteNode(i);

        if (ret == RedBlackTree<int>::DELETE_NODE::CANT_FIND)
        {
            cantFindList.push_back(i);
        }
        else if (ret == RedBlackTree<int>::DELETE_NODE::UNKNOWN_ERROR)
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
    RedBlackTree<int>::DELETE_NODE ret;

    if (node <= RAND_MAX)
    {
        for (int i = 0; i < node; i++)
        {
            int rand1 = (rand() % max);
            while (data[rand1] == -1)
            {
                rand1 = (rand() % max);
            }
            ret = _RedBlackTree.DeleteNode(data[rand1]);
            data[rand1] = -1;

            if (ret == RedBlackTree<int>::DELETE_NODE::CANT_FIND)
            {
                cantFindList.push_back(rand1);
            }
            else if (ret == RedBlackTree<int>::DELETE_NODE::UNKNOWN_ERROR)
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

            ret = _RedBlackTree.DeleteNode(data[rand1]);
            data[rand1] = -1;

            if (ret == RedBlackTree<int>::DELETE_NODE::CANT_FIND)
            {
                cantFindList.push_back(rand1);
            }
            else if (ret == RedBlackTree<int>::DELETE_NODE::UNKNOWN_ERROR)
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

void TreeTester::DrawTree(HDC hdc)
{
    _RedBlackTree.DrawAllNode(
        hdc, _hBlackPen, _hRedPen, _iXPad, _iYPad);
}

void TreeTester::PrintMenu()
{
    printf("\n0. Get Tree Data\n"
        "1. Get Leaf Data\n"
        "2. Insert Node\n"
        "3. Insert Random Node (under %d)\n"
        "4. Insert Random Node (under %d)\n"
        "5. Delete Node\n"
        "6. Delete Random Node\n"
        "7. Test Tree\n"
        "8. Clear Console\n\n"
        "Choose number\n",
        9999, INT_MAX);
}

bool TreeTester::GetTreeDataForTest(set<int>& testSet)
{
    if(_RedBlackTree.TestAllNode() != RedBlackTree<int>::TEST::SUCCESS)
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
    RedBlackTree<int>::INSERT_NODE ret;
    for (int i = 0; i < count; i++)
    {
        int rand1 = rand();
        int rand2 = rand();
        rand1 = rand1 << 7;
        rand1 |= rand2;
        ret = _RedBlackTree.InsertNode(rand1);

        if (ret == RedBlackTree<int>::INSERT_NODE::UNKNOWN_ERROR)
        {
            printf("Insert Unknown Error! Cur Data: %d\n", rand1);
            return false;
        }
        else if (ret == RedBlackTree<int>::INSERT_NODE::TREE_IS_FULL)
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
    RedBlackTree<int>::DELETE_NODE ret;
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

        ret = _RedBlackTree.DeleteNode(data[rand1]);
        int deleted = data[rand1];
        data[rand1] = -1;

        if (ret == RedBlackTree<int>::DELETE_NODE::UNKNOWN_ERROR)
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
