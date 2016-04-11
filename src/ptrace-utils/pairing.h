//  Different utils
//  Copyright (C) 2016  Sayutin Dmitry
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
