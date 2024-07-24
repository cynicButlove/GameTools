//
// Created by zhangshiping on 24-5-26.
//
#include "skip_list.h"
#include <cstdint>


int main() {
    // 创建一个长度为100的排行榜跳表
    auto *rank_skip_list = new GameTools::RankSkipList<int32_t , int32_t>(100);
    for(int i=0;i<15;i++){
        rank_skip_list->InsertOrUpdate(i,i);        //增
    }
    rank_skip_list->printSkipList();    //printSkipList()
    std::cout<<std::endl<<"val=7的rank= " <<rank_skip_list->Rank(7)<<std::endl;  //Rank()
    std::cout<<"val=7的key= "<<*(rank_skip_list->getKey(7))<<std::endl;        //getKey()
    rank_skip_list->InsertOrUpdate(90,7);                       //改
    std::cout<<"更新后val=7的key= "<<*(rank_skip_list->getKey(7))<<std::endl;
    rank_skip_list->printSkipList();
    std::cout<<"第7个是： "<<rank_skip_list->getNodeByRank(7)->value<<std::endl;
    std::cout<<"删除val=7"<<rank_skip_list->DeleteNode(7)<<std::endl;               //删
    std::cout<<"是否存在7： "<<rank_skip_list->has(7)<<std::endl;            //has()
    rank_skip_list->printSkipList();
    rank_skip_list->DeleteNodeByRange(2,8);    //删除第2到8个 //DeleteNodeByRange()
    std::cout<<"删除第2到8个"<<std::endl;
    rank_skip_list->printSkipList();
}