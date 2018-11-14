//
// Created by liuzhe on 2018-10-26.
//

#include "FileNpc.h"
#include "Other.h"

USING_YJJ_NAMESPACE

FileNpc::FileNpc(const char *filePath) {
    fd.open(filePath);
    assert(fd.is_open());
}

void FileNpc::setStrategy(IWCStrategy * _strategy) {
    this->strategy = _strategy;
}


template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

void FileNpc::string2tick(string line, LFMarketDataField *tick) {

    std::vector<string> parts = split(line, ',');
    std::stringstream ss;

    ss << parts[1];
    ss >> tick->InstrumentID;
    ss.clear();

    ss << parts[3];
    ss >> tick->LastPrice;
    ss.clear();

    ss << parts[12];
    ss >> tick->AskPrice1;
    ss.clear();

    ss << parts[13];
    ss >> tick->BidPrice1;
    ss.clear();


    ss << parts[2].substr(11, 8);
    ss >> tick->UpdateTime;

    // todo : bug fix
    ss << parts[2].substr(0, 10);
    ss >> tick->TradingDay;

}

void FileNpc::run() {
    string line;
    LFMarketDataField tick;
    getline(fd, line); // ignore header
    while(getline(fd, line)){
        string2tick(line, &tick);
        eventFire(&tick);
        strategy->on_market_data(&tick, 777, 777);
    }
    fd.close();
}

int FileNpc::fake_limit_order(short source, string instrument_id, string exchange_id, double price, int volume,
                              LfDirectionType direction, LfOffsetFlagType offset) {
    orderCounter++;

    if(source < 0 || price < 0){
        std::cerr<<"source or price < 0"<<std::endl;
    }

    LFRtnOrderField orderField;
    strcpy(orderField.InstrumentID, instrument_id.data());
    strcpy(orderField.ExchangeID, exchange_id.data());
    orderField.VolumeTraded = volume;
    orderField.Direction = direction;
    orderField.OffsetFlag = offset;
    orderField.RequestID = orderCounter;
    orderField.OrderStatus = LF_CHAR_AllTraded;
    orderQueue.push_back(orderField);


    // todo : trade id not needed ?
    LFRtnTradeField tradeField;
    strcpy(tradeField.InstrumentID, instrument_id.data());
    strcpy(tradeField.ExchangeID, exchange_id.data());
    tradeField.Volume = volume;
    tradeField.Direction = direction;
    tradeField.OffsetFlag = offset;
    tradeQueue.push_back(tradeField);

    return orderCounter;
}

void FileNpc::eventFire(LFMarketDataField* tick) {
    if (orderQueue.empty() && tradeQueue.empty()){
        return;
    }else{
        if (tickDelay <= 0){ // it's time to fire
            tickDelay = 2;
            if(! orderQueue.empty() ){
                LFRtnOrderField & orderField = orderQueue.front();
                this->strategy->on_rtn_order(&orderField, orderCounter, 999, 999);
                orderQueue.pop_front();
            }
            if (! tradeQueue.empty()) {
                LFRtnTradeField & tradeField = tradeQueue.front();
                this->strategy->on_rtn_trade(&tradeField, orderCounter, 888, 888);
                tradeQueue.pop_front();
            }
        } else { // has event, not the time, change the delay
            tickDelay -= 1;
            if(! tradeQueue.empty()){
                LFRtnTradeField & tradeField = tradeQueue.front();
                strcpy(tradeField.TradingDay, tick->TradingDay);
                if(tradeField.Direction == LF_CHAR_Buy) {
                    tradeField.Price = tick->BidPrice1;
                }else if(tradeField.Direction == LF_CHAR_Sell){
                    tradeField.Price = tick->AskPrice1;
                }else{
                    std::cerr<<" what direction "<< tradeField.Direction << std::endl;
                }
            }
        }
    }
}

