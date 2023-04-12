#include "MovingStars.h"

char oneStar[ONE + 1] = "*";
char twoStar[TWO + 1] = "**";
char threeStar[THREE + 1] = "***";

void Star::Update()
{ 
	_x += _interval;

	if (_x + _starSize >= MAX)
	{
		_dead = true;
		return;
	}
		
	int i = 0;

	for (; i < _x; i++)
		_line[i] = ' ';

	for (int j = 0; j < _starSize; j++)
	{
		_line[i] = _star[j];
		i++;
	}

	for (; i < MAX; i++)
		_line[i] = ' ';

	_line[i] = '\n';
}

void Star::Render()
{
	if (_dead == true)
		return;

	for (int i = 0; i <= MAX; i++)
		printf("%c", _line[i]);
}

bool Star::CheckDead()
{
	return _dead;
}

void OneStar::Initialize()
{
	_interval = ONE;
	_starSize = ONE;
	_star = oneStar;
}

void TwoStar::Initialize()
{
	_interval = TWO;
	_starSize = TWO;
	_star = twoStar;
}

void ThreeStar::Initialize()
{
	_interval = THREE;
	_starSize = THREE;
	_star = threeStar;
}