//
// Created by liuzhe on 2018-10-25.
//

#ifndef COPY_BLIND_WIZARDUTIL_H
#define COPY_BLIND_WIZARDUTIL_H


#include "Other.h"
#include "TimeSegment.h"

class WizardUtil {
public:
    WizardUtil();
    bool tickValidate(const LFMarketDataField* md);
    bool tickTimeValidate(const LFMarketDataField* md);
    bool tickDayValidate(const LFMarketDataField* md);
    std::vector<TimeSegment> shfeNormal;
};


#endif //COPY_BLIND_WIZARDUTIL_H
