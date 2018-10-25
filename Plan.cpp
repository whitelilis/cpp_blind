//
// Created by liuzhe on 2018-10-22.
//

#include "Plan.h"

Plan::Plan(char_64 name, double lossRate, double profitRate):lossRate(lossRate), profitRate(profitRate) {
    strcpy(this->name, name);
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
