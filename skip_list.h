//
// Created by zhangshiping on 24-5-24.
//

#ifndef GAMETOOLS_SKIP_LIST_H
#define GAMETOOLS_SKIP_LIST_H

#include <cstdint>
#include <cstdlib>
#include <unordered_map>
#include <iostream>
#include <memory>
#include <vector>
#include <random>

namespace GameTools{
//定义跳表节点 , K为数据，需要排序，V是唯一标识符，用来查找
template<class K,class V>
struct SkipListNode{
    K key;
    V value;
    //前节点
    SkipListNode* pre;
    //层数
    int32_t level;
    //每层节点信息，
    struct SkipListLevel{
        SkipListNode<K,V> *next= nullptr;
        //span（跨度）是记录当前层中此节点到下一个节点在整条链中相差的节点数量，相邻为1
        uint64_t span=0;
    };
    //动态数组,levels[i]即第i层
    std::unique_ptr<SkipListLevel[]> levels;

    SkipListNode(int32_t lev)
            : pre(nullptr), level(lev), levels(new SkipListLevel[level])
    {
    }
};

//定义跳表的链表, K为数据，需要排序，V是唯一标识符，用来查找
template<class K,class V>
struct SkipList{
    SkipListNode<K,V> *header, *tail;
    //节点数量
    uint64_t length;
    //已有层数
    int32_t level;
};

//定义排序跳表, K为数据，需要排序，V是唯一标识符，用来查找
template<class K,class V,class H=std::hash<V>> //H 是哈希函数对象的类型。如果你不指定 H，那么它将默认为 std::hash<V>。
class RankSkipList{
    using RankMap=typename std::unordered_map<V,SkipListNode<K,V>*>;
private:
    //记录所有节点，value==>SKNode*
    RankMap rank_map_;
    SkipList<K,V> skip_list_;
    //最大长度
    uint64_t max_len_=0;
    //链表最大层数
    constexpr static int32_t SKIPLIST_MAX_LEVEL=32;
    //用于控制随机层数的系数
    constexpr static double SKIPLIST_P=0.5;
private:
    SkipListNode<K,V>* createNode(int32_t level,K key,V val){
        auto *new_node=new SkipListNode<K,V>(level); // 不同节点level可以是不一样
        new_node->key=key;
        new_node->value=val;
        return new_node;
    }
    /// 删除节点
    /// \param node 被删除的节点
    /// \param pre_nodes pre_nodes[i]表示node在第i层中的前节点
    void DeleteNode(SkipListNode<K,V>* node,SkipListNode<K,V>** pre_nodes){
        if(node== nullptr || pre_nodes == nullptr) return;
        //更新
        for(int32_t i=0;i<skip_list_.level;i++){
            //高层不一定包含这个待删除的节点
            if(node == pre_nodes[i]->levels[i].next){
                pre_nodes[i]->levels[i].span+= node->levels[i].span - 1;
                pre_nodes[i]->levels[i].next=node->levels[i].next;
            }
            else pre_nodes[i]->levels[i].span--;
        }
        //更新pre和tail
        if(node->levels[0].next){
            node->levels[0].next->pre=node->pre;
        }
        else{  //node就是队尾
            skip_list_.tail=node->pre;
        }
        //删除空层
        while(skip_list_.level>1&&skip_list_.header->levels[skip_list_.level-1].next== nullptr){
            skip_list_.level--;
        }
        skip_list_.length--;
        delete node;
    }
    /// 随机层数
    ///   理论来讲，一级索引中元素个数应该占原始数据的 50%，二级索引中元素个数占 25%，三级索引12.5% ，一直到最顶层。
    ///    因为这里每一层的晋升概率是 50%。对于每一个新插入的节点，都需要调用 randomLevel 生成一个合理的层数。
    ///    该 randomLevel 方法会随机生成 1~MAX_LEVEL 之间的数，且 ：
    ///            50%的概率返回 1
    ///            25%的概率返回 2
    ///          12.5%的概率返回 3 ...
    /// \return
    int32_t RandomLevel(){
        std::random_device rd; // 真实的随机数生成器
        std::mt19937 gen(rd()); // 使用Mersenne Twister算法的伪随机数生成器
        std::uniform_int_distribution<> dis(1, 100); // 生成1到100之间的均匀分布的随机数
        int random_number = dis(gen);
        int32_t level=1;
        while(random_number<=SKIPLIST_P*100){
            random_number=dis(gen);
            level++;
        }
        return (level<SKIPLIST_MAX_LEVEL)?level:SKIPLIST_MAX_LEVEL;
    }
    /// 更新节点 ，并让更新后的链表依然有序
    /// \param key
    /// \param val
    /// \return 更新节点指针 or nullptr
    SkipListNode<K,V>* UpdateNode(K key,V val){
        //待更新节点在每一层的前节点
        SkipListNode<K,V>* pre_nodes[SKIPLIST_MAX_LEVEL]={nullptr};
//      std::vector<SkipListNode<K,V>*> pre_nodes(SKIPLIST_MAX_LEVEL, nullptr);
        SkipListNode<K,V>* tmpNode=skip_list_.header;
        auto iter=rank_map_.find(val);
        //如果未找到
        if(iter==rank_map_.end()) return nullptr;
        //更新前原来的key
        K& cur_key=iter->second->key;
        for(int32_t i=skip_list_.level-1;i>=0;i--){
            while(tmpNode->levels[i].next
                &&(tmpNode->levels[i].next->key>cur_key||
                    (tmpNode->levels[i].next->key==cur_key&&tmpNode->levels[i].next->value>val)))
            {
                tmpNode=tmpNode->levels[i].next;
            }
            pre_nodes[i]=tmpNode;
        }
        tmpNode=tmpNode->levels[0].next;
        if(tmpNode== nullptr||tmpNode->key!=cur_key||tmpNode->value!=val) return nullptr;
        //如果更新key依然有序
        if((tmpNode->pre== nullptr||tmpNode->pre->key>key)
            &&(tmpNode->levels[0].next== nullptr||tmpNode->levels[0].next->key<key))
        {
            tmpNode->key=key;
            return tmpNode;
        }
        //调整位置使得链有序
        //删除旧的节点
        DeleteNode(tmpNode, pre_nodes);
        rank_map_.erase(val); //这里不能写tmpNode->value,因为tmpNode已经被删除了
        //插入新节点
        return InsertOrUpdate(key,val);
    }

public:
    ///
    /// \param max_len 跳表最大长度
    RankSkipList(uint64_t max_len=0){
        skip_list_.header=new SkipListNode<K,V>(SKIPLIST_MAX_LEVEL);
        skip_list_.tail= nullptr;
        skip_list_.length=0;
        skip_list_.level=1;
        rank_map_.clear();
        max_len_=max_len;
    }
    ~RankSkipList(){
        SkipListNode<K,V> *node=skip_list_.header->levels[0].next;
        SkipListNode<K,V> *next= nullptr;
        //遍历删除0层全部节点
        while(node){
            next=node->levels[0].next;
            delete node;
            node=next;
        }
        //删除头节点
        delete skip_list_.header;
        skip_list_.header= nullptr;
    }
    /// 插入新节点，若存在则更新
    /// \param key
    /// \param val
    /// \return 节点指针
    SkipListNode<K,V>* InsertOrUpdate(K key,V val){
        //每一层中待插入节点的前节点,pre_nodes[i]是节点指针，但是新节点并不一定在i层，但一定在0层
        SkipListNode<K,V>* pre_nodes[SKIPLIST_MAX_LEVEL]={nullptr};

        SkipListNode<K,V>* tmpNode=skip_list_.header;
        //节点的排名，rank[i]其实是pre_node[i]在整条链中的排名，rank[0]才是新节点排名
        uint64_t rank[SKIPLIST_MAX_LEVEL]={0};
        //如果节点已经存在
        if(rank_map_.find(val)!=rank_map_.end()){
            return UpdateNode(key,val);
        }
        //从上层向下层计算rank ，上层节点包含在下层中
        for(int32_t i=skip_list_.level-1;i>=0;i--){
            rank[i]=(i==skip_list_.level-1)?0:rank[i+1];
            while(tmpNode->levels[i].next
                &&(tmpNode->levels[i].next->key>key
                    ||(tmpNode->levels[i].next->key==key&&tmpNode->levels[i].next->value>val)))
            {
                rank[i]+=tmpNode->levels[i].span;//span的累和，是排名
                tmpNode=tmpNode->levels[i].next;
            }
            pre_nodes[i]=tmpNode;
        }
        //长度超过上限
        if(max_len_>0&&skip_list_.length>=max_len_){
            //且待插入节点位于链尾
            if(pre_nodes[0]->levels[0].next == nullptr
               || pre_nodes[0]->levels[0].next->key > key){
                return nullptr;
            }
        }
        //随机层数
        int32_t level=RandomLevel();
        if(level>skip_list_.level){
            //扩展新的层数
            for(int32_t i=skip_list_.level;i<level;i++){
                rank[i]=0;
                pre_nodes[i]=skip_list_.header;
                pre_nodes[i]->levels[i].span=skip_list_.length;//span等于队首到队尾
            }
            skip_list_.level=level;
        }
        //创建节点
        tmpNode= createNode(level,key,val);
        for(int32_t i=0;i<level;i++){
            //插入新节点
            tmpNode->levels[i].next=pre_nodes[i]->levels[i].next;
            pre_nodes[i]->levels[i].next=tmpNode;
            //计算新节点的span
            ///  rank[0]就是插入节点的在整条链中位置,rank[i]就是前节点的在整条链中位置
            /// pre_node    insert_node    next_node
            ///   |  |__rank0-rank i__|      |
            ///   |_______________span______|
            tmpNode->levels[i].span= pre_nodes[i]->levels[i].span - (rank[0] - rank[i]);
            //更新插入节点的前节点的span
            pre_nodes[i]->levels[i].span= rank[0] - rank[i] + 1;
        }
        //插入节点的上层，更新前节点的span
        for(int32_t i=level;i<skip_list_.level;i++){
            pre_nodes[i]->levels[i].span++;
        }
        //设置回溯节点
        tmpNode->pre= (pre_nodes[0] == skip_list_.header) ? nullptr : pre_nodes[0];
        if(tmpNode->levels[0].next) tmpNode->levels[0].next->pre=tmpNode;
        else skip_list_.tail=tmpNode;

        skip_list_.length++;
        //更新map
        rank_map_.emplace(val,tmpNode);
        //长度>max_len_
        if(max_len_>0&&skip_list_.length>max_len_){
            DeleteNodeByRank(max_len_+1);
        }
        return tmpNode;
    }
    bool DeleteNode(V val){
        SkipListNode<K,V>* pre_nodes[SKIPLIST_MAX_LEVEL]={nullptr};
        SkipListNode<K,V>* tmpNode=skip_list_.header;
        auto iter=rank_map_.find(val);
        if(iter==rank_map_.end())  return false;
        K& key=iter->second->key;
        for(int32_t i=skip_list_.level-1;i>=0;i--){
            while (tmpNode->levels[i].next
                &&(tmpNode->levels[i].next->key>key||
                (tmpNode->levels[i].next->key==key&&tmpNode->levels[i].next->value>val)))
            {
                tmpNode=tmpNode->levels[i].next;
            }
            pre_nodes[i]=tmpNode;
        }
        tmpNode=tmpNode->levels[0].next; //被删除节点
        if(tmpNode&&tmpNode->key==key&&tmpNode->value==val)
        {
            DeleteNode(tmpNode,pre_nodes);
            rank_map_.erase(val);
            return true;
        }
        return false;
    }
    /// 删除第rank个节点
    /// \param rank
    /// \return
    uint64_t DeleteNodeByRank(uint64_t rank){
        if(rank>skip_list_.length) {
            std::cout<<"DeleteNodeByRank parameter error "<<std::endl;
            std::cout<<"rank="<<rank<<" length="<<skip_list_.length<<std::endl;
            return 0;
        }
        else return DeleteNodeByRange(rank,rank);
    }
    /// 删除第start个到end个节点
    /// \param start
    /// \param end
    /// \return
    uint64_t DeleteNodeByRange(uint64_t start,uint64_t end){
        if(!((start<=end)&&(start>0)&&(end<=skip_list_.length))) {
            std::cout<<"DeleteNodeByRange parameter error "<<std::endl;
            std::cout<<"start="<<start<<" end="<<end<<" length="<<skip_list_.length<<std::endl;
            return 0;
        }
        SkipListNode<K,V>* pre_nodes[SKIPLIST_MAX_LEVEL]={nullptr};
        SkipListNode<K,V>* tmpNode=skip_list_.header;
        uint64_t traversed=0,removed=0;
        for(int32_t i=skip_list_.level-1;i>=0;i--){
            while(tmpNode->levels[i].next&&(traversed+tmpNode->levels[i].span)<start){
                traversed+=tmpNode->levels[i].span;
                tmpNode=tmpNode->levels[i].next;
            }
            pre_nodes[i]=tmpNode;
        }
        tmpNode=tmpNode->levels[0].next;
        traversed++;
        while(tmpNode&&traversed<=end){
            auto* next=tmpNode->levels[0].next;
            DeleteNode(tmpNode,pre_nodes);
            rank_map_.erase(tmpNode->value);
            tmpNode=next;
            traversed++;
            removed++;
        }
        return removed;
    }
    /// 查询指定val在跳表中的排名(index)
    /// \param val
    /// \return
    int64_t Rank(const V& val){
        auto iter=rank_map_.find(val);
        if(iter==rank_map_.end()||!iter->second) return -1;
        K& cur_key=iter->second->key;
        SkipListNode<K,V>* tmpNode=skip_list_.header;
        uint64_t rank=0;
        for(int32_t i=skip_list_.level-1;i>=0;i--) {
            while(tmpNode->levels[i].next
                &&(tmpNode->levels[i].next->key>cur_key
                ||(tmpNode->levels[i].next->key==cur_key&&tmpNode->levels[i].next->value>val)))
            {
                rank+=tmpNode->levels[i].span;
                tmpNode=tmpNode->levels[i].next;
            }
            if(tmpNode->levels[i].next!= nullptr&&tmpNode->levels[i].next->value==val){
                rank+=tmpNode->levels[i].span;
                return rank;
            }
        }
        return -1;
    }
    /// 查找指定val的key
    /// \param val
    /// \return key的指针
    K* getKey(const V& val){
        auto iter=rank_map_.find(val);
        if(iter==rank_map_.end()||!iter->second) return nullptr;
        return &(iter->second->key);
    }
    /// 查找第rank个节点
    /// \param rank
    /// \return 节点指针
    SkipListNode<K,V>* getNodeByRank(uint64_t rank){
        SkipListNode<K,V>* tmpNode=skip_list_.header;
        uint64_t traversed=0;
        for(int32_t i=skip_list_.level-1;i>=0;i--){
            while(tmpNode->levels[i].next&&(traversed+tmpNode->levels[i].span)<=rank)
            {
                traversed+=tmpNode->levels[i].span;
                tmpNode=tmpNode->levels[i].next;
            }
            if(traversed==rank) return tmpNode;
        }
        return nullptr;
    }
    /// 是否存在指定val
    /// \param val
    /// \return
    bool has(const V& val) {
        return rank_map_.find(val)!=rank_map_.end() ;
    }
    /// 跳表当前长度
    /// \return
    uint64_t length(){
        return skip_list_.length;
    }
    ///逻辑上打印跳表
    void printSkipList(){
        for(int32_t i=skip_list_.level-1;i>=0;i--){
            for(auto iter=skip_list_.header->levels[i].next;iter!= nullptr; iter=iter->levels[i].next) {
                std::cout<<"["<<iter->key<<","<<iter->value<<","<<iter->levels[i].span<<"]";
                if(iter->levels[i].next) std::cout<<" => ";
            }
            std::cout<<std::endl;
        }
    }
};

}

#endif //GAMETOOLS_SKIP_LIST_H
