//
// Created by zhangshiping on 24-5-29.
//
#include "memory_pool.h"
int main(){
    auto *memory_pool= MemPoolManager;
    memory_pool->DebugPrint();
    auto mem= memory_pool->GetMemory(10);
    memory_pool->GiveBack(mem);
    memory_pool->DebugPrint();

    mem=memory_pool->GetMemory(100);
    memory_pool->DebugPrint();
    memory_pool->GiveBack(mem);
    memory_pool->DebugPrint();

    mem=memory_pool->GetMemory(1000);
    memory_pool->DebugPrint();
    memory_pool->GiveBack(mem);
    memory_pool->DebugPrint();
}