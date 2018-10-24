//
// Created by liuzhe on 2018-10-22.
//

#ifndef COPY_BLIND_PLAN_H
#define COPY_BLIND_PLAN_H


#include <vector>
#include <longfist/LFDataStruct.h>
#include "Signal.h"
#include <fstream>
#include <iostream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

class Plan {

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        //ar & name;
        ar & useMin;
        ar & useMax;
        ar & todayVolume;
        ar & yesterdayVolume;
        ar & lastInDate;
        ar & lossRate;
        ar & profitRate;
        ar & inPrice;
        ar & outPrice;
        ar & direction;
        //ar & inPrices;
    }
public:
    Plan(char_64 name, double lossRate, double profitRate);
    void resetAsBuy();
    void resetAsSell();
    void save();
    void load();


    static constexpr double useMin = 0.88;
    static constexpr double useMax = 88888888.88;
    int todayVolume = 0;
    int yesterdayVolume = 0;
    char_13 lastInDate = {'0'};
    char_64 name;
    double profitRate = 0.028F;
    double lossRate = 0.007F;
    double inPrice;
    double outPrice;
    Signal direction = IMPOSSIABLE;
    std::vector<double> inPrices;
};

#endif //COPY_BLIND_PLAN_H
