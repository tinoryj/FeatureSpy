#ifndef FEATURESPY_DEDUPCORE_HPP
#define FEATURESPY_DEDUPCORE_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "database.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include <bits/stdc++.h>

using namespace std;

class DedupCore {
public:
    DedupCore();
    ~DedupCore();
    bool dedupByHash(u_char* inputHashList, int chunkNumber, bool* out, int& requiredChunkNumber);
};

#endif //FEATURESPY_DEDUPCORE_HPP
