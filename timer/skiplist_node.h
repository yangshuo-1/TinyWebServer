#ifndef SKIPLIST_NODE
#define SKIPLIST_NODE

#include <vector>
#include <functional>

#include "commen.h"


class SkiplistNode{
public:
    SkiplistNode(int key, int level, handler recall): key_(key), level_(level){
        cd_func = recall;
    };

    ~SkiplistNode(){

    };

    int getKey() {return key_;};
    int getVal() {return level_;};

private:
    int key_;
    int level_;

    handler cd_func;

    std::vector<SkiplistNode*> next_;
};




#endif SKIPLIST_NODE