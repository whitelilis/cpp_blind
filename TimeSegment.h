//
// Created by liuzhe on 2018-10-25.
//

#ifndef COPY_BLIND_TIMEHELPER_H
#define COPY_BLIND_TIMEHELPER_H


class TimeSegment {
public:
    int start;
    int end;

    TimeSegment(int start, int end);
    bool isIn(int hour, int minute);
};


#endif //COPY_BLIND_TIMEHELPER_H
