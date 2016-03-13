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
