/**
 * Strategy demo, same logic as band_demo_strategy.py
 * @Author cjiang (changhao.jiang@taurus.ai)
 * @since   Nov, 2017
 */

#include "IWCStrategy.h"
#include "Timer.h"
#include "Plan.h"
#include <deque>
#include <set>
#include <iostream>
#include "time.h"

USING_WC_NAMESPACE

#define SOURCE_INDEX SOURCE_CTP
#define M_TICKER "rb1901"
#define M_EXCHANGE EXCHANGE_SHFE

class Blind: public IWCStrategy
{
protected:
    bool td_connected;
    Plan* plan;
    std::set<int> doingOrders;
    double impossiablePrice = 1e100;
public:
    virtual void init();
    virtual void on_market_data(const LFMarketDataField* data, short source, long rcv_time);
    virtual void on_rsp_position(const PosHandlerPtr posMap, int request_id, short source, long rcv_time);
    virtual void on_rtn_trade(const LFRtnTradeField* data, int request_id, short source, long rcv_time);
    virtual void on_rtn_order(const LFRtnOrderField* data, int request_id, short source, long rcv_time);

public:
    Blind(const string& name, double lossRate, double times);
    friend void callback_func_blind(Blind *str, int param);
};

void callback_func_blind(Blind *str, int param)
{
    std::cout << "ready to start!" << std::endl;
    KF_LOG_INFO(str->logger, "ready to start! test for param: " << param);
}

Blind::Blind(const string& name, double lossRate, double times): IWCStrategy(name)
{
    srand(time(0));
    int r = rand();
    double profitRate = lossRate * times;
    // todo : reload saved
    KF_LOG_INFO(logger, "rand :"<< r << " lossRate: " << lossRate << " times: " << times);
    char_64 planName = "needAName";
    plan = new Plan(planName, lossRate, profitRate);
    if(r % 2 == 0){
        plan->resetAsBuy();
    }else{
        plan->resetAsSell();
    }
}

void Blind::init()
{
    data->add_market_data(SOURCE_INDEX);
    data->add_register_td(SOURCE_INDEX);
    vector<string> tickers;
    tickers.push_back(M_TICKER);
    util->subscribeMarketData(tickers, SOURCE_INDEX);
    // necessary initialization of internal fields.
    td_connected = false;
    BLCallback bl = boost::bind(callback_func_blind, this, 998);
    util->insert_callback(kungfu::yijinjing::getNanoTime() + 2* 1e9, bl);
}

void Blind::on_rsp_position(const PosHandlerPtr posMap, int request_id, short source, long rcv_time)
{
    if (request_id == -1 && source == SOURCE_INDEX)
    {
        td_connected = true;
        KF_LOG_INFO(logger, "td connected");
        if (posMap.get() == nullptr)
        {
            data->set_pos(PosHandlerPtr(new PosHandler(source)), source);
        }
    }
    else
    {
        KF_LOG_DEBUG(logger, "[RSP_POS] " << posMap->to_string());
    }
}

