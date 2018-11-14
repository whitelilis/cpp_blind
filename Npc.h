//
// Created by liuzhe on 2018-10-26.
//

#ifndef COPY_BLIND_NPC_H
#define COPY_BLIND_NPC_H

#include "Other.h"

USING_WC_NAMESPACE

class Npc{
public:
    virtual void setStrategy(IWCStrategy* strategy) = 0;
    virtual void run() = 0; // loop the tick in file, and fake send back order/trade rsp
    virtual int fake_limit_order(short source, string instrument_id, string exchange_id, double price, int volume,
                         LfDirectionType direction, LfOffsetFlagType offset) = 0;
    virtual void eventFire(LFMarketDataField* tick) = 0;
    virtual void string2tick(string line, LFMarketDataField* tick) = 0;
};

#endif //COPY_BLIND_NPC_H
