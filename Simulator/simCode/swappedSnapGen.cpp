#include "featureExtraction.hpp"
#include <bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/time.h>
using namespace std;
#define PREFIX_UNIT 16 // block unit
#define SYSTEM_CIPHER_SIZE 32
#define setbit(x, y) x |= (1 << y)
#define clrbit(x, y) x &= !(1 << y)
int chunkID = 0;
int swapAbleChunkNumber = 0;
u_char targetByteArray[256];

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
    // generate prefix
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
        // output
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
        // out features
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

bool chunking(u_char* tempChunk, int chunkSize, FeatureGen* featureGenObj, ofstream& outputStream)
{
    u_char chunkHash[SYSTEM_CIPHER_SIZE];
    SHA256(tempChunk, chunkSize, chunkHash);
    char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
    for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
        sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
    }
    outputStream << chunkID << endl
                 << chunkHashHexBuffer << endl
                 << chunkSize << endl;
    chunkID++;
    getFeaturePrefix(featureGenObj, tempChunk, chunkSize, outputStream);
    return true;
}

int main(int argv, char* argc[])
{
    // snapshot path, random seed, swap ratio, chunk size, modify pos, modify length
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
    struct timeval timestart;
    struct timeval timeend;
    vector<int> targetSwapChunkList;
    ifstream targetSwapChunkStream;
    string fileName(argc[1]);
    targetSwapChunkStream.open(fileName + ".targetSwapList", ios::in | ios::binary);
    string currentLineStr;
    int totalChunkNumber = 0;
    while (getline(targetSwapChunkStream, currentLineStr)) {
        int chunkID = stoi(currentLineStr);
        targetSwapChunkList.push_back(chunkID);
        totalChunkNumber++;
    }
    targetSwapChunkStream.close();
    targetSwapChunkStream.clear();
    string randomSeed(argc[6]);
    srand(stoi(randomSeed));
    // random target chunk
    random_shuffle(targetSwapChunkList.begin(), targetSwapChunkList.end());
    // cerr << "Target total chunk number = " << totalChunkNumber << endl;
    string swapRatioStr(argc[5]);
    double swapRatio = stod(swapRatioStr);
    int totalSwapChunkNumber = ceil(totalChunkNumber * swapRatio);
    set<int> targetSwapChunkIDSet;
    for (int i = 0; i < totalSwapChunkNumber; i++) {
        targetSwapChunkIDSet.insert(targetSwapChunkList[i]);
        // cerr << targetSwapChunkList[i] << "\t";
    }
    cerr << endl;
    cerr << "Target swap chunk number = " << targetSwapChunkIDSet.size() << endl;
    // gen base chunk
    string targetChunkSizeStr(argc[2]);
    int chunkSize = stoi(targetChunkSizeStr);
    u_char baseChunkBuffer[chunkSize];
    memset(baseChunkBuffer, 0, 1);
    for (int i = 0; i < chunkSize; i++) {
        baseChunkBuffer[i] = targetByteArray[rand() % 256];
    }
    string modifyPosStr(argc[3]);
    string modifyLengthStr(argc[4]);
    int modifyPos = stoi(modifyPosStr);
    int modifyLength = stoi(modifyLengthStr);
    // processing
    FeatureGen* featureGenObj = new FeatureGen();
    gettimeofday(&timestart, NULL);
    ifstream originChunkInfo;
    originChunkInfo.open(fileName + ".chunkInfo", ios::in);
    if (!originChunkInfo.is_open()) {
        cerr << "Error load original chunk info list" << endl;
        return 0;
    }
    int currentChunkID = 0;
    ofstream chunkInfo;
    chunkInfo.open(fileName + ".swap.chunkInfo", ios::out);
    while (getline(originChunkInfo, currentLineStr)) {
        if (targetSwapChunkIDSet.find(currentChunkID) != targetSwapChunkIDSet.end()) {
            // cerr << "Swap Chunk ID = " << currentChunkID << endl;
            u_char tempChunk[chunkSize];
            // select modify pos
            vector<int> modifyPosVec;
            bool modifiedFlag[chunkSize];
            std::fill_n(modifiedFlag, chunkSize, false);
            while (modifyPosVec.size() != modifyPos) {
                int currentModifyPos = 0;
                while (true) {
                    int currentPos = rand() % chunkSize;
                    bool modifyCheckFlag = modifiedFlag[currentPos];
                    for (int i = currentPos; i < currentPos + modifyLength; i++) {
                        modifyCheckFlag = (modifyCheckFlag || modifiedFlag[i]);
                    }
                    if (modifyCheckFlag == false && (currentPos + modifyLength) < chunkSize) {
                        modifyPosVec.push_back(currentPos);
                        break;
                    }
                }
            }
            generateFakeFile(baseChunkBuffer, chunkSize, modifyPosVec, modifyLength, tempChunk);
            chunking(tempChunk, chunkSize, featureGenObj, chunkInfo);
            for (int index = 0; index < 2 + featureNumber * 2; index++) {
                getline(originChunkInfo, currentLineStr);
            }
        } else {
            chunkInfo << currentLineStr << endl;
            for (int index = 0; index < 2 + featureNumber * 2; index++) {
                getline(originChunkInfo, currentLineStr);
                chunkInfo << currentLineStr << endl;
            }
        }
        currentChunkID++;
    }
    chunkInfo.close();
    originChunkInfo.close();
    gettimeofday(&timeend, NULL);
    long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    double second = diff / 1000000.0;
    cerr << "System : total work time = " << second << " s" << endl;
    return 0;
}
