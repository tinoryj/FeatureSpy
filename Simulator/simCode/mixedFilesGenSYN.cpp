#include "featureExtraction.hpp"
#include <bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/time.h>
using namespace std;
#define setbit(x, y) x |= (1 << y)
#define clrbit(x, y) x &= !(1 << y)
#define PREFIX_UNIT 16 // block unit

bool encryptWithKey(u_char* dataBuffer, const int dataSize, u_char* key, u_char* ciphertext)
{
    EVP_CIPHER_CTX* cipherctx_;
    u_char iv_[16];
    cipherctx_ = EVP_CIPHER_CTX_new();
    if (cipherctx_ == nullptr) {
        cerr << "can not initial cipher ctx" << endl;
    }
    EVP_CIPHER_CTX_init(cipherctx_);
    memset(iv_, 0, 16);

    int cipherlen, len;
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    if (EVP_EncryptInit_ex(cipherctx_, EVP_aes_256_cfb(), nullptr, key, iv_) != 1) {
        cerr << "encrypt error\n";
        EVP_CIPHER_CTX_reset(cipherctx_);
        EVP_CIPHER_CTX_cleanup(cipherctx_);
        return false;
    }
    if (EVP_EncryptUpdate(cipherctx_, ciphertext, &cipherlen, dataBuffer, dataSize) != 1) {
        cerr << "encrypt error\n";
        EVP_CIPHER_CTX_reset(cipherctx_);
        EVP_CIPHER_CTX_cleanup(cipherctx_);
        return false;
    }
    if (EVP_EncryptFinal_ex(cipherctx_, ciphertext + cipherlen, &len) != 1) {
        cerr << "encrypt error\n";
        EVP_CIPHER_CTX_reset(cipherctx_);
        EVP_CIPHER_CTX_cleanup(cipherctx_);
        return false;
    }
    cipherlen += len;
    if (cipherlen != dataSize) {
        cerr << "CryptoPrimitive : encrypt output size not equal to origin size" << endl;
        EVP_CIPHER_CTX_reset(cipherctx_);
        EVP_CIPHER_CTX_cleanup(cipherctx_);
        return false;
    }
    EVP_CIPHER_CTX_reset(cipherctx_);
    EVP_CIPHER_CTX_cleanup(cipherctx_);
    return true;
}

bool getPrefix(u_char** ChunkBufferSet, int chunkNumber, int chunkSize, ofstream& outputStream)
{

    int prefixLength = prefixBlockNumber;
    //generate prefix
    u_char hash[SHA256_DIGEST_LENGTH];
    u_char content[prefixLength * PREFIX_UNIT];
    memset(hash, 0, SHA256_DIGEST_LENGTH);
    memset(content, 0, prefixLength * PREFIX_UNIT);
    for (int j = 0; j < chunkNumber; j++) {
        if (chunkSize < prefixLength * PREFIX_UNIT) {
            memcpy(content, ChunkBufferSet[j], chunkSize);
            SHA256(content, chunkSize, hash);
        } else {
            memcpy(content, ChunkBufferSet[j], prefixLength * PREFIX_UNIT);
            SHA256(content, prefixLength * PREFIX_UNIT, hash);
        }
        //output
        char chunkFPrefixHexBuffer[SHA256_DIGEST_LENGTH * 2 + 1];
        for (int index = 0; index < SHA256_DIGEST_LENGTH; index++) {
            sprintf(chunkFPrefixHexBuffer + 2 * index, "%02X", hash[index]);
        }
        outputStream << chunkFPrefixHexBuffer << endl;
        memset(hash, 0, SHA256_DIGEST_LENGTH);
        memset(content, 0, prefixLength * PREFIX_UNIT);
    }
    return true;
}

