//
// Created by liuzhe on 2018-10-25.
//

#include "Other.h"
#include "WizardUtil.h"
#include "TimeSegment.h"


WizardUtil::WizardUtil() {
    TimeSegment day1(540, 615);     // 9:00 - 10:15
    TimeSegment day2(630, 690);     // 10:30 - 11:30
    TimeSegment day3(810, 900);     // 13:30 - 15:00
    TimeSegment night1(1260, 1380); // 21:00 - 23:00

    shfeNormal.push_back(day1);
    shfeNormal.push_back(day2);
    shfeNormal.push_back(day3);
    shfeNormal.push_back(night1);
}


bool WizardUtil::tickDayValidate(const LFMarketDataField* md){
    // todo: using another one
    return md->LastPrice > 0;
}

bool WizardUtil::tickTimeValidate(const LFMarketDataField* md){
    int asciiBase = 48;
    const char * ss = md->UpdateTime;
    int hour = (ss[0] - asciiBase) * 10 + ss[1] - asciiBase;
    int minute = (ss[3] - asciiBase) * 10 + ss[4] - asciiBase;

    for(std::vector<TimeSegment>::iterator i = shfeNormal.begin(); i != shfeNormal.end(); ++i){
        if(i->isIn(hour, minute)){
            printf("true\n");
            return true;
        }
    }
    printf("false\n");
    return false;
}

bool WizardUtil::tickValidate(const LFMarketDataField* md) {
    return tickDayValidate(md) && tickTimeValidate(md);
}
