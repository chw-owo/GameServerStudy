#include "BasicBinaryTree.h"
#include <stdio.h>

int main()
{
    BasicBinaryTree<int> tree;
    tree.InsertNode(3);
    tree.PrintAllNode();
    tree.InsertNode(8);
    tree.PrintAllNode();
    tree.InsertNode(1);
    tree.PrintAllNode();
    tree.InsertNode(5);
    tree.PrintAllNode();
    tree.InsertNode(2);
    tree.PrintAllNode();
    tree.InsertNode(7);
    tree.PrintAllNode();
    tree.InsertNode(4);
    tree.PrintAllNode();
    tree.InsertNode(9);
    tree.PrintAllNode();
    tree.InsertNode(6);
    tree.PrintAllNode();

    int* r5 = tree.SearchNode(5);
    int* r3 = tree.SearchNode(3);
    int* r9 = tree.SearchNode(9);
    if (r3 != nullptr)  printf("%d", *r3);
    if (r5 != nullptr)  printf("%d", *r5);
    if (r9 != nullptr)  printf("%d", *r9);

    printf("\n\n");

    tree.DeleteNode(8);
    tree.PrintAllNode();
    printf("\n");
    tree.DeleteNode(3);
    tree.PrintAllNode();
    printf("\n");
    tree.DeleteNode(1);
    tree.PrintAllNode();
    printf("\n");
    tree.DeleteNode(5);
    tree.PrintAllNode();
    printf("\n");
    tree.DeleteNode(2);
    tree.PrintAllNode();
    printf("\n");
    tree.DeleteNode(7);
    tree.PrintAllNode();
    printf("\n");
    tree.DeleteNode(4);
    tree.PrintAllNode();
    printf("\n");
    tree.DeleteNode(9);
    tree.PrintAllNode();
    printf("\n");
    tree.DeleteNode(6);
    tree.PrintAllNode();
    printf("\n");

    r5 = tree.SearchNode(5);
}