void Blind::on_market_data(const LFMarketDataField* md, short source, long rcv_time) {
    if (td_connected) {
        if(md->AskPrice1 > impossiablePrice || md->BidPrice1 > impossiablePrice || md->OpenPrice > impossiablePrice ){
            KF_LOG_INFO(logger, "impossiable tick, ignored");
        }else {
            if (strcmp(M_TICKER, md->InstrumentID) == 0) { // maybe many kinds
                if (doingOrders.size() > 0) { // some order is itill doing
                    KF_LOG_INFO(logger, "[on tick] some order is still doing");
                    return;
                } else {
                    if (plan->direction == LONG_IN) {
                        if (md->LastPrice > plan->inPrice) {
                            KF_LOG_INFO(logger, "will long in " << md->LastPrice << " @ " << md->UpdateTime);
                            int oid = util->insert_limit_order(SOURCE_INDEX, M_TICKER, M_EXCHANGE,
                                                               md->UpperLimitPrice, 1,
                                                               LF_CHAR_Buy, LF_CHAR_Open);
                            doingOrders.insert(oid);
                        } else if (md->LastPrice < plan->outPrice) {
                            if (strcmp(md->TradingDay, plan->lastInDate) != 0) { // all yesterday or far
                                KF_LOG_INFO(logger,
                                            "will long out all yesterday " << md->LastPrice << " @ " << md->UpdateTime);
                                int oid = util->insert_limit_order(SOURCE_INDEX, M_TICKER, M_EXCHANGE,
                                                                   md->LowerLimitPrice,
                                                                   plan->todayVolume + plan->yesterdayVolume,
                                                                   LF_CHAR_Sell, LF_CHAR_CloseYesterday);
                                doingOrders.insert(oid);
                            } else {// some volume is today
                                if (plan->todayVolume > 0) {
                                    KF_LOG_INFO(logger,
                                                "will long out today " << md->LastPrice << " @ " << md->UpdateTime);
                                    int oidT = util->insert_limit_order(SOURCE_INDEX, M_TICKER, M_EXCHANGE,
                                                                        md->LowerLimitPrice, plan->todayVolume,
                                                                        LF_CHAR_Sell, LF_CHAR_CloseToday);
                                    doingOrders.insert(oidT);
                                } else {
                                    KF_LOG_INFO(logger, "no today volume");
                                }
                                if (plan->yesterdayVolume > 0) {
                                    KF_LOG_INFO(logger,
                                                "will long out yesterday " << md->LastPrice << " @ " << md->UpdateTime);
                                    int oidY = util->insert_limit_order(SOURCE_INDEX, M_TICKER, M_EXCHANGE,
                                                                        md->LowerLimitPrice, plan->yesterdayVolume,
                                                                        LF_CHAR_Sell, LF_CHAR_CloseYesterday);
                                    doingOrders.insert(oidY);
                                } else {
                                    KF_LOG_INFO(logger, "no yesterday volume");
                                }
                            }
                        } else { // no in, no out, maybe update out price
                            double maybeNewOut = md->LastPrice * (1 - plan->lossRate);
                            if (maybeNewOut > plan->outPrice) {
                                double lastInPrice = plan->inPrices[plan->inPrices.size() - 1];
                                KF_LOG_INFO(logger, "[on tick] update long out "
                                        << plan->outPrice << " --> " << maybeNewOut
                                        << " => " << maybeNewOut - lastInPrice);
                                plan->outPrice = maybeNewOut;
                            }
                        }
                    } else if (plan->direction == SHORT_IN) {
                        if (md->LastPrice < plan->inPrice) {
                            KF_LOG_INFO(logger, "will short in " << md->LastPrice << " @ " << md->UpdateTime);
                            int oid = util->insert_limit_order(SOURCE_INDEX, M_TICKER, M_EXCHANGE,
                                                               md->LowerLimitPrice, 1,
                                                               LF_CHAR_Sell, LF_CHAR_Open);
                            doingOrders.insert(oid);
                        } else if (md->LastPrice > plan->outPrice) {
                            if (strcmp(md->TradingDay, plan->lastInDate) != 0) { // all yesterday or far
                                KF_LOG_INFO(logger, "will short out all yesterday " << md->LastPrice << " @ "
                                                                                    << md->UpdateTime);
                                int oid = util->insert_limit_order(SOURCE_INDEX, M_TICKER, M_EXCHANGE,
                                                                   md->LowerLimitPrice,
                                                                   plan->todayVolume + plan->yesterdayVolume,
                                                                   LF_CHAR_Buy, LF_CHAR_CloseYesterday);
                                doingOrders.insert(oid);
                            } else {// some volume is today
                                if (plan->todayVolume > 0) {
                                    KF_LOG_INFO(logger,
                                                "will short out today " << md->LastPrice << " @ " << md->UpdateTime);
                                    int oidT = util->insert_limit_order(SOURCE_INDEX, M_TICKER, M_EXCHANGE,
                                                                        md->LowerLimitPrice, plan->todayVolume,
                                                                        LF_CHAR_Buy, LF_CHAR_CloseToday);
                                    doingOrders.insert(oidT);
                                } else {
                                    KF_LOG_INFO(logger, "no today volume");
                                }
                                if (plan->yesterdayVolume > 0) {
                                    KF_LOG_INFO(logger,
                                                "will short out yesterday " << md->LastPrice << " @ "
                                                                            << md->UpdateTime);
                                    int oidY = util->insert_limit_order(SOURCE_INDEX, M_TICKER, M_EXCHANGE,
                                                                        md->LowerLimitPrice, plan->yesterdayVolume,
                                                                        LF_CHAR_Buy, LF_CHAR_CloseYesterday);
                                    doingOrders.insert(oidY);
                                } else {
                                    KF_LOG_INFO(logger, "no yesterday volume");
                                }
                            }
                        } else { // no in, no out, maybe update out price
                            double maybeNewOut = md->LastPrice * (1 - plan->lossRate);
                            if (maybeNewOut < plan->outPrice) {
                                double lastInPrice = plan->inPrices[plan->inPrices.size() - 1];
                                KF_LOG_INFO(logger, "[on tick] update short out "
                                        << plan->outPrice << " --> " << maybeNewOut
                                        << " => " << lastInPrice - maybeNewOut);
                                plan->outPrice = maybeNewOut;
                            }
                        }
                    } else {
                        KF_LOG_FATAL(logger, "[on tick] get plan direction " << plan->direction);
                    }
                }
            }
        }
    }else{
        KF_LOG_FATAL(logger, "[wizard] not connected");
    }
}

