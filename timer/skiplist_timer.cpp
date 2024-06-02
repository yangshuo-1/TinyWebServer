# include "skiplist_timer.h"


SkipList::SkipList(){
    skplist_length_ = 0;
    skplist_level_ = 0;
    
    skplit_head_ = new SkiplistNode(0, MAX_SKIPLIST_LEVEL, nullptr);
};


SkipList::~SkipList(){

};

int SkipList::getRondomLevel(){

    int level = 1;

    std::mt19937 generator(std::random_device{}());
    // 创建一个在[0, 1)范围内的均匀分布
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    while(1){
        // 生成随机数
        double randomValue = distribution(generator);  

        if(randomValue < SKIPLIST_P) level++;
        else break;
    }

    return (level < MAX_SKIPLIST_LEVEL)?level:MAX_SKIPLIST_LEVEL;

}


bool SkipList::InsertTimer(Timer* timer){
    std::vector<SkiplistNode*> temp;
    temp.reserve(MAX_SKIPLIST_LEVEL);

    SkiplistNode* cur = skplit_head_;
    for(int i = 0; i < skplist_level_; ++i){
        while(cur->getNextNode(i) && cur->getNextNode(i)->getKey() < timer->getExpire()){
            cur = cur->getNextNode(i);
        }
        // todo: node类是否需要增加拷贝构造函数或者赋值操作符 
        temp[i] = cur;
    }

    int new_level = this->getRondomLevel();

    // 需要增加层数
    if(new_level > this->skplist_level_){
        for(int i = this->skplist_level_; i < new_level; ++i){
            temp[i] = skplit_head_;
        }
        this->skplist_level_ = new_level;
    }

    // todo   skipnode需要保存的信息需要和timer类对齐
    cur = new SkiplistNode();

    // 新节点插入到各个level中 
    for(int i = 0; i < new_level; ++i){
        cur->setNextNode(i, temp[i]->getNextNode(i));
        temp[i]->setNextNode(i, cur);
    }
    ++skplist_length_;
    return true;
};


bool SkipList::DeleteSkipListNode(SkiplistNode* node){
    std::vector<SkiplistNode*> temp;
    temp.reserve(MAX_SKIPLIST_LEVEL);

    SkiplistNode* cur = this->skplit_head_;

    for(int i = this->skplist_level_ - 1; i >= 0; ++i){
        while(cur->getNextNode(i) && cur->getNextNode(i)->getKey() < node->getKey()){
            cur = cur->getNextNode(i);
        }
        temp[i]->setNextNode(i, cur);
    }
    
};
bool SkipList::AdjustTimer(Timer* timer){};
void SkipList::Tick(){};
