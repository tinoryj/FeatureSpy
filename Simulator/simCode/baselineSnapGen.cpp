#include "featureExtraction.hpp"
#include <bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/time.h>
using namespace std;
#define PREFIX_UNIT 16 // block unit
#define SHA256_DIGEST_LENGTH 32
#define setbit(x, y) x |= (1 << y)
#define clrbit(x, y) x &= !(1 << y)
int chunkID = 0;
int swapAbleChunkNumber = 0;
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

bool chunking(u_char* fileContent, int fileSize, int targetChunkSize, FeatureGen* featureGenObj, ofstream& outputStream, ofstream& swapStream)
{
    int chunkNumber = ceil((double)fileSize / (double)targetChunkSize);
    for (int i = 0; i < chunkNumber; i++) {
        u_char tempChunk[targetChunkSize];
        int chunkSize = 0;
        if (i != chunkNumber - 1) {
            swapAbleChunkNumber++;
            chunkSize = targetChunkSize;
            memcpy(tempChunk, fileContent + i * targetChunkSize, targetChunkSize);
            swapStream << chunkID << endl;
        } else {
            chunkSize = fileSize - i * targetChunkSize;
            memcpy(tempChunk, fileContent + i * targetChunkSize, chunkSize);
        }
        u_char chunkHash[SHA256_DIGEST_LENGTH];
        SHA256(tempChunk, chunkSize, chunkHash);
        char chunkHashHexBuffer[SHA256_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
        }
        outputStream << chunkID << endl
                     << chunkHashHexBuffer << endl
                     << chunkSize << endl;
        chunkID++;
        getFeaturePrefix(featureGenObj, tempChunk, chunkSize, outputStream);
    }
    return true;
}

int main(int argv, char* argc[])
{
    u_char targetByteArray[256], targetByte;
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
    srand(1);
    struct timeval timestart;
    struct timeval timeend;
    vector<int> targetFileSizeList;
    ifstream fileListStream;
    string fileName(argc[1]);
    fileListStream.open(fileName, ios::in | ios::binary);
    string currentLineStr;
    uint64_t totalFileSize = 0;
    while (getline(fileListStream, currentLineStr)) {
        int fileNumber = stoi(currentLineStr);
        getline(fileListStream, currentLineStr);
        int fileSize = stoi(currentLineStr);
        for (int i = 0; i < fileNumber; i++) {
            int targetFileSize = fileSize * 1024;
            targetFileSizeList.push_back(targetFileSize);
            totalFileSize += targetFileSize;
        }
    }
    fileListStream.close();
    fileListStream.clear();
    cerr << "Target total file number = " << targetFileSizeList.size() << " , snapshot size = " << (double)totalFileSize / 1024 / 1024 << " MiB" << endl;
    // random file seq
    random_shuffle(targetFileSizeList.begin(), targetFileSizeList.end());
    FeatureGen* featureGenObj = new FeatureGen();
    gettimeofday(&timestart, NULL);
    ofstream fileInfo, chunkInfo, swapInfo;
    fileInfo.open(fileName + ".fileInfo", ios::out);
    chunkInfo.open(fileName + ".chunkInfo", ios::out);
    swapInfo.open(fileName + ".targetSwapList", ios::out);
    int fileIndex = 0;
    string chunkSizeStr(argc[2]);
    int chunkSize = stoi(chunkSizeStr);
    for (auto currentFileSize : targetFileSizeList) {
        // cerr << currentFileSize << endl;
        u_char fileBuffer[currentFileSize];
        memset(fileBuffer, 0, 1);
        for (int i = 0; i < currentFileSize; i++) {
            fileBuffer[i] = targetByteArray[rand() % 256];
        }
        uint64_t tempChunkID = chunkID;
        chunking(fileBuffer, currentFileSize, chunkSize, featureGenObj, chunkInfo, swapInfo);
        uint64_t chunkNumber = chunkID - tempChunkID;
        fileInfo << to_string(fileIndex) + ".file" << endl
                 << chunkNumber << endl;
        fileIndex++;
    }
    fileInfo.close();
    chunkInfo.close();
    swapInfo.close();
    gettimeofday(&timeend, NULL);
    long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    double second = diff / 1000000.0;
    cerr << "System : total work time = " << second << " s" << endl;
    return 0;
}
