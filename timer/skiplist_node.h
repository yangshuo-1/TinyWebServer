#ifndef SKIPLIST_NODE
#define SKIPLIST_NODE

#include <vector>
#include <functional>
#include <iostream>

#include "../log/log.h"
#include "commen.h"

class SkiplistNode;

class SkiplistLevel{
public:
        SkiplistLevel(): next_(nullptr){};
        ~SkiplistLevel() {next_ = nullptr;};
        SkiplistNode* getNextNode() {return next_;};
        void setNextNode(SkiplistNode* t) {next_ = t;};
private:
    SkiplistNode* next_;
};



class SkiplistNode{
public:
    SkiplistNode(int key, int level, Handler_ptr recall): key_(key), level_(level){
        cd_func_ = recall;
        if(level > MAX_SKIPLIST_LEVEL){
            // LOG_ERROR("skiplist level error");
        }
        skip_level_.reserve(level);
        for(int i = 0; i < level; ++i){
            skip_level_[i] = new SkiplistLevel();
        }
    };

    ~SkiplistNode(){
        // 析构时会调用SkiplistLevel析构函数
    };

    int getKey() {return key_;};
    int getVal() {return level_;};

    SkiplistNode* getNextNode(int level) {return skip_level_[level]->getNextNode();};
    void setNextNode(int level, SkiplistNode* node) {skip_level_[level]->setNextNode(node);};

private:
    int key_;
    int level_;

    Handler_ptr cd_func_;

    std::vector<SkiplistLevel*> skip_level_;
};




#endif SKIPLIST_NODE