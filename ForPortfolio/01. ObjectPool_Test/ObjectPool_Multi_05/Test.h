#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <cmath>
#include "CTlsPool.h"  
#include "Data.h"

#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#define dfTEST_CNT 2000
#define dfTHREAD_CNT 5
#define dfNODE_MAX 12000
double TlsPoolTotals[dfTHREAD_CNT];
double NewDeleteTotals[dfTHREAD_CNT];

std::vector<int> blockSizes;
std::vector<double> TlsPoolDurations;
std::vector<double> NewDeleteDurations;

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
    TlsPoolTotals[idx] = 0;

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        auto TlsPoolStart = std::chrono::high_resolution_clock::now();
        T* obj = pool->Alloc();
        pool->Free(obj);
        auto TlsPoolEnd = std::chrono::high_resolution_clock::now();

        auto TlsPoolDuration = std::chrono::duration_cast<std::chrono::microseconds>(TlsPoolEnd - TlsPoolStart).count();
        TlsPoolTotals[idx] += TlsPoolDuration;
    }
}

template <typename T>
unsigned __stdcall NewDeleteThread(void* arg)
{
    int idx = (int)arg;
    NewDeleteTotals[idx] = 0;

    for (size_t i = 0; i < dfTEST_CNT; ++i)
    {
        auto NewDeleteStart = std::chrono::high_resolution_clock::now();
        T* obj = new T;
        delete obj;
        auto NewDeleteEnd = std::chrono::high_resolution_clock::now();

        auto NewDeleteDuration = std::chrono::duration_cast<std::chrono::microseconds>(NewDeleteEnd - NewDeleteStart).count();
        NewDeleteTotals[idx] += NewDeleteDuration;
    }
}

template <typename T>
void comparePerformance() 
{
    // Test TlsPool

    CTlsPool<T> pool(dfNODE_MAX, false);
    HANDLE TlsPoolThreads[dfTHREAD_CNT];
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
    WaitForMultipleObjects(dfTHREAD_CNT, TlsPoolThreads, true, INFINITE);
    
    double TlsPoolTotal = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        TlsPoolTotal += TlsPoolTotals[i];
    }
    TlsPoolTotal /= 10000;
    TlsPoolDurations.push_back(TlsPoolTotal);

    // Test New Delete

    HANDLE NewDeleteThreads[dfTHREAD_CNT];
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        NewDeleteThreads[i] = (HANDLE)_beginthreadex(NULL, 0, NewDeleteThread<T>, (void*)i, 0, nullptr);
        if (NewDeleteThreads[i] == NULL)
        {
            ::printf(" Error! %s(%d)\n", __func__, __LINE__);
            __debugbreak();
        }
    }
    WaitForMultipleObjects(dfTHREAD_CNT, NewDeleteThreads, true, INFINITE);

    double NewDeleteTotal = 0;
    for (int i = 0; i < dfTHREAD_CNT; i++)
    {
        NewDeleteTotal += NewDeleteTotals[i];
    }
    NewDeleteTotal /= 10000;
    NewDeleteDurations.push_back(NewDeleteTotal);

    blockSizes.push_back(sizeof(T));
}

void DrawGraph()
{
    // 그래프 그리기
    plt::plot(blockSizes, TlsPoolDurations, "r-");
    plt::plot(blockSizes, NewDeleteDurations, "k-");

    plt::xlabel("Block Size (byte)");
    plt::ylabel("Avg Time (ms)");
    plt::title("CTlsPool vs new/delete Performance");
    plt::legend();
    plt::grid(true);

    plt::save("performance_graph.png");
    std::cout << "Graph generated: performance_graph.png\n";
}

void Test()
{
    const size_t numAllocations = 10000;

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