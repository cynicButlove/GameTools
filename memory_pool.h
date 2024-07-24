//
// Created by zhangshiping on 24-5-29.
//

#ifndef GAMETOOLS_MEMORY_POOL_H
#define GAMETOOLS_MEMORY_POOL_H

#include "singleton.h"
#include <list>
#include <unordered_map>
#include <iostream>

namespace GameTools{

#ifndef ENABLE_MEMORY_POOL
#define ENABLE_MEMORY_POOL 1
#endif

#ifndef ENABLE_DEBUG_MEMORY_POOL
#define ENABLE_DEBUG_MEMORY_POOL 0
#endif

#define MemPoolManager GameTools::Singleton<GameTools::MemoryPool>::Instance()

///相同大小内存块的管理
class Chunk{
public:
    Chunk(){};
    ~Chunk(){
        for(auto p:mem_list_){
            delete p;
        }
        mem_list_.clear();
    }
    ///设置内存块大小
    bool SetChunkSize(std::size_t size){
        if(chunk_size_==0){
            chunk_size_=size;
            return true;
        }
        return false;
    }
    ///获取内存块,优先从链上获得
    char* GemMemory(){
        bool is_empty=false;
        do{
            std::lock_guard<std::mutex> lock(mutex_);
            if(mem_list_.empty()){
                is_empty=true;
                break;
            }
            auto pointer=mem_list_.front();
            mem_list_.pop_front();
            return pointer;
        }while(true);
        if(is_empty){
            char* new_char=new char[chunk_size_];
            return new_char;
        }
        return nullptr;
    }
    ///归还内存块 ，加入到链中
    void GiveBack(char* pointer,std::string debug_tag=""){
        std::lock_guard<std::mutex> lock(mutex_);
        mem_list_.push_back(pointer);
    }
    ///剩余内存块个数
    std::size_t Size(){
        return mem_list_.size();
    }

private:
    std::size_t chunk_size_=0;
    std::mutex mutex_;
    std::list<char*> mem_list_;///空闲内存块链

};


///内存池
class MemoryPool{
private:
    std::unordered_map<std::size_t ,Chunk> pool_;
    /// return 大于等于num的最小2的次方
    /// \param num
    /// \return
    int32_t upToPowerOfTwo(int32_t num){
        if(num<=0) return  0;
        int count=0;
        num--;///防止num本就是2的次方
        while(num!=0){
            num=num>>1;
            count++;
        }
        return 1<<count;
    }
public:
    MemoryPool(){};
    ~MemoryPool(){
        pool_.clear();
    }
    ///申请内存
    char* GetMemory(std::size_t size){
        size= upToPowerOfTwo(size+sizeof(uint32_t));
#if !ENABLE_MEMORY_POOL
        return new char[size];
#endif
        auto iter=pool_.find(size);
        if(iter==pool_.end()){
            pool_[size].SetChunkSize(size);
        }
        auto p=reinterpret_cast<std::uint32_t *>(pool_[size].GemMemory());
        if(p== nullptr) return nullptr;
        *p=size;///前四个字节记录分配内存大小
        return reinterpret_cast<char*>(p) + sizeof(std::uint32_t );///指针偏移
    }

    ///归还内存
    void GiveBack(char *pointer,std::string debug_tag=""){
#if !ENABLE_MEMORY_POOL
        delete pointer;
        return;
#endif
        if(pointer== nullptr)   return;
        auto size=*(reinterpret_cast<std::uint32_t *>(pointer-sizeof(uint32_t)));
        if(size== 0){
            delete (pointer-sizeof(std::uint32_t ));
            return ;
        }
        return pool_[size].GiveBack(pointer-sizeof(uint32_t),debug_tag);
    }
    ///打印
    void DebugPrint(){
        std::cout<<"内存池中有"<<pool_.size()<<"种不同大小的内存块"<<std::endl;
        for(auto iter=pool_.begin();iter!=pool_.end();iter++){
            std::cout<<"    内存块大小为 "<<iter->first<<"还有 "<<iter->second.Size()<<"个"<<std::endl;
        }
    }

};

}


#endif //GAMETOOLS_MEMORY_POOL_H
