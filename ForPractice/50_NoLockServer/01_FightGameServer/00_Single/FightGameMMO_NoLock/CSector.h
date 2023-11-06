#pragma once
#include "Protocol.h"
#include <vector>
#include <synchapi.h>
using namespace std;

class CPlayer;
class CSector
{
public:
	inline void InitializeSector(int idx, short xIndex, short yIndex)
	{
		_idx = idx;
		_xIndex = xIndex;
		_yIndex = yIndex;

		if (_xIndex < 2 || _xIndex >= (dfSECTOR_CNT_X - 2) ||
			_yIndex < 2 || _yIndex >= (dfSECTOR_CNT_Y - 2))
			return;

		_xPosMin = (xIndex - 2) * dfSECTOR_SIZE_X;
		_yPosMin = (yIndex - 2) * dfSECTOR_SIZE_Y;
		_xPosMax = (xIndex - 1) * dfSECTOR_SIZE_X;
		_yPosMax = (yIndex - 1) * dfSECTOR_SIZE_Y;
		_players.reserve(dfPLAYER_PER_SECTOR);
	}

public:
	int _idx;
	short _xIndex;
	short _yIndex;
	short _xPosMin;
	short _yPosMin;
	short _xPosMax;
	short _yPosMax;
	vector<CPlayer*> _players;

public:
	CSector* _llNew[dfVERT_SECTOR_NUM];
	CSector* _luNew[dfDIAG_SECTOR_NUM];
	CSector* _uuNew[dfVERT_SECTOR_NUM];
	CSector* _ruNew[dfDIAG_SECTOR_NUM];
	CSector* _rrNew[dfVERT_SECTOR_NUM];
	CSector* _rdNew[dfDIAG_SECTOR_NUM];
	CSector* _ddNew[dfVERT_SECTOR_NUM];
	CSector* _ldNew[dfDIAG_SECTOR_NUM];

	CSector* _llOld[dfVERT_SECTOR_NUM];
	CSector* _luOld[dfDIAG_SECTOR_NUM];
	CSector* _uuOld[dfVERT_SECTOR_NUM];
	CSector* _ruOld[dfDIAG_SECTOR_NUM];
	CSector* _rrOld[dfVERT_SECTOR_NUM];
	CSector* _rdOld[dfDIAG_SECTOR_NUM];
	CSector* _ddOld[dfVERT_SECTOR_NUM];
	CSector* _ldOld[dfDIAG_SECTOR_NUM];

public:
	CSector* _around[dfAROUND_SECTOR_NUM];
	CSector** _new[dfMOVE_DIR_MAX] =
	{
		_llNew, _luNew, _uuNew, _ruNew,
		_rrNew, _rdNew, _ddNew, _ldNew
	};

	CSector** _old[dfMOVE_DIR_MAX] =
	{
		_llOld, _luOld, _uuOld, _ruOld,
		_rrOld, _rdOld, _ddOld, _ldOld
	};
};

