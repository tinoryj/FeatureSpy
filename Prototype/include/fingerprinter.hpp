#ifndef FEATURESPY_FINGERPRINTER_HPP
#define FEATURESPY_FINGERPRINTER_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "keyClient.hpp"
#include "messageQueue.hpp"

class Fingerprinter {
private:
    messageQueue<Data_t>* inputMQ_;
    uint64_t mSet[16] = { 3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511, 3517, 3527, 3529, 3533, 3539, 3541 };
    uint64_t aSet[16] = { 3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181, 3187, 3191, 3203, 3209, 3217, 3221 };
    int keySeedSubFeatureNumber_;
    int keySeedSuperFeatureNumber_;
    int featureNumberPerSuperFeature_;
    int slidingWinSize_;
    uint64_t polyBase_;
    uint64_t polyMOD_;
    uint64_t* powerLUT_;
    uint64_t* removeLUT_;
    KeyClient* keyClientObj_;
    CryptoPrimitive* cryptoObj_;

    int jobDoneFlagCounterForFpGen_ = 0;
    std::mutex mutexFpGenJobFlag_;

#if SYSTEM_BREAK_DOWN == 1
    double* featureGenerateWorkTime;
    double* fpListGenerateWorkTime;
#endif

public:
    Fingerprinter(KeyClient* keyClientObjTemp);
    ~Fingerprinter();
    void runFingerprintGen(int index);
    bool insertMQ(Data_t& newChunk);
    bool extractMQ(Data_t& newChunk);
    bool initFeatureGen();
    bool extractSFList(uint64_t fpList[], int chunkSize, vector<string>& superFeatureList);
    bool fpListGen(u_char* chunkBuffer, uint64_t chunkSize, uint64_t fpList[]);
    bool editJobDoneFlag();
};

#endif //FEATURESPY_FINGERPRINTER_HPP
