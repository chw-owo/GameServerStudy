#pragma once
/*

#include <iostream>
#include <vector>
#include <chrono>
#include <tuple>
#include "ObjectPool.h"  // ObjectPool 템플릿 클래스 헤더 파일을 포함하세요.

// 구조체 정의
struct MyStruct0 { char data[8]; };
struct MyStruct1 { char data[16]; };
struct MyStruct2 { char data[24]; };
struct MyStruct3 { char data[32]; };
struct MyStruct4 { char data[40]; };
struct MyStruct5 { char data[48]; };
// ... MyStruct6, MyStruct7, ..., MyStruct255에 대한 정의를 추가하세요

// ObjectPool과 new/delete의 성능 비교 함수 템플릿
template <typename T>
void comparePerformance(size_t numAllocations) {
    std::vector<double> objectPoolDurations;
    std::vector<double> newDeleteDurations;

    T objectArray[256];

    ObjectPool<T> objectPool(numAllocations);

    auto objectPoolStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < numAllocations; ++i) {
        T* obj = objectPool.allocate();
        objectPool.deallocate(obj);
    }
    auto objectPoolEnd = std::chrono::high_resolution_clock::now();
    auto objectPoolDuration = std::chrono::duration_cast<std::chrono::microseconds>(objectPoolEnd - objectPoolStart).count();
    objectPoolDurations.push_back(objectPoolDuration);

    auto newDeleteStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < numAllocations; ++i) {
        T* obj = new T;
        delete obj;
    }
    auto newDeleteEnd = std::chrono::high_resolution_clock::now();
    auto newDeleteDuration = std::chrono::duration_cast<std::chrono::microseconds>(newDeleteEnd - newDeleteStart).count();
    newDeleteDurations.push_back(newDeleteDuration);

    // 그래프 그리기
    plt::plot(objectPoolDurations, "r-", label="ObjectPool");
    plt::plot(newDeleteDurations, "k-", label="new/delete");

    plt::xlabel("Struct Index");
    plt::ylabel("Time (microseconds)");
    plt::title("ObjectPool vs new/delete Performance");
    plt::legend();
    plt::grid(true);

    plt::save("performance_graph.png");
    std::cout << "Graph generated: performance_graph.png\n";
}

int main() {
    const size_t numAllocations = 10000;

    comparePerformance<MyStruct0>(numAllocations);
    comparePerformance<MyStruct1>(numAllocations);
    comparePerformance<MyStruct2>(numAllocations);
    comparePerformance<MyStruct3>(numAllocations);
    comparePerformance<MyStruct4>(numAllocations);
    comparePerformance<MyStruct5>(numAllocations);
    // ... MyStruct6, MyStruct7, ..., MyStruct255에 대한 호출을 추가하세요

    return 0;
}

*/