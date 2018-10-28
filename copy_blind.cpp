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
#include "WizardUtil.h"
#include "FileNpc.h"


#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>


USING_WC_NAMESPACE
USING_YJJ_NAMESPACE

#define SOURCE_INDEX SOURCE_CTP
// todo : read from command line exchange

class Blind: public IWCStrategy
{
protected:
    bool td_connected;
    Plan* plan;
    std::set<int> doingOrders;
    double impossiablePrice = 1e100;
    WizardUtil wizardUtil;
    Npc* fakeUtil;
    char_31 ticker;
public:
    virtual void init();
    virtual void on_market_data(const LFMarketDataField* data, short source, long rcv_time);
    virtual void on_rsp_position(const PosHandlerPtr posMap, int request_id, short source, long rcv_time);
    virtual void on_rtn_trade(const LFRtnTradeField* data, int request_id, short source, long rcv_time);
    virtual void on_rtn_order(const LFRtnOrderField* data, int request_id, short source, long rcv_time);
    virtual int limit_order(short source, string instrument_id, string exchange_id, double price, int volume,
                                    LfDirectionType direction, LfOffsetFlagType offset);

public:
    // todo : support tick list
    Blind(const string& name, double lossRate, double times, std::string ticker, Npc * util = NULL);
    friend void callback_func_blind(Blind *str, int param);

    void on_rsp_order(const LFInputOrderField *data, int request_id, short source, long rcv_time, short errorId,
                      const char *errorMsg) override;
};

void callback_func_blind(Blind *str, int param)
{
    std::cout << "ready to start!" << std::endl;
    KF_LOG_INFO(str->logger, "ready to start! test for param: " << param);
}

int Blind::limit_order(short source, string instrument_id, string exchange_id, double price, int volume,
                LfDirectionType direction, LfOffsetFlagType offset){
    if(fakeUtil == NULL){
        return util->insert_limit_order(source, instrument_id, exchange_id, price, volume, direction, offset);
    } else {
        return fakeUtil->fake_limit_order(source, instrument_id, exchange_id, price, volume, direction, offset);
    }
}

