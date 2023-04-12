#include "custom_new.h"

int main()
{
	int* p4 = new int;
	int* p400 = new int[100];
	delete[](p400);
	char* pZ1 = new char[50];
	char* pZ2 = new char[150];
	delete[](pZ1);
	char* pZ3 = new char[200];

	//delete(p4);
	delete[](p400);
	delete[](pZ2);
	//delete[](pZ3);
}
