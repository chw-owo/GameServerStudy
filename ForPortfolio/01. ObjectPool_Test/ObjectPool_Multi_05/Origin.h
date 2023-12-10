#pragma once
/*

#include <iostream>
#include <vector>
#include <chrono>
#include <tuple>
#include "ObjectPool.h"  // ObjectPool ���ø� Ŭ���� ��� ������ �����ϼ���.

// ����ü ����
struct MyStruct0 { char data[8]; };
struct MyStruct1 { char data[16]; };
struct MyStruct2 { char data[24]; };
struct MyStruct3 { char data[32]; };
struct MyStruct4 { char data[40]; };
struct MyStruct5 { char data[48]; };
// ... MyStruct6, MyStruct7, ..., MyStruct255�� ���� ���Ǹ� �߰��ϼ���

// ObjectPool�� new/delete�� ���� �� �Լ� ���ø�
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

    // �׷��� �׸���
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
    // ... MyStruct6, MyStruct7, ..., MyStruct255�� ���� ȣ���� �߰��ϼ���

    return 0;
}

*/