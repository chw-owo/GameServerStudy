#pragma once
#include <Windows.h>
#include <process.h>  
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#pragma comment(lib, "winmm.lib")

#include "matplotlibcpp.h"
namespace plt_alloc = matplotlibcpp;
namespace plt_free = matplotlibcpp;

#include "CTlsPool.h"
#include "Data.h"

#define dfTEST_CNT 2000
#define dfTHREAD_CNT 4
#define dfTEST_TOTAL_CNT 10000
#define dfNODE_MAX 12000
#define MS_PER_SEC 1000000000

double TlsPoolAllocTotals[dfTHREAD_CNT];
double TlsPoolFreeTotals[dfTHREAD_CNT];
double NewTotals[dfTHREAD_CNT];
double DeleteTotals[dfTHREAD_CNT];
HANDLE beginThreadComplete = nullptr;

std::vector<int> blockSizes;
std::vector<double> TlsPoolAllocDurations;
std::vector<double> TlsPoolFreeDurations;
std::vector<double> NewDurations;
std::vector<double> DeleteDurations;

struct argForThread
{
    argForThread(size_t pool, int idx)
        : _pool(pool), _idx(idx) {};
    size_t _pool;
    int _idx;
};

template <typename T>
unsigned __stdcall TlsPoolThread(void* arg)
{
    argForThread* input = (argForThread*)arg;
    CTlsPool<T>* pool = (CTlsPool<T>*)input->_pool;
    int idx = input->_idx;
    pool->Initialize();

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    double newMax = 0;
    double deleteMax = 0;
    double interval = 0;

    TlsPoolAllocTotals[idx] = 0;
    TlsPoolFreeTotals[idx] = 0;
    T* data[dfTEST_CNT] = { nullptr, };

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        data[i] = pool->Alloc();
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > newMax) newMax = interval;
        TlsPoolAllocTotals[idx] += interval;
    }
    TlsPoolAllocTotals[idx] -= newMax;

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        pool->Free(data[i]);
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > deleteMax) deleteMax = interval;
        TlsPoolFreeTotals[idx] += interval;
    }
    TlsPoolFreeTotals[idx] -= deleteMax;

    return 0;
}

template <typename T>
unsigned __stdcall NewDeleteThread(void* arg)
{
    int idx = (int)arg;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    double newMax = 0;
    double deleteMax = 0;
    double interval = 0;

    NewTotals[idx] = 0;
    DeleteTotals[idx] = 0;
    T* data[dfTEST_CNT] = { nullptr, };

    QueryPerformanceFrequency(&freq);
    WaitForSingleObject(beginThreadComplete, INFINITE);

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        data[i] = new T;
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > newMax) newMax = interval;
        NewTotals[idx] += interval;
    }
    NewTotals[idx] -= newMax;

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        QueryPerformanceCounter(&start);
        delete data[i];
        QueryPerformanceCounter(&end);

        interval = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
        if (interval > deleteMax) deleteMax = interval;
        DeleteTotals[idx] += interval;
    }
    DeleteTotals[idx] -= deleteMax;

    return 0;
}

template <typename T>
void comparePerformance()
{
    // Test TlsPool
    CTlsPool<T> pool(dfNODE_MAX, false);
    HANDLE TlsPoolThreads[dfTHREAD_CNT];
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        argForThread* arg = new argForThread((size_t)&pool, i);
        TlsPoolThreads[i] = (HANDLE)_beginthreadex(NULL, 0, TlsPoolThread<T>, arg, 0, nullptr);
        if (TlsPoolThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, TlsPoolThreads, true, INFINITE);

    double TlsPoolAllocTotal = 0;
    double TlsPoolFreeTotal = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        TlsPoolAllocTotal += TlsPoolAllocTotals[i];
        TlsPoolFreeTotal += TlsPoolFreeTotals[i];
    }
    TlsPoolAllocTotal = TlsPoolAllocTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);
    TlsPoolFreeTotal = TlsPoolFreeTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);
    TlsPoolAllocDurations.push_back(TlsPoolAllocTotal);
    TlsPoolFreeDurations.push_back(TlsPoolFreeTotal);

    // Test New Delete
    HANDLE NewDeleteThreads[dfTHREAD_CNT];
    ResetEvent(beginThreadComplete);
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        NewDeleteThreads[i] = (HANDLE)_beginthreadex(NULL, 0, NewDeleteThread<T>, (void*)i, 0, nullptr);
        if (NewDeleteThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    SetEvent(beginThreadComplete);
    WaitForMultipleObjects(dfTHREAD_CNT, NewDeleteThreads, true, INFINITE);

    double NewTotal = 0;
    double DeleteTotal = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        NewTotal += NewTotals[i];
        DeleteTotal += DeleteTotals[i];
    }
    NewTotal = NewTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);
    DeleteTotal = DeleteTotal * MS_PER_SEC / (dfTEST_TOTAL_CNT - 1);
    NewDurations.push_back(NewTotal);
    DeleteDurations.push_back(DeleteTotal);
    
    // Set X-Axis
    blockSizes.push_back(sizeof(T));
    ::printf(">");
}

