#ifndef SKIPLISTTIMER
#define SKIPLISTTIMER

#include <iostream>
#include <time.h>
#include <random>


#include "commen.h"
#include "skiplist_node.h"

class SkipList{
public:
    SkipList();
    ~SkipList();

    bool InsertTimer(Timer* timer);
    bool DeleteTimer(Timer* timer);
    bool AdjustTimer(Timer* timer);

    void Tick();

private:
    bool DeleteSkipListNode(SkiplistNode* node);
    int getRondomLevel();

private:
    SkiplistNode* skplit_head_;
    int skplist_length_;
    int skplist_level_;
};






#endif SKIPLISTTIMER