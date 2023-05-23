#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneLoad.h"
#include "SceneGame.h"
#include "SceneOver.h"
#include "SceneClear.h"

CSceneManager::CSceneManager()
{
	_pScene = new CSceneTitle;
}

CSceneManager::~CSceneManager()
{

}

CSceneManager* CSceneManager::GetInstance(void)
{
	static CSceneManager _SceneMgr;
	return &_SceneMgr;
}

void CSceneManager::Run() 
{
	_pScene->Update();

	if (_pScene->_bExit)
	{
		_bExit = _pScene->_bExit;
		return;
	}
	else if (_pScene->_SceneType != _SceneType)
	{
		_SceneType = _pScene->_SceneType;
		LoadScene(_SceneType);
	}
}

void CSceneManager::LoadScene(CSceneBase::SceneType scene)
{
	delete _pScene;

	switch (scene)
	{
	case CSceneBase::TITLE:
		_pScene = new CSceneTitle;
		break;

	case CSceneBase::LOAD:
		_pScene = new CSceneLoad;
		break;

	case CSceneBase::GAME:
		_pScene = new CSceneGame;
		break;

	case CSceneBase::OVER:
		_pScene = new CSceneOver;
		break;

	case CSceneBase::CLEAR:
		_pScene = new CSceneClear;
		break;
	}
}

