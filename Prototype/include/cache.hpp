#ifndef FEATURESPY_CACHE_HPP
#define FEATURESPY_CACHE_HPP

#include "boost/thread.hpp"
#include "configure.hpp"
#include "lruCache.hpp"
#include <bits/stdc++.h>

extern Configure config;

using namespace std;

class Cache {
private:
    lru11::Cache<string, uint32_t>* Cache_;
    uint8_t** containerPool_;
    uint64_t cacheSize_ = 0;
    size_t currentIndex_ = 0;
    boost::shared_mutex mtx;

public:
    Cache();
    void insertToCache(string& name, uint8_t* data, uint32_t length);
    bool existsInCache(string& name);
    uint8_t* getFromCache(string& name);
    ~Cache();
};

#endif //FEATURESPY_CACHE_HPP
