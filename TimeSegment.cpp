//
// Created by liuzhe on 2018-10-25.
//

#include "TimeSegment.h"

bool TimeSegment::isIn(int hour, int minute) {
    int aim = hour * 60 + minute;
    return aim >= start && aim <= end;
}

TimeSegment::TimeSegment(int start, int end) : start(start), end(end) {}