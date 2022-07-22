#include <bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
using namespace std;

#define SYSTEM_CIPHER_SIZE 32

class Chunker {
public:
    /*chunk size setting*/
    uint64_t avgChunkSize_ = 8192;
    uint64_t minChunkSize_ = 4096;
    uint64_t maxChunkSize_ = 16384;

    uint64_t ReadSize_ = 256 * 1024 * 1024;

    /*VarSize chunking*/
    /*sliding window size*/
    uint64_t slidingWinSize_ = 48;
    uint32_t polyBase_;
    /*the modulus for limiting the value of the polynomial in rolling hash*/
    uint32_t polyMOD_;
    /*note: to avoid overflow, _polyMOD*255 should be in the range of "uint32_t"*/
    /*      here, 255 is the max value of "unsigned char"                       */
    /*the lookup table for accelerating the power calculation in rolling hash*/
    uint32_t* powerLUT_;
    /*the lookup table for accelerating the byte remove in rolling hash*/
    uint32_t* removeLUT_;
    /*the mask for determining an anchor*/
    uint32_t anchorMask_;
    /*the value for determining an anchor*/
    uint32_t anchorValue_;

    set<string> chunkHashList;
    unordered_map<string, uint32_t> chunkHashMap;

    Chunker();
    ~Chunker();
};

Chunker::Chunker()
{
    int numOfMaskBits;
    polyBase_ = 257; /*a prime larger than 255, the max value of "unsigned char"*/
    polyMOD_ = (1 << 23) - 1; /*polyMOD_ - 1 = 0x7fffff: use the last 23 bits of a polynomial as its hash*/
    /*initialize the lookup table for accelerating the power calculation in rolling hash*/
    powerLUT_ = (uint32_t*)malloc(sizeof(uint32_t) * slidingWinSize_);
    /*powerLUT_[i] = power(polyBase_, i) mod polyMOD_*/
    powerLUT_[0] = 1;
    for (int i = 1; i < slidingWinSize_; i++) {
        /*powerLUT_[i] = (powerLUT_[i-1] * polyBase_) mod polyMOD_*/
        powerLUT_[i] = (powerLUT_[i - 1] * polyBase_) & polyMOD_;
        // cout << powerLUT_[i] << endl;
    }
    /*initialize the lookup table for accelerating the byte remove in rolling hash*/
    removeLUT_ = (uint32_t*)malloc(sizeof(uint32_t) * 256); /*256 for unsigned char*/
    for (int i = 0; i < 256; i++) {
        /*removeLUT_[i] = (- i * powerLUT_[_slidingWinSize-1]) mod polyMOD_*/
        removeLUT_[i] = (i * powerLUT_[slidingWinSize_ - 1]) & polyMOD_;
        if (removeLUT_[i] != 0) {
            removeLUT_[i] = (polyMOD_ - removeLUT_[i] + 1) & polyMOD_;
        }
        /*note: % is a remainder (rather than modulus) operator*/
        /*      if a < 0,  -polyMOD_ < a % polyMOD_ <= 0       */
    }
    /*initialize the anchorMask_ for depolytermining an anchor*/
    /*note: power(2, numOfanchorMaskBits) = avgChunkSize_*/
    numOfMaskBits = 1;
    while ((avgChunkSize_ >> numOfMaskBits) != 1) {
        numOfMaskBits++;
    }
    // cerr << "mask size = " << numOfMaskBits << endl;
    anchorMask_ = (1 << numOfMaskBits) - 1;
    /*initialize the value for depolytermining an anchor*/
    anchorValue_ = 0;
}

Chunker::~Chunker()
{
    free(powerLUT_);
    free(removeLUT_);
    chunkHashList.clear();
    chunkHashMap.clear();
}