void DrawGraph()
{
    // Generate Alloc Graph
    plt_alloc::plot(blockSizes, TlsPoolAllocDurations, "r-");
    plt_alloc::plot(blockSizes, NewDurations, "k-");

    plt_alloc::xlabel("Block Size (byte)");
    plt_alloc::ylabel("Avg Time (ms)");
    plt_alloc::title("CTlsPool Alloc vs New Performance");
    plt_alloc::legend();
    plt_alloc::grid(true);

    plt_alloc::save("Alloc.png");
    plt_alloc::show();

    std::cout << "Graph generated: Alloc.png\n";

    // Generate Free Graph
    plt_free::plot(blockSizes, TlsPoolFreeDurations, "r-");
    plt_free::plot(blockSizes, DeleteDurations, "k-");

    plt_free::xlabel("Block Size (byte)");
    plt_free::ylabel("Avg Time (ms)");
    plt_free::title("CTlsPool Free vs Delete Performance");
    plt_free::legend();
    plt_free::grid(true);

    plt_free::save("Free.png");
    plt_free::show();

    std::cout << "Graph generated: Free.png\n";
}

void SaveToCSV() 
{
    // Generate Alloc CSV
    std::ofstream allocFile("Alloc.csv");
    if (!allocFile.is_open()) 
    {
        std::cerr << "Can not open the file: Alloc.csv" << std::endl;
        return;
    }

    for (size_t i = 0; i < blockSizes.size(); ++i) 
    {
        allocFile << blockSizes[i];
        if (i < blockSizes.size() - 1)
            allocFile << ",";
    }
    allocFile << "\n";

    for (size_t i = 0; i < TlsPoolAllocDurations.size(); ++i)
    {
        allocFile << TlsPoolAllocDurations[i];
        if (i < TlsPoolAllocDurations.size() - 1)
            allocFile << ",";
    }
    allocFile << "\n";

    for (size_t i = 0; i < NewDurations.size(); ++i)
    {
        allocFile << NewDurations[i];
        if (i < NewDurations.size() - 1)
            allocFile << ",";
    }
    allocFile << "\n";
   
    allocFile.close();
    std::cout << "CSV generated: Alloc.csv" << std::endl;

    // Generate Free CSV
    std::ofstream freeFile("Free.csv");
    if (!freeFile.is_open())
    {
        std::cerr << "Can not open the file: Free.csv" << std::endl;
        return;
    }

    for (size_t i = 0; i < blockSizes.size(); ++i)
    {
        freeFile << blockSizes[i];
        if (i < blockSizes.size() - 1)
            freeFile << ",";
    }
    freeFile << "\n";

    for (size_t i = 0; i < TlsPoolFreeDurations.size(); ++i)
    {
        freeFile << TlsPoolFreeDurations[i];
        if (i < TlsPoolFreeDurations.size() - 1)
            freeFile << ",";
    }
    freeFile << "\n";

    for (size_t i = 0; i < DeleteDurations.size(); ++i)
    {
        freeFile << DeleteDurations[i];
        if (i < DeleteDurations.size() - 1)
            freeFile << ",";
    }
    freeFile << "\n";

    freeFile.close();
    std::cout << "CSV generated: Free.csv" << std::endl;
}

