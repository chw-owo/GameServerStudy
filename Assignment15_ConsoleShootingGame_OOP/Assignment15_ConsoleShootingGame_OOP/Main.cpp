#include "SceneManager.h"

int main()
{
	while (1)
	{
		CSceneManager::GetInstance()->Run();
		if (CSceneManager::GetInstance()->_bExit)
			return 0;
	}

	return 0;
}