void Blind::on_rtn_trade(const LFRtnTradeField* rtn_trade, int request_id, short source, long rcv_time)
{
    KF_LOG_DEBUG(logger, "[TRADE]" << " (t)" << rtn_trade->InstrumentID << " (p)" << rtn_trade->Price
                                   << " (v)" << rtn_trade->Volume << " POS:" << data->get_pos(source)->to_string());
    if(rtn_trade->OffsetFlag == LF_CHAR_Open){
        plan->inPrices.push_back(rtn_trade->Price);
        if(strcmp(plan->lastInDate, rtn_trade->TradingDay) == 0){ //same day
            plan->todayVolume += rtn_trade->Volume;
        }else{// not same day
            plan->yesterdayVolume += plan->todayVolume;
            plan->todayVolume = rtn_trade->Volume;
            strcpy(plan->lastInDate, rtn_trade->TradingDay);
        }
        if(rtn_trade->Direction == LF_CHAR_Buy){
            double newIn = rtn_trade->Price * (1 + plan->profitRate);
            KF_LOG_INFO(logger, "[blind] update buy in "<< plan->inPrice << "-->" << newIn);
            plan->inPrice = newIn;
        }else if (rtn_trade->Direction == LF_CHAR_Sell){
            double newIn = rtn_trade->Price * (1 - plan->profitRate);
            KF_LOG_INFO(logger, "[blind] update sell in "<< plan->inPrice << "-->" << newIn);
            plan->inPrice = newIn;
        }else{
            KF_LOG_FATAL(logger, "[TRADE] direction " << rtn_trade->OffsetFlag << " complete, dont know what to do");
        }
    }else if(rtn_trade->OffsetFlag == LF_CHAR_CloseYesterday ||
    rtn_trade->OffsetFlag == LF_CHAR_CloseToday ||
    rtn_trade->OffsetFlag == LF_CHAR_Close){ // every kind of close
        if(rtn_trade->Direction == LF_CHAR_Buy){
            KF_LOG_INFO(logger, "[blind] over trade, add sell");
            double sum = 0;
            for(std::vector<double>::iterator p = plan->inPrices.begin(); p != plan->inPrices.end(); p++){
                sum += rtn_trade->Price - *p;
            }
            KF_LOG_INFO(logger, "[blind] " << plan->inPrices.size() << " -> " << rtn_trade->Price << " ==>" << sum);
            plan->resetAsSell();
        }else if(rtn_trade->Direction == LF_CHAR_Sell){
            KF_LOG_INFO(logger, "[blind] over trade, add buy");
            double sum = 0;
            for(std::vector<double>::iterator p = plan->inPrices.begin(); p != plan->inPrices.end(); p++){
                sum += *p - rtn_trade->Price;
            }
            KF_LOG_INFO(logger, "[blind] " << plan->inPrices.size() << " -> " << rtn_trade->Price << " ==>" << sum);
            plan->resetAsBuy();
        }
    } else {
        KF_LOG_FATAL(logger, "[TRADE] Offset " << rtn_trade->OffsetFlag << " complete, dont know what to do");
    }
}

void Blind::on_rtn_order(const LFRtnOrderField* data, int request_id, short source, long rcv_time)
{
    if (data->OrderStatus == LF_CHAR_AllTraded) {
        KF_LOG_INFO(logger, "[order] "<< request_id << " complete.");
        doingOrders.erase(request_id);
    }else{
        KF_LOG_ERROR(logger, " [order] status" << data->OrderStatus << "(order_id)" << request_id << " (source)" << source);
    }
}

int main(int argc, const char* argv[])
{
    double lossRate = 0.007;
    double times = 4;
    if(argc >= 3){
        std::istringstream iss(argv[1]);
        iss>>lossRate;
        std::istringstream iss2(argv[2]);
        iss2>>times;
    }
    Blind str(string("copy_blind"), lossRate, times);
    str.init();
    str.start();
    str.block();
    return 0;
}