bool getFeaturePrefix(FeatureGen* featureGenObj, u_char* chunkBuffer, int chunkSize, ofstream& outputStream)
{
    // generate features
    vector<uint64_t> baselineFeatureList;
    featureGenObj->getFeatureList(chunkBuffer, chunkSize, baselineFeatureList);
    vector<string> baselineSFList;
    for (int i = 0; i < baselineFeatureList.size(); i += FEATURE_NUMBER_PER_SF) {
        u_char superFeature[SHA256_DIGEST_LENGTH];
        u_char featuresBuffer[FEATURE_NUMBER_PER_SF * sizeof(uint64_t)];
        for (int j = 0; j < FEATURE_NUMBER_PER_SF; j++) {
            memcpy(featuresBuffer + j * sizeof(uint64_t), &baselineFeatureList[i + j], sizeof(uint64_t));
        }
        SHA256(featuresBuffer, FEATURE_NUMBER_PER_SF * sizeof(uint64_t), superFeature);
        string SFStr((char*)superFeature, SHA256_DIGEST_LENGTH);
        baselineSFList.push_back(SFStr);
    }
    u_char** baselineCipherAll;
    baselineCipherAll = (u_char**)malloc(featureNumber * sizeof(u_char*));
    for (int i = 0; i < featureNumber; i++) {
        baselineCipherAll[i] = (u_char*)malloc(chunkSize * sizeof(u_char));
    }
    for (int i = 1; i < featureNumber + 1; i++) {
        u_char baselineFeatureBuffer[i * SHA256_DIGEST_LENGTH], baselineFeatureHashBuffer[SHA256_DIGEST_LENGTH];
        for (int featureIndex = 0; featureIndex < i; featureIndex++) {
            memcpy(baselineFeatureBuffer + featureIndex * SHA256_DIGEST_LENGTH, &baselineSFList[featureIndex][0], SHA256_DIGEST_LENGTH);
        }
        SHA256(baselineFeatureBuffer, i * SHA256_DIGEST_LENGTH, baselineFeatureHashBuffer);
        encryptWithKey(chunkBuffer, chunkSize, baselineFeatureHashBuffer, baselineCipherAll[i - 1]);
        //out features
        char featureHexBuffer[SHA256_DIGEST_LENGTH * 2 + 1];
        for (int printIndex = 0; printIndex < SHA256_DIGEST_LENGTH; printIndex++) {
            sprintf(featureHexBuffer + 2 * printIndex, "%02X", baselineFeatureHashBuffer[printIndex]);
        }
        outputStream << featureHexBuffer << endl;
    }
    getPrefix(baselineCipherAll, featureNumber, chunkSize, outputStream);

    for (int i = 0; i < featureNumber; i++) {
        free(baselineCipherAll[i]);
    }
    free(baselineCipherAll);
    return true;
}

u_char targetByteArray[256];

bool generateFakeFile(u_char* baseChunkBuffer, int chunkSize, vector<int> modifyPos, int modifyBytesNumber, u_char* targetChunkBuffer)
{
    // start random
    memcpy(targetChunkBuffer, baseChunkBuffer, chunkSize);
    for (int currentModifyPos = 0; currentModifyPos < modifyPos.size(); currentModifyPos++) {
        int tempPos = modifyPos[currentModifyPos];
        for (int currentModifyBytesNumber = 0; currentModifyBytesNumber < modifyBytesNumber; currentModifyBytesNumber++) {
            u_char tempByte[1];
            memcpy(tempByte, targetByteArray + (rand() % 256), 1);
            memcpy(targetChunkBuffer + tempPos + currentModifyBytesNumber, tempByte, 1);
        }
    }
    return true;
}

