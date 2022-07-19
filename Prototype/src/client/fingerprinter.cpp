#include "fingerprinter.hpp"
#include "openssl/rsa.h"
#include <sys/time.h>

extern Configure config;

void PRINT_BYTE_ARRAY_FINGERPRINTER(
    FILE* file, void* mem, uint32_t len)
{
    if (!mem || !len) {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t* array = (uint8_t*)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++) {
        fprintf(file, "0x%x, ", array[i]);
        if (i % 8 == 7)
            fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}

Fingerprinter::Fingerprinter(KeyClient* keyClientObjTemp)
{
    keyClientObj_ = keyClientObjTemp;
    cryptoObj_ = new CryptoPrimitive();
    inputMQ_ = new messageQueue<Data_t>;
    initFeatureGen();
    featureNumberPerSuperFeature_ = 4;
    keySeedSubFeatureNumber_ = featureNumberPerSuperFeature_ * config.getKeySeedFeatureNumber();
#if SYSTEM_BREAK_DOWN == 1
    featureGenerateWorkTime = (double*)malloc(config.getKeySeedGenFeatureThreadNumber() * sizeof(double));
    fpListGenerateWorkTime = (double*)malloc(config.getKeySeedGenFeatureThreadNumber() * sizeof(double));
#endif
}

Fingerprinter::~Fingerprinter()
{
#if SYSTEM_BREAK_DOWN == 1
    double totalFpListTime = 0;
    double totalFeatureTime = 0;
    for (int i = 0; i < config.getKeySeedGenFeatureThreadNumber(); i++) {
        cout
            << "Fingerprinter : " << i << "-th thread chunk super-features generate work time = " << featureGenerateWorkTime[i] << " s"
            << ", fingerprint generate work time = " << fpListGenerateWorkTime[i] << endl;
        totalFpListTime += fpListGenerateWorkTime[i];
        totalFeatureTime += featureGenerateWorkTime[i];
    }
    cout
        << "Fingerprinter : total chunk super-features generate work time = " << totalFeatureTime << " s"
        << ", fingerprint generate work time = " << totalFpListTime << endl;
    free(featureGenerateWorkTime);
    free(fpListGenerateWorkTime);
#endif
    delete cryptoObj_;
    inputMQ_->~messageQueue();
    delete inputMQ_;
}

bool Fingerprinter::initFeatureGen()
{
    slidingWinSize_ = 64;
    polyBase_ = 257; /*a prime larger than 255, the max value of "unsigned char"*/
    polyMOD_ = UINT64_MAX; /*polyMOD_ - 1 = 0x7fffff: use the last 23 bits of a polynomial as its hash*/
    /*initialize the lookup table for accelerating the power calculation in rolling hash*/
    powerLUT_ = (uint64_t*)malloc(sizeof(uint64_t) * slidingWinSize_);
    memset(powerLUT_, 0, sizeof(uint64_t) * slidingWinSize_);
    /*powerLUT_[i] = power(polyBase_, i) mod polyMOD_*/
    powerLUT_[0] = 1;
    for (int i = 1; i < slidingWinSize_; i++) {
        /*powerLUT_[i] = (powerLUT_[i-1] * polyBase_) mod polyMOD_*/
        powerLUT_[i] = (powerLUT_[i - 1] * polyBase_) & polyMOD_;
    }
    /*initialize the lookup table for accelerating the byte remove in rolling hash*/
    removeLUT_ = (uint64_t*)malloc(sizeof(uint64_t) * 256); /*256 for unsigned char*/
    memset(removeLUT_, 0, sizeof(uint64_t) * 256);
    for (int i = 0; i < 256; i++) {
        /*removeLUT_[i] = (- i * powerLUT_[_slidingWinSize-1]) mod polyMOD_*/
        removeLUT_[i] = (i * powerLUT_[slidingWinSize_ - 1]) & polyMOD_;
        if (removeLUT_[i] != 0) {

            removeLUT_[i] = (polyMOD_ - removeLUT_[i] + 1) & polyMOD_;
        }
    }
    return true;
}

bool Fingerprinter::fpListGen(u_char* chunkBuffer, uint64_t chunkSize, uint64_t fpList[])
{
    uint64_t winFp = 0;
    for (int i = 0; i < chunkSize; i++) {
        /*full fill sliding window*/
        if (i < slidingWinSize_) {
            winFp = winFp + (chunkBuffer[i] * powerLUT_[slidingWinSize_ - i - 1]) & polyMOD_; // Refer to doc/Chunking.md hash function:RabinChunker
            continue;
        }
        winFp &= (polyMOD_);
        fpList[i] = winFp;
        /*slide window*/
        unsigned short int v = chunkBuffer[i - slidingWinSize_]; //queue
        winFp = ((winFp + removeLUT_[v]) * polyBase_ + chunkBuffer[i]) & polyMOD_; //remove queue front and add queue tail
    }
    return true;
}

void Fingerprinter::runFingerprintGen(int index)
{
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartFingerprinter;
    struct timeval timeendFingerprinter;
    double generateSuperFeatureTime = 0, generateFpListTime = 0;
    long diff;
    double second;
#endif
    bool JobDoneFlag = false;
    while (true) {
        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            JobDoneFlag = true;
            mutexFpGenJobFlag_.lock();
            jobDoneFlagCounterForFpGen_++;
            mutexFpGenJobFlag_.unlock();
        }
        if (extractMQ(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                keyClientObj_->insertMQ(tempChunk);
                continue;
            } else {
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartFingerprinter, NULL);
#endif
                uint64_t fpList[MAX_CHUNK_SIZE];
                vector<string> superFeatureList;
                fpListGen(tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize, fpList);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendFingerprinter, NULL);
                diff = 1000000 * (timeendFingerprinter.tv_sec - timestartFingerprinter.tv_sec) + timeendFingerprinter.tv_usec - timestartFingerprinter.tv_usec;
                second = diff / 1000000.0;
                generateFpListTime += second;
#endif
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartFingerprinter, NULL);
#endif
                bool generateKeySeedStatus = extractSFList(fpList, tempChunk.chunk.logicDataSize, superFeatureList);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendFingerprinter, NULL);
                diff = 1000000 * (timeendFingerprinter.tv_sec - timestartFingerprinter.tv_sec) + timeendFingerprinter.tv_usec - timestartFingerprinter.tv_usec;
                second = diff / 1000000.0;
                generateSuperFeatureTime += second;