Blind::Blind(const string& name, double lossRate, double times, std::string ticker, Npc * util): IWCStrategy(name)
{
    if(util != nullptr){ // for back testing
        memcpy(&this->fakeUtil, &util, sizeof(Npc));
        td_connected = true;
    }
    strcpy(this->ticker, ticker.data());
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
    tickers.push_back(ticker);
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
        if(md->AskPrice1 > impossiablePrice ||
        md->BidPrice1 > impossiablePrice ||
        md->OpenPrice > impossiablePrice){
            KF_LOG_INFO(logger, "impossiable tick price (a/b/o) " << md->AskPrice1 << "/" << md->BidPrice1 << "/" << md->OpenPrice << " ignored");
        } else if( ! wizardUtil.tickValidate(md)){
            KF_LOG_INFO(logger, "impossiable tick time, ignored (u) " << md->UpdateTime);
        } else {
            if (strcmp(ticker, md->InstrumentID) == 0) { // maybe many kinds
                if (doingOrders.size() > 0) { // some order is itill doing
                    KF_LOG_INFO(logger, "[on tick] some order is still doing ---------------");
                    for(std::set<int>::iterator i = doingOrders.begin(); i != doingOrders.end(); ++i){
                        KF_LOG_INFO(logger, "[on tick] order " << *i);
                    }
                    return;
                } else {
                    if (plan->direction == LONG_IN) {
                        if (md->LastPrice > plan->inPrice) {
                            KF_LOG_INFO(logger, "will long in " << md->LastPrice << " @ " << md->UpdateTime);
                            int oid = limit_order(SOURCE_INDEX, md->InstrumentID, md->ExchangeID,
                                                               md->UpperLimitPrice, 1,
                                                               LF_CHAR_Buy, LF_CHAR_Open);
                            doingOrders.insert(oid);
                        } else if (md->LastPrice < plan->outPrice) {
                            KF_LOG_INFO(logger, md->LastPrice << " < " << plan->outPrice);
                            double usePrice = md->LowerLimitPrice;
                            if (strcmp(md->TradingDay, plan->lastInDate) != 0) { // all yesterday or far
                                int v = plan->todayVolume + plan->yesterdayVolume;
                                KF_LOG_INFO(logger, "will long out all yesterday " << v << " @ " << md->UpdateTime);
                                int oid = limit_order(SOURCE_INDEX, md->InstrumentID, md->ExchangeID,
                                                                   usePrice, v,
                                                                   LF_CHAR_Sell, LF_CHAR_CloseYesterday);
                                doingOrders.insert(oid);
                            } else {// some volume is today
                                if (plan->todayVolume > 0) {
                                    KF_LOG_INFO(logger,
                                                "will long out today " << md->LastPrice << " @ " << md->UpdateTime);
                                    int oidT = limit_order(SOURCE_INDEX, md->InstrumentID, md->ExchangeID,
                                                                        usePrice, plan->todayVolume,
                                                                        LF_CHAR_Sell, LF_CHAR_CloseToday);
                                    doingOrders.insert(oidT);
                                } else {
                                    KF_LOG_INFO(logger, "no today volume");
                                }
                                if (plan->yesterdayVolume > 0) {
                                    KF_LOG_INFO(logger,
                                                "will long out yesterday " << md->LastPrice << " @ " << md->UpdateTime);
                                    int oidY = limit_order(SOURCE_INDEX, md->InstrumentID, md->ExchangeID,
                                                                        usePrice, plan->yesterdayVolume,
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
                            int oid = limit_order(SOURCE_INDEX, md->InstrumentID, md->ExchangeID,
                                                               md->LowerLimitPrice, 1,
                                                               LF_CHAR_Sell, LF_CHAR_Open);
                            doingOrders.insert(oid);
                        } else if (md->LastPrice > plan->outPrice) {
                            KF_LOG_INFO(logger, md->LastPrice << " > " << plan->outPrice);
                            double usePrice = md->UpperLimitPrice;
                            if (strcmp(md->TradingDay, plan->lastInDate) != 0) { // all yesterday or far
                                int v = plan->yesterdayVolume + plan->todayVolume;
                                KF_LOG_INFO(logger, "will short out all yesterday " << v << " @ " << md->UpdateTime);
                                int oid = limit_order(SOURCE_INDEX, md->InstrumentID, md->ExchangeID,
                                                                   usePrice, v,
                                                                   LF_CHAR_Buy, LF_CHAR_CloseYesterday);
                                doingOrders.insert(oid);
                            } else {// some volume is today
                                if (plan->todayVolume > 0) {
                                    KF_LOG_INFO(logger,
                                                "will short out today " << md->LastPrice << " @ " << md->UpdateTime);
                                    int oidT = limit_order(SOURCE_INDEX, md->InstrumentID, md->ExchangeID,
                                                                        usePrice, plan->todayVolume,
                                                                        LF_CHAR_Buy, LF_CHAR_CloseToday);
                                    doingOrders.insert(oidT);
                                } else {
                                    KF_LOG_INFO(logger, "no today volume");
                                }
                                if (plan->yesterdayVolume > 0) {
                                    KF_LOG_INFO(logger,
                                                "will short out yesterday " << md->LastPrice << " @ "
                                                                            << md->UpdateTime);
                                    int oidY = limit_order(SOURCE_INDEX, md->InstrumentID, md->ExchangeID,
                                                                        usePrice, plan->yesterdayVolume,
                                                                        LF_CHAR_Buy, LF_CHAR_CloseYesterday);
                                    doingOrders.insert(oidY);
                                } else {
                                    KF_LOG_INFO(logger, "no yesterday volume");
                                }
                            }
                        } else { // no in, no out, maybe update out price
                            double maybeNewOut = md->LastPrice * (1 + plan->lossRate);
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

void Blind::on_rsp_order(const LFInputOrderField *data, int request_id, short source, long rcv_time, short errorId,
                         const char *errorMsg) {
    KF_LOG_INFO(logger, "[rsp order] (r) " << request_id << " (s) " << source << " (e) " << errorId);
    IWCStrategy::on_rsp_order(data, request_id, source, rcv_time, errorId, errorMsg);
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
            KF_LOG_INFO(logger, "[blind] update next long in "<< plan->inPrice << "-->" << newIn);
            plan->inPrice = newIn;
        }else if (rtn_trade->Direction == LF_CHAR_Sell){
            double newIn = rtn_trade->Price * (1 - plan->profitRate);
            KF_LOG_INFO(logger, "[blind] update next short in "<< plan->inPrice << "-->" << newIn);
            plan->inPrice = newIn;
        }else{
            KF_LOG_FATAL(logger, "[TRADE] direction " << rtn_trade->OffsetFlag << " complete, dont know what to do");
        }
        // consider every kind of close
    }else if(rtn_trade->OffsetFlag == LF_CHAR_CloseYesterday ||
    rtn_trade->OffsetFlag == LF_CHAR_CloseToday ||
    rtn_trade->OffsetFlag == LF_CHAR_Close){
        if(rtn_trade->Direction == LF_CHAR_Buy){ // buy close, add buy
            KF_LOG_INFO(logger, "[blind] over short, add long");
            double sum = 0;
            for(std::vector<double>::iterator p = plan->inPrices.begin(); p != plan->inPrices.end(); p++){
                sum += rtn_trade->Price - *p;
            }
            KF_LOG_INFO(logger, "[blind] " << plan->inPrices.size() << " -> " << rtn_trade->Price << " ==>" << sum);
            plan->resetAsBuy();
        }else if(rtn_trade->Direction == LF_CHAR_Sell){ // sell close, add sell
            KF_LOG_INFO(logger, "[blind] over long, add short");
            double sum = 0;
            for(std::vector<double>::iterator p = plan->inPrices.begin(); p != plan->inPrices.end(); p++){
                sum += *p - rtn_trade->Price;
            }
            KF_LOG_INFO(logger, "[blind] " << plan->inPrices.size() << " -> " << rtn_trade->Price << " ==>" << sum);
            plan->resetAsSell();
        }
    } else {
        KF_LOG_FATAL(logger, "[TRADE] Offset " << rtn_trade->OffsetFlag << " complete, dont know what to do");
    }
}

void Blind::on_rtn_order(const LFRtnOrderField* data, int request_id, short source, long rcv_time)
{
    if (data->OrderStatus == LF_CHAR_AllTraded) {
        KF_LOG_INFO(logger, "[order] "<< request_id << " complete, erase it.");
        doingOrders.erase(request_id);
    }else{
        KF_LOG_ERROR(logger, " [order] status " << data->OrderStatus << "(order_id)" << request_id << " (source)" << source);
    }
}

int main(int argc, const char* argv[])
{
    double lossRate;
    double times;
    std::string filePath;
    std::string ticker;

    boost::program_options::options_description desc("Options");

    desc.add_options()
            ("help,h", "produce help message")
            ("loss,l", boost::program_options::value<double>(&lossRate)->default_value(0.007), "set loss rate")
            ("times,t", boost::program_options::value<double>(&times)->default_value(4), "set p/l times")
            ("tickers,k", boost::program_options::value<std::string>(&ticker)->default_value("rb1710"), "set tickers")
            ("file,f", boost::program_options::value<std::string>(&filePath)->default_value(""), "back test file");

    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
    }
    catch (boost::exception& e) {
        std::cerr << boost::diagnostic_information(e) << std::endl;
        return 1;
    }

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    } else {

        if (filePath.size() == 0) {
            Blind str(string("copy_blind"), lossRate, times, ticker);
            str.init();
            str.start();
            str.block();
            return 0;
        } else {
            FileNpc * inner = new FileNpc(filePath.data());
            Blind str(string("copy_blind"), lossRate, times, ticker, inner);
            inner->setStrategy(&str);
            inner->run();
            delete inner;
            return 0;
        }
    }
}