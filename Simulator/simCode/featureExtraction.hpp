#include <bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
using namespace std;

#define SYSTEM_CIPHER_SIZE 32
#define FEATURE_NUMBER 12
#define FEATURE_NUMBER_PER_SF 4
#define SF_NUMBER FEATURE_NUMBER / FEATURE_NUMBER_PER_SF
int featureNumber = SF_NUMBER;
int prefixBlockNumber = 2;
class FeatureGen {
private:
    /*VarSize chunking*/
    /*sliding window size*/
    uint64_t slidingWinSize_ = 64;
    uint64_t polyBase_;
    /*the modulus for limiting the value of the polynomial in rolling hash*/
    uint64_t polyMOD_;
    /*note: to avoid overflow, _polyMOD*255 should be in the range of "uint64_t"*/
    /*      here, 255 is the max value of "unsigned char"                       */
    /*the lookup table for accelerating the power calculation in rolling hash*/
    uint64_t* powerLUT_;
    /*the lookup table for accelerating the byte remove in rolling hash*/
    uint64_t* removeLUT_;
    int mSet[20] = { 3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511, 3517, 3527, 3529, 3533, 3539, 3541, 3547, 3557, 3559, 3571 };
    int aSet[20] = { 3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181, 3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257 };

public:
    FeatureGen();
    ~FeatureGen();
    bool getFeatureList(u_char* chunkBuffer, int chunkSize, vector<uint64_t>& featureList);
};

FeatureGen::FeatureGen()
{
    polyBase_ = 257; /*a prime larger than 255, the max value of "unsigned char"*/
    polyMOD_ = UINT64_MAX; /*polyMOD_ - 1 = 0x7fffff: use the last 23 bits of a polynomial as its hash*/
    /*initialize the lookup table for accelerating the power calculation in rolling hash*/
    powerLUT_ = (uint64_t*)malloc(sizeof(uint64_t) * slidingWinSize_);
    /*powerLUT_[i] = power(polyBase_, i) mod polyMOD_*/
    powerLUT_[0] = 1;
    for (int i = 1; i < slidingWinSize_; i++) {
        /*powerLUT_[i] = (powerLUT_[i-1] * polyBase_) mod polyMOD_*/
        powerLUT_[i] = (powerLUT_[i - 1] * polyBase_) & polyMOD_;
        // cout << powerLUT_[i] << endl;
    }
    /*initialize the lookup table for accelerating the byte remove in rolling hash*/
    removeLUT_ = (uint64_t*)malloc(sizeof(uint64_t) * 256); /*256 for unsigned char*/
    for (int i = 0; i < 256; i++) {
        /*removeLUT_[i] = (- i * powerLUT_[_slidingWinSize-1]) mod polyMOD_*/
        removeLUT_[i] = (i * powerLUT_[slidingWinSize_ - 1]) & polyMOD_;
        if (removeLUT_[i] != 0) {
            removeLUT_[i] = (polyMOD_ - removeLUT_[i] + 1) & polyMOD_;
        }
        /*note: % is a remainder (rather than modulus) operator*/
        /*      if a < 0,  -polyMOD_ < a % polyMOD_ <= 0       */
    }
}

FeatureGen::~FeatureGen()
{
    free(powerLUT_);
    free(removeLUT_);
}

bool FeatureGen::getFeatureList(u_char* chunkBufferIn, int chunkSize, vector<uint64_t>& featureList)
{
    int chunkBufferCnt = 0;
    uint64_t winFp = 0;
    /*full fill sliding window*/
    vector<uint64_t> fpList;
    u_char chunkBuffer[chunkSize];
    memcpy(chunkBuffer, chunkBufferIn, chunkSize);
    while (chunkBufferCnt < chunkSize) {
        if (chunkBufferCnt < slidingWinSize_) {
            winFp = winFp + (chunkBuffer[chunkBufferCnt] * powerLUT_[slidingWinSize_ - chunkBufferCnt - 1]) & polyMOD_;
            chunkBufferCnt++;
            continue;
        }
        winFp &= (polyMOD_);
        fpList.push_back(winFp);

        unsigned short int v = chunkBuffer[chunkBufferCnt - slidingWinSize_]; // queue
        winFp = ((winFp + removeLUT_[v]) * polyBase_ + chunkBuffer[chunkBufferCnt]) & polyMOD_; //remove queue front and add queue tail
        chunkBufferCnt++;
    }
    uint64_t modNumber = UINT_MAX;
    for (auto i = 0; i < FEATURE_NUMBER; i++) {
        uint64_t maxPi = 0;
        uint64_t targetFp = 0;
        for (auto j = 0; j < fpList.size(); j++) {
            uint64_t Pi = (mSet[i] * fpList[j] + aSet[i]) & modNumber;
            if (maxPi < Pi) {
                maxPi = Pi;
                targetFp = fpList[j];
            }
        }
        featureList.push_back(targetFp);
    }
    fpList.clear();
    return true;
}