#endif
                if (generateKeySeedStatus) {
                    memcpy(tempChunk.chunk.chunkHash, &superFeatureList[0][0], SYSTEM_CIPHER_SIZE);
#if SYSTEM_DEBUG_FLAG == 1
                    cerr << "Fingerprinter : super feature = " << endl;
                    PRINT_BYTE_ARRAY_FINGERPRINTER(stdout, &superFeatureList[0][0], SYSTEM_CIPHER_SIZE);
#endif
                    keyClientObj_->insertMQ(tempChunk);
                } else {
                    cerr << "Fingerprinter : generate cipher chunk hash error, exiting" << endl;
                    return;
                }
            }
        }
        if (JobDoneFlag) {
            if (jobDoneFlagCounterForFpGen_ == config.getKeySeedGenFeatureThreadNumber()) {
                if (!keyClientObj_->editJobDoneFlag()) {
                    cerr << "Fingerprinter : current fp generation threads job done number = " << jobDoneFlagCounterForFpGen_ << endl;
                    cerr << "Fingerprinter : error to set job done flag for encoder" << endl;
                }
            }
            break;
        }
    }
#if SYSTEM_BREAK_DOWN == 1
    featureGenerateWorkTime[index] = generateSuperFeatureTime;
    fpListGenerateWorkTime[index] = generateFpListTime;
#endif
    return;
}

bool Fingerprinter::insertMQ(Data_t& newChunk)
{
    return inputMQ_->push(newChunk);
}

bool Fingerprinter::extractMQ(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
}

bool Fingerprinter::extractSFList(uint64_t fpList[], int chunkSize, vector<string>& superFeatureList)
{
    vector<uint64_t> featureList;
    for (auto i = 0; i < keySeedSubFeatureNumber_; i++) {
        uint64_t maxPi = 0;
        uint64_t targetFp = 0;
        for (auto j = 0; j < chunkSize; j++) {
            uint64_t Pi = (mSet[i] * fpList[j] + aSet[i]) & UINT64_MAX;
            if (maxPi < Pi) {
                maxPi = Pi;
                targetFp = fpList[j];
            }
        }
        featureList.push_back(targetFp);
        // cout << targetFp << " ";
    }
    // cout << endl;
    u_char featuresBuffer[keySeedSubFeatureNumber_ * sizeof(uint64_t)];
    memset(featuresBuffer, 0, keySeedSubFeatureNumber_ * sizeof(uint64_t));
    for (int i = 0; i < featureList.size(); i++) {
        memcpy(featuresBuffer + i * sizeof(uint64_t), &featureList[i], sizeof(uint64_t));
    }
    u_char superFeatureBuffer[SYSTEM_CIPHER_SIZE];
    cryptoObj_->generateHash(featuresBuffer, keySeedSubFeatureNumber_ * sizeof(uint64_t), superFeatureBuffer);
    string SFStr((char*)superFeatureBuffer, SYSTEM_CIPHER_SIZE);
    superFeatureList.push_back(SFStr);
    return true;
}

bool Fingerprinter::editJobDoneFlag()
{
    inputMQ_->done_ = true;
    if (inputMQ_->done_) {
        return true;
    } else {
        return false;
    }
}
