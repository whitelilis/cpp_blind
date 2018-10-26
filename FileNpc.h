//
// Created by liuzhe on 2018-10-26.
//

#ifndef COPY_BLIND_FILENPC_H
#define COPY_BLIND_FILENPC_H

#include "Npc.h"
#include <fstream>

USING_WC_NAMESPACE
USING_YJJ_NAMESPACE

class FileNpc : public Npc{
public:
    FileNpc(const char *filePath);
    void setStrategy(IWCStrategy* strategy) override;
    void run() override; // loop the tick in file, and fake send back order/trade rsp
    int fake_limit_order(short source, string instrument_id, string exchange_id, double price, int volume,
                         LfDirectionType direction, LfOffsetFlagType offset) override;
    void eventFire(LFMarketDataField* tick) override;
    void string2tick(string line, LFMarketDataField* tick) override;

private:
    long orderCounter = 701000000;
    int tickDelay = 2;
    std::deque<LFRtnTradeField> tradeQueue;
    std::deque<LFRtnOrderField> orderQueue;
    std::ifstream fd;
    IWCStrategy * strategy;
    char_31 ticker;
};

#endif //COPY_BLIND_FILENPC_H