int main(int argv, char* argc[])
{
    FeatureGen* featureGenObj = new FeatureGen();
    struct timeval timestart;
    struct timeval timeend;
    gettimeofday(&timestart, NULL);
    string fileListPathName(argc[1]);
    string chunkListPathName(argc[2]);
    ifstream fileListStream, chunkListStream;
    fileListStream.open(fileListPathName, ios::in | ios::binary);
    if (!fileListStream.is_open()) {
        cerr << "Error open file list" << endl;
        return 0;
    }
    chunkListStream.open(chunkListPathName, ios::in | ios::binary);
    if (!chunkListStream.is_open()) {
        cerr << "Error open chunk list" << endl;
        return 0;
    }
    vector<uint64_t> fileChunkNumberVec;
    string currentLineStr;
    while (getline(fileListStream, currentLineStr)) {
        getline(fileListStream, currentLineStr);
        uint64_t chunkNumber = stoi(currentLineStr);
        fileChunkNumberVec.push_back(chunkNumber);
    }
    cerr << "Target snapshot file numebr = " << fileChunkNumberVec.size() << endl;
    fileListStream.close();
    // generate fake file base
    u_char targetByte;
    for (int i = 0; i < 256; i++) {
        int currentNumber = i;
        memset(&targetByte, 0, sizeof(u_char));
        for (int j = 0; j < 8; j++) {
            int cnt = 2;
            if (currentNumber % cnt == 1) {
                setbit(targetByte, j);
            }
            currentNumber = currentNumber / 2;
            cnt = cnt * 2;
        }
        memcpy(targetByteArray + i, &targetByte, 1);
    }

    string baseFileSizeStr(argc[3]);
    string modifyPosStr(argc[4]);
    string modifyLengthStr(argc[5]);
    string modifyTimesStr(argc[6]);
    string randomeSeedStr(argc[7]);
    int baseFileSize = stoi(baseFileSizeStr);
    int modifyPos = stoi(modifyPosStr);
    int modifyLength = stoi(modifyLengthStr);
    int modifyTimes = stoi(modifyTimesStr);
    int randomSeed = stoi(randomeSeedStr);
    srand(randomSeed);
    // select modifypos
    vector<int> modifyPosVec;
    bool modifiedFlag[baseFileSize];
    std::fill_n(modifiedFlag, baseFileSize, false);
    while (modifyPosVec.size() != modifyPos) {
        int currentModifyPos = 0;
        while (true) {
            int currentPos = rand() % baseFileSize;
            bool modifyCheckFlag = modifiedFlag[currentPos];
            for (int i = currentPos; i < currentPos + modifyLength; i++) {
                modifyCheckFlag = (modifyCheckFlag || modifiedFlag[i]);
            }
            if (modifyCheckFlag == false && (currentPos + modifyLength) < baseFileSize) {
                modifyPosVec.push_back(currentPos);
                break;
            }
        }
    }
    srand(1);
    u_char baseFileBuffer[baseFileSize];
    memset(baseFileBuffer, 0, 1);
    for (int i = 0; i < baseFileSize; i++) {
        baseFileBuffer[i] = targetByteArray[rand() % 256];
    }
    // generate base file done
    int rawFileNumber = fileChunkNumberVec.size();
    int fakeFileNumber = modifyTimes;
    int fakeFileNumberPerSlot = fakeFileNumber / (rawFileNumber + 1);

    srand(randomSeed);
    ofstream chunkInfo;
    chunkInfo.open(baseFileSizeStr + "-" + modifyPosStr + "-" + modifyLengthStr + "-" + to_string(fakeFileNumber) + "-" + randomeSeedStr + ".chunkInfo", ios::out);

    if (fakeFileNumberPerSlot == 0) {
        cerr << "Raw files' number > fake files' number" << endl;
        int rawFileNumberPerSlot = rawFileNumber / (fakeFileNumber + 1);
        // output
        int chunkIDCnt = 0;
        int generatedFileNumber = 0;
        int rawFileIndex = 0;
        for (int i = 0; i < fakeFileNumber; i++) {
            for (int j = 0; j < rawFileNumberPerSlot; j++) {
                for (int j = 0; j < fileChunkNumberVec[rawFileIndex]; j++) {
                    for (int index = 0; index < 3 + featureNumber * 2; index++) {
                        string tempLine;
                        getline(chunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                rawFileIndex++;
                generatedFileNumber++;
            }
            u_char currentFakeFile[baseFileSize];
            generateFakeFile(baseFileBuffer, baseFileSize, modifyPosVec, modifyLength, currentFakeFile);
            u_char chunkHash[SYSTEM_CIPHER_SIZE];
            SHA256(currentFakeFile, baseFileSize, chunkHash);
            string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
            char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
            for (int tempIndex = 0; tempIndex < SYSTEM_CIPHER_SIZE; tempIndex++) {
                sprintf(chunkHashHexBuffer + 2 * tempIndex, "%02X", chunkHash[tempIndex]);
            }
            chunkInfo << chunkIDCnt << endl
                      << chunkHashHexBuffer << endl
                      << baseFileSize << endl;
            getFeaturePrefix(featureGenObj, currentFakeFile, baseFileSize, chunkInfo);
        }
        if (rawFileNumber > generatedFileNumber) {
            for (int i = 0; i < rawFileNumber - generatedFileNumber; i++) {
                for (int j = 0; j < fileChunkNumberVec[rawFileIndex]; j++) {
                    for (int index = 0; index < 3 + featureNumber * 2; index++) {
                        string tempLine;
                        getline(chunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                rawFileIndex++;
            }
        }
    } else {
        // output
        int chunkIDCnt = 0;
        int generatedFileNumber = 0;
        for (int i = 0; i < rawFileNumber; i++) {
            for (int j = 0; j < fakeFileNumberPerSlot; j++) {
                u_char currentFakeFile[baseFileSize];
                generateFakeFile(baseFileBuffer, baseFileSize, modifyPosVec, modifyLength, currentFakeFile);
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(currentFakeFile, baseFileSize, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
                char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                for (int tempIndex = 0; tempIndex < SYSTEM_CIPHER_SIZE; tempIndex++) {
                    sprintf(chunkHashHexBuffer + 2 * tempIndex, "%02X", chunkHash[tempIndex]);
                }
                chunkInfo << chunkIDCnt << endl
                          << chunkHashHexBuffer << endl
                          << baseFileSize << endl;
                getFeaturePrefix(featureGenObj, currentFakeFile, baseFileSize, chunkInfo);
                generatedFileNumber++;
            }
            for (int j = 0; j < fileChunkNumberVec[i]; j++) {
                for (int index = 0; index < 3 + featureNumber * 2; index++) {
                    string tempLine;
                    getline(chunkListStream, tempLine);
                    chunkInfo << tempLine << endl;
                }
            }
        }
        if (fakeFileNumber > generatedFileNumber) {
            for (int i = 0; i < fakeFileNumber - generatedFileNumber; i++) {
                u_char currentFakeFile[baseFileSize];
                generateFakeFile(baseFileBuffer, baseFileSize, modifyPosVec, modifyLength, currentFakeFile);
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(currentFakeFile, baseFileSize, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
                char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                for (int tempIndex = 0; tempIndex < SYSTEM_CIPHER_SIZE; tempIndex++) {
                    sprintf(chunkHashHexBuffer + 2 * tempIndex, "%02X", chunkHash[tempIndex]);
                }
                chunkInfo << chunkIDCnt << endl
                          << chunkHashHexBuffer << endl
                          << baseFileSize << endl;
                getFeaturePrefix(featureGenObj, currentFakeFile, baseFileSize, chunkInfo);
            }
        }
    }
    chunkInfo.close();
    chunkListStream.close();
    gettimeofday(&timeend, NULL);
    long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    double second = diff / 1000000.0;
    cerr << "System : total work time = " << second << " s" << endl;
    return 0;
}
