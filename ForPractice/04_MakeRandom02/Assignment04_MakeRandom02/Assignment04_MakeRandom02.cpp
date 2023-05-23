#include <iostream>
#include <conio.h>

#define CNT 7

struct st_ITEM
{
	char	Name[30];
	int		Rate;	// 아이템이 뽑힐 비율
};

st_ITEM g_Gatcha[CNT] = {
			{"칼",		20},
			{"방패",		20},
			{"신발",		20},
			{"물약",		20},
			{"초강력무기",	5},
			{"초강력방패",		5},
			{"초강력레어레어레어신발", 1}
};

void Gatcha()
{
	int rate[CNT] = { };
	int rateAccum[CNT] = {};

	int total = 0;
	int iRand;
	int idx;
	int i;

	while (1)
	{
		if (_kbhit())
		{
			//logic section
			_getch();

			if (total <= 0)
			{
				for (i = 0; i < CNT; i++)
				{
					rate[i] = g_Gatcha[i].Rate;
				}
				idx = 0;
			}

			total = 0;
			for (i = 0; i < CNT; i++)
			{
				total += rate[i];
				rateAccum[i] = total;
			}

			iRand = rand() % (total + 1);

			for (i = 0; i < CNT; i++)
			{
				if (iRand <= rateAccum[i])
				{
					rate[i]--;
					break;
				}
			}

			//render section       
			printf("%d : %s (origin: %d / left: %d)\n", idx, g_Gatcha[i].Name, g_Gatcha[i].Rate, rate[i]);
			idx++;
		}
	}
}

int main()
{
	Gatcha();
	return 0;
}
