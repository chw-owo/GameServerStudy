#include <iostream>
#include <conio.h>
#include <windows.h>

#define CNT 9
int g_Timing[CNT] = { 5, 10, 14, 17, 20, 25, 29, 31, 33 };

void TimingGame()
{
    float fLeftTime;
    DWORD dwStartTime = timeGetTime();
    float error[CNT] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    int idx = 0;

    while (1)
    {
        //logic section 
        fLeftTime = (timeGetTime() - dwStartTime) / (float)CLOCKS_PER_SEC;

        if (fLeftTime >= g_Timing[idx] + 1 && error[idx] == -1)
        {
            error[idx] = 1;
            idx++;
        }           

        if (idx >= CNT)
            break;
           
        if (_kbhit())
        {
            _getch();
            error[idx] = abs(fLeftTime - g_Timing[idx]);
            idx++;        
        }

        //render section
        system("cls");
        printf("%.3f sec\n\n", fLeftTime);
        for (int i = 0; i < CNT; i++)
        {
            if(error[i] == -1)
                printf("%u sec:\n", g_Timing[i]);

            else if(error[i] >= 0 && error[i] < 0.25)
                printf("%u sec: Great!\n", g_Timing[i]);

            else if (error[i] >= 0.25 && error[i] < 0.5)
                printf("%u sec: Good!\n", g_Timing[i]); 

            else if (error[i] >= 0.5 && error[i] < 0.75)
                printf("%u sec: Nogood\n", g_Timing[i]);

            else if (error[i] >= 0.75 && error[i] < 1)
                printf("%u sec: Bad\n", g_Timing[i]);

            else if (error[i] >= 1)
                printf("%u sec: Fail\n", g_Timing[i]);
        } 
    }
}

int main()
{
    timeBeginPeriod(1);
    TimingGame();
    timeEndPeriod(1);
}
