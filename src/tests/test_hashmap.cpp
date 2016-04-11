//  Util to test hashmaps.
//  Copyright (C) 2016  Vasiliy Alferov
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

#define HASHMAP_SIZE 1791791
#include <hashmap.h>

#include <iostream>
#include <map>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

using namespace std;
    
hashmap hm;

int main()
{
    srand(time(0));
    cout << "Performing hashmap test... ";
    cout.flush();
    hashmap_init(&hm);
    map<int, void*> mp;
    vector<int> ks;
    for (int i = 0; 2 * i < HASHMAP_SIZE; i++)
    {
        int x = rand() % HASHMAP_SIZE;
        long y = rand();
        void* yv = (void*)y;
        hashmap_set(&hm, x, yv);
        mp[x] = yv;
        ks.push_back(x);
    }
    for (int i = 0; 4 * i < HASHMAP_SIZE; i++)
    {
        mp.erase(ks[i]);
        hashmap_remove(&hm, ks[i]);
    }
    for (auto p : mp)
    {
        if (p.second != hashmap_val(&hm, p.first))
        {
            cout << "Failed" << endl;
            return -1;
        }
    }
    cout << "OK" << endl;
    return 0;
}
