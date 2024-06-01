#ifndef SKIPLISTTIMER
#define SKIPLISTTIMER

#include "commen.h"

class SkipList{
public:
    SkipList();
    ~SkipList();

    bool InsertTimer(Timer* timer);
    bool DeleteTimer(Timer* timer);
    bool AdjustTimer(Timer* timer);

    void Tick();
private:
    
};






#endif SKIPLISTTIMER