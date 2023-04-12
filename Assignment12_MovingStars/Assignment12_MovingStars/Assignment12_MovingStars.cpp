#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include "MovingStars.h"

#define STAR_CNT 20
BaseObject* stars[STAR_CNT];

void KeyProcess();
void Update();
void Render();

int main()
{	
	while (1)
	{
		KeyProcess();
		Update();
		Render();
		Sleep(50);
	}
}

void KeyProcess()
{
	if (_kbhit())
	{
		char key = _getch();
		BaseObject* star = nullptr; 

		switch (key)
		{
		case '1':
			star = new OneStar;
			break;
		case '2':
			star = new TwoStar;
			break;
		case '3':
			star = new ThreeStar;
			break;
		default:
			return;
		}

		for (int i = 0; i < STAR_CNT; i++)
		{
			if (stars[i] == nullptr && star != nullptr)
			{
				star->Initialize();
				stars[i] = star;
				break;
			}
		}
	}
}

void Update()
{
	for (int i = 0; i < STAR_CNT; i++)
	{
		if (stars[i] != nullptr)
		{
			stars[i]->Update();
			if (stars[i]->CheckDead())
			{
				delete(stars[i]);
				stars[i] = nullptr;
			}
		}
	}
}

void Render()
{
	system("cls");
	for (int i = 0; i < STAR_CNT; i++)
	{
		if (stars[i] == nullptr)
			_tprintf(_T("\n"));
		else
			stars[i]->Render();
	}
}