void Test()
{
    beginThreadComplete = CreateEvent(NULL, true, false, nullptr);
    if (beginThreadComplete == NULL) printf("Error\n");

    timeBeginPeriod(1);

    {
        comparePerformance<MyStruct0>();
        comparePerformance<MyStruct1>();
        comparePerformance<MyStruct2>();
        comparePerformance<MyStruct3>();
        comparePerformance<MyStruct4>();
        comparePerformance<MyStruct5>();
        comparePerformance<MyStruct6>();
        comparePerformance<MyStruct7>();
        comparePerformance<MyStruct8>();
        comparePerformance<MyStruct9>();
        comparePerformance<MyStruct10>();
        comparePerformance<MyStruct11>();
        comparePerformance<MyStruct12>();
        comparePerformance<MyStruct13>();
        comparePerformance<MyStruct14>();
        comparePerformance<MyStruct15>();
        comparePerformance<MyStruct16>();
        comparePerformance<MyStruct17>();
        comparePerformance<MyStruct18>();
        comparePerformance<MyStruct19>();
        comparePerformance<MyStruct20>();
        comparePerformance<MyStruct21>();
        comparePerformance<MyStruct22>();
        comparePerformance<MyStruct23>();
        comparePerformance<MyStruct24>();
        comparePerformance<MyStruct25>();
        comparePerformance<MyStruct26>();
        comparePerformance<MyStruct27>();
        comparePerformance<MyStruct28>();
        comparePerformance<MyStruct29>();
        comparePerformance<MyStruct30>();
        comparePerformance<MyStruct31>();
        comparePerformance<MyStruct32>();
        comparePerformance<MyStruct33>();
        comparePerformance<MyStruct34>();
        comparePerformance<MyStruct35>();
        comparePerformance<MyStruct36>();
        comparePerformance<MyStruct37>();
        comparePerformance<MyStruct38>();
        comparePerformance<MyStruct39>();
        comparePerformance<MyStruct40>();
        comparePerformance<MyStruct41>();
        comparePerformance<MyStruct42>();
        comparePerformance<MyStruct43>();
        comparePerformance<MyStruct44>();
        comparePerformance<MyStruct45>();
        comparePerformance<MyStruct46>();
        comparePerformance<MyStruct47>();
        comparePerformance<MyStruct48>();
        comparePerformance<MyStruct49>();
        comparePerformance<MyStruct50>();
        comparePerformance<MyStruct51>();
        comparePerformance<MyStruct52>();
        comparePerformance<MyStruct53>();
        comparePerformance<MyStruct54>();
        comparePerformance<MyStruct55>();
        comparePerformance<MyStruct56>();
        comparePerformance<MyStruct57>();
        comparePerformance<MyStruct58>();
        comparePerformance<MyStruct59>();
        comparePerformance<MyStruct60>();
        comparePerformance<MyStruct61>();
        comparePerformance<MyStruct62>();
        comparePerformance<MyStruct63>();
        comparePerformance<MyStruct64>();
        comparePerformance<MyStruct65>();
        comparePerformance<MyStruct66>();
        comparePerformance<MyStruct67>();
        comparePerformance<MyStruct68>();
        comparePerformance<MyStruct69>();
        comparePerformance<MyStruct70>();
        comparePerformance<MyStruct71>();
        comparePerformance<MyStruct72>();
        comparePerformance<MyStruct73>();
        comparePerformance<MyStruct74>();
        comparePerformance<MyStruct75>();
        comparePerformance<MyStruct76>();
        comparePerformance<MyStruct77>();
        comparePerformance<MyStruct78>();
        comparePerformance<MyStruct79>();
        comparePerformance<MyStruct80>();
        comparePerformance<MyStruct81>();
        comparePerformance<MyStruct82>();
        comparePerformance<MyStruct83>();
        comparePerformance<MyStruct84>();
        comparePerformance<MyStruct85>();
        comparePerformance<MyStruct86>();
        comparePerformance<MyStruct87>();
        comparePerformance<MyStruct88>();
        comparePerformance<MyStruct89>();
        comparePerformance<MyStruct90>();
        comparePerformance<MyStruct91>();
        comparePerformance<MyStruct92>();
        comparePerformance<MyStruct93>();
        comparePerformance<MyStruct94>();
        comparePerformance<MyStruct95>();
        comparePerformance<MyStruct96>();
        comparePerformance<MyStruct97>();
        comparePerformance<MyStruct98>();
        comparePerformance<MyStruct99>();
        comparePerformance<MyStruct100>();
        comparePerformance<MyStruct101>();
        comparePerformance<MyStruct102>();
        comparePerformance<MyStruct103>();
        comparePerformance<MyStruct104>();
        comparePerformance<MyStruct105>();
        comparePerformance<MyStruct106>();
        comparePerformance<MyStruct107>();
        comparePerformance<MyStruct108>();
        comparePerformance<MyStruct109>();
        comparePerformance<MyStruct110>();
        comparePerformance<MyStruct111>();
        comparePerformance<MyStruct112>();
        comparePerformance<MyStruct113>();
        comparePerformance<MyStruct114>();
        comparePerformance<MyStruct115>();
        comparePerformance<MyStruct116>();
        comparePerformance<MyStruct117>();
        comparePerformance<MyStruct118>();
        comparePerformance<MyStruct119>();
        comparePerformance<MyStruct120>();
        comparePerformance<MyStruct121>();
        comparePerformance<MyStruct122>();
        comparePerformance<MyStruct123>();
        comparePerformance<MyStruct124>();
        comparePerformance<MyStruct125>();
        comparePerformance<MyStruct126>();
        comparePerformance<MyStruct127>();
        comparePerformance<MyStruct128>();
        comparePerformance<MyStruct129>();
        comparePerformance<MyStruct130>();
        comparePerformance<MyStruct131>();
        comparePerformance<MyStruct132>();
        comparePerformance<MyStruct133>();
        comparePerformance<MyStruct134>();
        comparePerformance<MyStruct135>();
        comparePerformance<MyStruct136>();
        comparePerformance<MyStruct137>();
        comparePerformance<MyStruct138>();
        comparePerformance<MyStruct139>();
        comparePerformance<MyStruct140>();
        comparePerformance<MyStruct141>();
        comparePerformance<MyStruct142>();
        comparePerformance<MyStruct143>();
        comparePerformance<MyStruct144>();
        comparePerformance<MyStruct145>();
        comparePerformance<MyStruct146>();
        comparePerformance<MyStruct147>();
        comparePerformance<MyStruct148>();
        comparePerformance<MyStruct149>();
        comparePerformance<MyStruct150>();
        comparePerformance<MyStruct151>();
        comparePerformance<MyStruct152>();
        comparePerformance<MyStruct153>();
        comparePerformance<MyStruct154>();
        comparePerformance<MyStruct155>();
        comparePerformance<MyStruct156>();
        comparePerformance<MyStruct157>();
        comparePerformance<MyStruct158>();
        comparePerformance<MyStruct159>();
        comparePerformance<MyStruct160>();
        comparePerformance<MyStruct161>();
        comparePerformance<MyStruct162>();
        comparePerformance<MyStruct163>();
        comparePerformance<MyStruct164>();
        comparePerformance<MyStruct165>();
        comparePerformance<MyStruct166>();
        comparePerformance<MyStruct167>();
        comparePerformance<MyStruct168>();
        comparePerformance<MyStruct169>();
        comparePerformance<MyStruct170>();
        comparePerformance<MyStruct171>();
        comparePerformance<MyStruct172>();
        comparePerformance<MyStruct173>();
        comparePerformance<MyStruct174>();
        comparePerformance<MyStruct175>();
        comparePerformance<MyStruct176>();
        comparePerformance<MyStruct177>();
        comparePerformance<MyStruct178>();
        comparePerformance<MyStruct179>();
        comparePerformance<MyStruct180>();
        comparePerformance<MyStruct181>();
        comparePerformance<MyStruct182>();
        comparePerformance<MyStruct183>();
        comparePerformance<MyStruct184>();
        comparePerformance<MyStruct185>();
        comparePerformance<MyStruct186>();
        comparePerformance<MyStruct187>();
        comparePerformance<MyStruct188>();
        comparePerformance<MyStruct189>();
        comparePerformance<MyStruct190>();
        comparePerformance<MyStruct191>();
        comparePerformance<MyStruct192>();
        comparePerformance<MyStruct193>();
        comparePerformance<MyStruct194>();
        comparePerformance<MyStruct195>();
        comparePerformance<MyStruct196>();
        comparePerformance<MyStruct197>();
        comparePerformance<MyStruct198>();
        comparePerformance<MyStruct199>();
        comparePerformance<MyStruct200>();
        comparePerformance<MyStruct201>();
        comparePerformance<MyStruct202>();
        comparePerformance<MyStruct203>();
        comparePerformance<MyStruct204>();
        comparePerformance<MyStruct205>();
        comparePerformance<MyStruct206>();
        comparePerformance<MyStruct207>();
        comparePerformance<MyStruct208>();
        comparePerformance<MyStruct209>();
        comparePerformance<MyStruct210>();
        comparePerformance<MyStruct211>();
        comparePerformance<MyStruct212>();
        comparePerformance<MyStruct213>();
        comparePerformance<MyStruct214>();
        comparePerformance<MyStruct215>();
        comparePerformance<MyStruct216>();
        comparePerformance<MyStruct217>();
        comparePerformance<MyStruct218>();
        comparePerformance<MyStruct219>();
        comparePerformance<MyStruct220>();
        comparePerformance<MyStruct221>();
        comparePerformance<MyStruct222>();
        comparePerformance<MyStruct223>();
        comparePerformance<MyStruct224>();
        comparePerformance<MyStruct225>();
        comparePerformance<MyStruct226>();
        comparePerformance<MyStruct227>();
        comparePerformance<MyStruct228>();
        comparePerformance<MyStruct229>();
        comparePerformance<MyStruct230>();
        comparePerformance<MyStruct231>();
        comparePerformance<MyStruct232>();
        comparePerformance<MyStruct233>();
        comparePerformance<MyStruct234>();
        comparePerformance<MyStruct235>();
        comparePerformance<MyStruct236>();
        comparePerformance<MyStruct237>();
        comparePerformance<MyStruct238>();
        comparePerformance<MyStruct239>();
        comparePerformance<MyStruct240>();
        comparePerformance<MyStruct241>();
        comparePerformance<MyStruct242>();
        comparePerformance<MyStruct243>();
        comparePerformance<MyStruct244>();
        comparePerformance<MyStruct245>();
        comparePerformance<MyStruct246>();
        comparePerformance<MyStruct247>();
        comparePerformance<MyStruct248>();
        comparePerformance<MyStruct249>();
        comparePerformance<MyStruct250>();
        comparePerformance<MyStruct251>();
        comparePerformance<MyStruct252>();
        comparePerformance<MyStruct253>();
        comparePerformance<MyStruct254>();
        comparePerformance<MyStruct255>();

    }

    /*
    {
        comparePerformance<MyStruct256>();
        comparePerformance<MyStruct257>();
        comparePerformance<MyStruct258>();
        comparePerformance<MyStruct259>();
        comparePerformance<MyStruct260>();
        comparePerformance<MyStruct261>();
        comparePerformance<MyStruct262>();
        comparePerformance<MyStruct263>();
        comparePerformance<MyStruct264>();
        comparePerformance<MyStruct265>();
        comparePerformance<MyStruct266>();
        comparePerformance<MyStruct267>();
        comparePerformance<MyStruct268>();
        comparePerformance<MyStruct269>();
        comparePerformance<MyStruct270>();
        comparePerformance<MyStruct271>();
        comparePerformance<MyStruct272>();
        comparePerformance<MyStruct273>();
        comparePerformance<MyStruct274>();
        comparePerformance<MyStruct275>();
        comparePerformance<MyStruct276>();
        comparePerformance<MyStruct277>();
        comparePerformance<MyStruct278>();
        comparePerformance<MyStruct279>();
        comparePerformance<MyStruct280>();
        comparePerformance<MyStruct281>();
        comparePerformance<MyStruct282>();
        comparePerformance<MyStruct283>();
        comparePerformance<MyStruct284>();
        comparePerformance<MyStruct285>();
        comparePerformance<MyStruct286>();
        comparePerformance<MyStruct287>();
        comparePerformance<MyStruct288>();
        comparePerformance<MyStruct289>();
        comparePerformance<MyStruct290>();
        comparePerformance<MyStruct291>();
        comparePerformance<MyStruct292>();
        comparePerformance<MyStruct293>();
        comparePerformance<MyStruct294>();
        comparePerformance<MyStruct295>();
        comparePerformance<MyStruct296>();
        comparePerformance<MyStruct297>();
        comparePerformance<MyStruct298>();
        comparePerformance<MyStruct299>();
        comparePerformance<MyStruct300>();
        comparePerformance<MyStruct301>();
        comparePerformance<MyStruct302>();
        comparePerformance<MyStruct303>();
        comparePerformance<MyStruct304>();
        comparePerformance<MyStruct305>();
        comparePerformance<MyStruct306>();
        comparePerformance<MyStruct307>();
        comparePerformance<MyStruct308>();
        comparePerformance<MyStruct309>();
        comparePerformance<MyStruct310>();
        comparePerformance<MyStruct311>();
        comparePerformance<MyStruct312>();
        comparePerformance<MyStruct313>();
        comparePerformance<MyStruct314>();
        comparePerformance<MyStruct315>();
        comparePerformance<MyStruct316>();
        comparePerformance<MyStruct317>();
        comparePerformance<MyStruct318>();
        comparePerformance<MyStruct319>();
        comparePerformance<MyStruct320>();
        comparePerformance<MyStruct321>();
        comparePerformance<MyStruct322>();
        comparePerformance<MyStruct323>();
        comparePerformance<MyStruct324>();
        comparePerformance<MyStruct325>();
        comparePerformance<MyStruct326>();
        comparePerformance<MyStruct327>();
        comparePerformance<MyStruct328>();
        comparePerformance<MyStruct329>();
        comparePerformance<MyStruct330>();
        comparePerformance<MyStruct331>();
        comparePerformance<MyStruct332>();
        comparePerformance<MyStruct333>();
        comparePerformance<MyStruct334>();
        comparePerformance<MyStruct335>();
        comparePerformance<MyStruct336>();
        comparePerformance<MyStruct337>();
        comparePerformance<MyStruct338>();
        comparePerformance<MyStruct339>();
        comparePerformance<MyStruct340>();
        comparePerformance<MyStruct341>();
        comparePerformance<MyStruct342>();
        comparePerformance<MyStruct343>();
        comparePerformance<MyStruct344>();
        comparePerformance<MyStruct345>();
        comparePerformance<MyStruct346>();
        comparePerformance<MyStruct347>();
        comparePerformance<MyStruct348>();
        comparePerformance<MyStruct349>();
        comparePerformance<MyStruct350>();
        comparePerformance<MyStruct351>();
        comparePerformance<MyStruct352>();
        comparePerformance<MyStruct353>();
        comparePerformance<MyStruct354>();
        comparePerformance<MyStruct355>();
        comparePerformance<MyStruct356>();
        comparePerformance<MyStruct357>();
        comparePerformance<MyStruct358>();
        comparePerformance<MyStruct359>();
        comparePerformance<MyStruct360>();
        comparePerformance<MyStruct361>();
        comparePerformance<MyStruct362>();
        comparePerformance<MyStruct363>();
        comparePerformance<MyStruct364>();
        comparePerformance<MyStruct365>();
        comparePerformance<MyStruct366>();
        comparePerformance<MyStruct367>();
        comparePerformance<MyStruct368>();
        comparePerformance<MyStruct369>();
        comparePerformance<MyStruct370>();
        comparePerformance<MyStruct371>();
        comparePerformance<MyStruct372>();
        comparePerformance<MyStruct373>();
        comparePerformance<MyStruct374>();
        comparePerformance<MyStruct375>();
        comparePerformance<MyStruct376>();
        comparePerformance<MyStruct377>();
        comparePerformance<MyStruct378>();
        comparePerformance<MyStruct379>();
        comparePerformance<MyStruct380>();
        comparePerformance<MyStruct381>();
        comparePerformance<MyStruct382>();
        comparePerformance<MyStruct383>();
        comparePerformance<MyStruct384>();
        comparePerformance<MyStruct385>();
        comparePerformance<MyStruct386>();
        comparePerformance<MyStruct387>();
        comparePerformance<MyStruct388>();
        comparePerformance<MyStruct389>();
        comparePerformance<MyStruct390>();
        comparePerformance<MyStruct391>();
        comparePerformance<MyStruct392>();
        comparePerformance<MyStruct393>();
        comparePerformance<MyStruct394>();
        comparePerformance<MyStruct395>();
        comparePerformance<MyStruct396>();
        comparePerformance<MyStruct397>();
        comparePerformance<MyStruct398>();
        comparePerformance<MyStruct399>();
        comparePerformance<MyStruct400>();
        comparePerformance<MyStruct401>();
        comparePerformance<MyStruct402>();
        comparePerformance<MyStruct403>();
        comparePerformance<MyStruct404>();
        comparePerformance<MyStruct405>();
        comparePerformance<MyStruct406>();
        comparePerformance<MyStruct407>();
        comparePerformance<MyStruct408>();
        comparePerformance<MyStruct409>();
        comparePerformance<MyStruct410>();
        comparePerformance<MyStruct411>();
        comparePerformance<MyStruct412>();
        comparePerformance<MyStruct413>();
        comparePerformance<MyStruct414>();
        comparePerformance<MyStruct415>();
        comparePerformance<MyStruct416>();
        comparePerformance<MyStruct417>();
        comparePerformance<MyStruct418>();
        comparePerformance<MyStruct419>();
        comparePerformance<MyStruct420>();
        comparePerformance<MyStruct421>();
        comparePerformance<MyStruct422>();
        comparePerformance<MyStruct423>();
        comparePerformance<MyStruct424>();
        comparePerformance<MyStruct425>();
        comparePerformance<MyStruct426>();
        comparePerformance<MyStruct427>();
        comparePerformance<MyStruct428>();
        comparePerformance<MyStruct429>();
        comparePerformance<MyStruct430>();
        comparePerformance<MyStruct431>();
        comparePerformance<MyStruct432>();
        comparePerformance<MyStruct433>();
        comparePerformance<MyStruct434>();
        comparePerformance<MyStruct435>();
        comparePerformance<MyStruct436>();
        comparePerformance<MyStruct437>();
        comparePerformance<MyStruct438>();
        comparePerformance<MyStruct439>();
        comparePerformance<MyStruct440>();
        comparePerformance<MyStruct441>();
        comparePerformance<MyStruct442>();
        comparePerformance<MyStruct443>();
        comparePerformance<MyStruct444>();
        comparePerformance<MyStruct445>();
        comparePerformance<MyStruct446>();
        comparePerformance<MyStruct447>();
        comparePerformance<MyStruct448>();
        comparePerformance<MyStruct449>();
        comparePerformance<MyStruct450>();
        comparePerformance<MyStruct451>();
        comparePerformance<MyStruct452>();
        comparePerformance<MyStruct453>();
        comparePerformance<MyStruct454>();
        comparePerformance<MyStruct455>();
        comparePerformance<MyStruct456>();
        comparePerformance<MyStruct457>();
        comparePerformance<MyStruct458>();
        comparePerformance<MyStruct459>();
        comparePerformance<MyStruct460>();
        comparePerformance<MyStruct461>();
        comparePerformance<MyStruct462>();
        comparePerformance<MyStruct463>();
        comparePerformance<MyStruct464>();
        comparePerformance<MyStruct465>();
        comparePerformance<MyStruct466>();
        comparePerformance<MyStruct467>();
        comparePerformance<MyStruct468>();
        comparePerformance<MyStruct469>();
        comparePerformance<MyStruct470>();
        comparePerformance<MyStruct471>();
        comparePerformance<MyStruct472>();
        comparePerformance<MyStruct473>();
        comparePerformance<MyStruct474>();
        comparePerformance<MyStruct475>();
        comparePerformance<MyStruct476>();
        comparePerformance<MyStruct477>();
        comparePerformance<MyStruct478>();
        comparePerformance<MyStruct479>();
        comparePerformance<MyStruct480>();
        comparePerformance<MyStruct481>();
        comparePerformance<MyStruct482>();
        comparePerformance<MyStruct483>();
        comparePerformance<MyStruct484>();
        comparePerformance<MyStruct485>();
        comparePerformance<MyStruct486>();
        comparePerformance<MyStruct487>();
        comparePerformance<MyStruct488>();
        comparePerformance<MyStruct489>();
        comparePerformance<MyStruct490>();
        comparePerformance<MyStruct491>();
        comparePerformance<MyStruct492>();
        comparePerformance<MyStruct493>();
        comparePerformance<MyStruct494>();
        comparePerformance<MyStruct495>();
        comparePerformance<MyStruct496>();
        comparePerformance<MyStruct497>();
        comparePerformance<MyStruct498>();
        comparePerformance<MyStruct499>();
        comparePerformance<MyStruct500>();
        comparePerformance<MyStruct501>();
        comparePerformance<MyStruct502>();
        comparePerformance<MyStruct503>();
        comparePerformance<MyStruct504>();
        comparePerformance<MyStruct505>();
        comparePerformance<MyStruct506>();
        comparePerformance<MyStruct507>();
        comparePerformance<MyStruct508>();
        comparePerformance<MyStruct509>();
        comparePerformance<MyStruct510>();
        comparePerformance<MyStruct511>();
    }
    */

    timeEndPeriod(1);

    ::printf("\n");
    DrawGraph();
    SaveToCSV();
}