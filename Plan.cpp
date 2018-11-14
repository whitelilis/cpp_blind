//
// Created by liuzhe on 2018-10-22.
//

#include "Plan.h"

Plan::Plan(char_64 _name, double _lossRate, double _profitRate){
    strcpy(this->name, _name);
    this->lossRate = _lossRate;
    this->profitRate = _profitRate;
}

void Plan::resetAsBuy() {
    direction = LONG_IN;
    inPrice = useMin;
    outPrice = inPrice * (1 - lossRate);
    strcpy(lastInDate, "");
    inPrices.clear();
    todayVolume = 0;
    yesterdayVolume = 0;
}

void Plan::resetAsSell() {
    direction = SHORT_IN;
    inPrice = useMax;
    outPrice = inPrice * (1 + lossRate);
    strcpy(lastInDate, "");
    inPrices.clear();
    todayVolume = 0;
    yesterdayVolume = 0;
}
