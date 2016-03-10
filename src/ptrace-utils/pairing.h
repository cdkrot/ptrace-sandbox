#ifndef PTSANDBOX_UTILS_PAIRING_H
#define PTSANDBOX_UTILS_PAIRING_H

#include "attributes.h"

#define DECLAIR_PAIR(T1, T2) \
    struct TEMPLATED2(pair, T1, T2) { \
        T1 first; \
        T2 second; \
    }; \
    TEMPLATED2(pair, T1, T2) make_pair(T1 f, T2 s) { \
        TEMPLATED2(pair, T1, T2) res; \
        res.first = f, res.second = s; \
    } 

#endif
