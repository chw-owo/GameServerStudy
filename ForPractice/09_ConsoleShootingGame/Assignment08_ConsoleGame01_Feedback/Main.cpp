#include "Console.h"
#include "Scenes.h"

#include "Title.h"
#include "Load.h"
#include "Game.h"
#include "Clear.h"
#include "Over.h"

#include <iostream>

int main()
{
	cs_Initial();
	InitialSceneData();
	InitialStageData();

	while (1)
	{
		switch (g_Scene)
		{
		case TITLE:
			UpdateTitle();
			break;

		case LOAD:
			UpdateLoad();
			break;

		case GAME:
			UpdateGame();
			break;

		case CLEAR:
			UpdateClear();
			break;

		case OVER:
			UpdateOver();
			break;
		}

		if (g_exit == true)
			break;
	}

	return 0;
}
