#include "LinkedList.cpp"
#include <stdio.h>

class Player
{
public:
	Player(int ID) : _ID(ID) {}
	int GetID() { return _ID; }

private:
	int _ID;
};


int main()
{
	List<Player*> ListPlayer;
	ListPlayer.push_back(new Player(0));
	ListPlayer.push_back(new Player(1));
	ListPlayer.push_back(new Player(2));

	List<Player*>::iterator iter;
	for (iter = ListPlayer.begin(); iter != ListPlayer.end(); )
	{
		printf("%d\n", (*iter)->GetID());
		if ((*iter)->GetID() == 1)
		{
			iter = ListPlayer.erase(iter);
		}
		else
		{
			++iter;
		}
	}
	printf("size: %d\n", ListPlayer.size());
	printf("empty: %d\n", ListPlayer.empty());
	
	printf("======================================\n");

	ListPlayer.pop_front();
	iter = ListPlayer.begin();
	for (; iter != ListPlayer.end(); ++iter)
	{
		printf("%d\n", (*iter)->GetID());
	}
	printf("size: %d\n", ListPlayer.size());
	printf("empty: %d\n", ListPlayer.empty());

	printf("======================================\n");

	ListPlayer.clear();
	iter = ListPlayer.begin();
	for (; iter != ListPlayer.end(); ++iter)
	{
		printf("%d\n", (*iter)->GetID());
	}

	printf("size: %d\n", ListPlayer.size());
	printf("empty: %d\n", ListPlayer.empty());
}

