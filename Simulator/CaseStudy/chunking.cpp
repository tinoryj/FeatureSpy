#include "chunker.hpp"
#include "featureExtraction.hpp"
#include <bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/time.h>
using namespace std;
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

bool getPrefix(u_char* firstChunkBuffer, u_char* minChunkBuffer, u_char* allChunkBuffer, int chunkSize, ofstream& outputStream)
{
    int prefixLenSet[3] = { 1, 2, 4 };
    for (int i = 0; i < 3; i++) {
        int prefixLength = prefixLenSet[i];
        //generate prefix
        u_char hash[SHA256_DIGEST_LENGTH];
        u_char content[prefixLength * PREFIX_UNIT];
        //prefix of first hash
        memset(hash, 0, SHA256_DIGEST_LENGTH);
        memset(content, 0, prefixLength * PREFIX_UNIT);
        if (chunkSize < prefixLength * PREFIX_UNIT) {
            memcpy(content, firstChunkBuffer, chunkSize);
            SHA256(content, chunkSize, hash);
        } else {
            memcpy(content, firstChunkBuffer, prefixLength * PREFIX_UNIT);
            SHA256(content, prefixLength * PREFIX_UNIT, hash);
        }
        string hashStrFirst((char*)hash, SHA256_DIGEST_LENGTH);
        //prefix of min hash
        memset(hash, 0, SHA256_DIGEST_LENGTH);
        memset(content, 0, prefixLength * PREFIX_UNIT);
        if (chunkSize < prefixLength * PREFIX_UNIT) {
            memcpy(content, minChunkBuffer, chunkSize);
            SHA256(content, chunkSize, hash);
        } else {
            memcpy(content, minChunkBuffer, prefixLength * PREFIX_UNIT);
            SHA256(content, prefixLength * PREFIX_UNIT, hash);
        }
        string hashStrMin((char*)hash, SHA256_DIGEST_LENGTH);
        //prefix of all hash
        memset(hash, 0, SHA256_DIGEST_LENGTH);
        memset(content, 0, prefixLength * PREFIX_UNIT);
        if (chunkSize < prefixLength * PREFIX_UNIT) {
            memcpy(content, allChunkBuffer, chunkSize);
            SHA256(content, chunkSize, hash);
        } else {
            memcpy(content, allChunkBuffer, prefixLength * PREFIX_UNIT);
            SHA256(content, prefixLength * PREFIX_UNIT, hash);
        }
        string hashStrAll((char*)hash, SHA256_DIGEST_LENGTH);
        //output
        char chunkFirstFPrefixHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
        u_char firstPBuffer[SYSTEM_CIPHER_SIZE];
        memcpy(firstPBuffer, &hashStrFirst[0], SYSTEM_CIPHER_SIZE);
        for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
            sprintf(chunkFirstFPrefixHexBuffer + 2 * i, "%02X", firstPBuffer[i]);
        }
        outputStream << chunkFirstFPrefixHexBuffer << endl;
        u_char minPBuffer[SYSTEM_CIPHER_SIZE];
        memcpy(minPBuffer, &hashStrMin[0], SYSTEM_CIPHER_SIZE);
        char chunkMinFPrefixHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
        for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
            sprintf(chunkMinFPrefixHexBuffer + 2 * i, "%02X", minPBuffer[i]);
        }
        outputStream << chunkMinFPrefixHexBuffer << endl;
        u_char allPBuffer[SYSTEM_CIPHER_SIZE];
        memcpy(allPBuffer, &hashStrAll[0], SYSTEM_CIPHER_SIZE);
        char chunkAllFPrefixHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
        for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
            sprintf(chunkAllFPrefixHexBuffer + 2 * i, "%02X", allPBuffer[i]);
        }
        outputStream << chunkAllFPrefixHexBuffer << endl;
    }
    return true;
}

bool getFeaturePrefix(FeatureGen* featureGenObj, u_char* chunkBuffer, int chunkSize, ofstream& outputStream)
{
    // generate features
    vector<uint64_t> baselineFeatureList;
    featureGenObj->getFeatureList(chunkBuffer, chunkSize, baselineFeatureList);
    vector<string> baselineSFList, baselineSFListSorted;
    for (int i = 0; i < baselineFeatureList.size(); i += FEATURE_NUMBER_PER_SF) {
        u_char superFeature[SHA256_DIGEST_LENGTH];
        u_char featuresBuffer[FEATURE_NUMBER_PER_SF * sizeof(uint64_t)];
        for (int j = 0; j < FEATURE_NUMBER_PER_SF; j++) {
            memcpy(featuresBuffer + j * sizeof(uint64_t), &baselineFeatureList[i + j], sizeof(uint64_t));
        }
        SHA256(featuresBuffer, FEATURE_NUMBER_PER_SF * sizeof(uint64_t), superFeature);
        string SFStr((char*)superFeature, SHA256_DIGEST_LENGTH);
        baselineSFList.push_back(SFStr);
        baselineSFListSorted.push_back(SFStr);
    }
    sort(baselineSFListSorted.begin(), baselineSFListSorted.end());
    // generate baseline cipher
    u_char baselineAllFeatureBuffer[SF_NUMBER * SHA256_DIGEST_LENGTH], baselineAllFeatureHashBuffer[SHA256_DIGEST_LENGTH];
    for (int i = 0; i < SF_NUMBER; i++) {
        memcpy(baselineAllFeatureBuffer + i * SHA256_DIGEST_LENGTH, &baselineSFList[i][0], SHA256_DIGEST_LENGTH);
    }
    SHA256(baselineAllFeatureBuffer, SF_NUMBER * SHA256_DIGEST_LENGTH, baselineAllFeatureHashBuffer);
    u_char baselineCipherMin[chunkSize], baselineCipherFirst[chunkSize], baselineCipherAll[chunkSize];
    encryptWithKey(chunkBuffer, chunkSize, (u_char*)&baselineSFListSorted[0][0], baselineCipherMin);
    encryptWithKey(chunkBuffer, chunkSize, (u_char*)&baselineSFList[0][0], baselineCipherFirst);
    encryptWithKey(chunkBuffer, chunkSize, baselineAllFeatureHashBuffer, baselineCipherAll);

    //out features
    u_char firstFBuffer[SYSTEM_CIPHER_SIZE];
    memcpy(firstFBuffer, &baselineSFList[0][0], SYSTEM_CIPHER_SIZE);
    char chunkFirstFHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
    for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
        sprintf(chunkFirstFHexBuffer + 2 * i, "%02X", firstFBuffer[i]);
    }
    outputStream << chunkFirstFHexBuffer << endl;
    u_char minFBuffer[SYSTEM_CIPHER_SIZE];
    memcpy(minFBuffer, &baselineSFListSorted[0][0], SYSTEM_CIPHER_SIZE);
    char chunkMinFHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
    for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
        sprintf(chunkMinFHexBuffer + 2 * i, "%02X", minFBuffer[i]);
    }
    outputStream << chunkMinFHexBuffer << endl;
    u_char allFBuffer[SYSTEM_CIPHER_SIZE];
    memcpy(allFBuffer, baselineAllFeatureHashBuffer, SYSTEM_CIPHER_SIZE);
    char chunkAllFHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
    for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
        sprintf(chunkAllFHexBuffer + 2 * i, "%02X", allFBuffer[i]);
    }
    outputStream << chunkAllFHexBuffer << endl;

    getPrefix(baselineCipherFirst, baselineCipherMin, baselineCipherAll, chunkSize, outputStream);
    return true;
}

bool chunkingWithID(FeatureGen* featureGenObj, Chunker* chunkObj, string filePath, uint64_t& count, ofstream& outputStream)
{
    u_char *waitingForChunkingBuffer_, *chunkBuffer_;
    waitingForChunkingBuffer_ = (u_char*)malloc(sizeof(u_char) * chunkObj->ReadSize_);
    chunkBuffer_ = (u_char*)malloc(sizeof(u_char) * chunkObj->maxChunkSize_);
#if SYSTEM_BREAK_DOWN == 1
    double insertTime = 0;
    double readFileTime = 0;
    long diff;
    double second;
#endif
    uint16_t winFp = 0;
    uint64_t chunkBufferCnt = 0, chunkIDCnt = count;
    ifstream fin;
    fin.open(filePath, ios::in | ios::binary);
    if (fin.is_open() == false) {
        cerr << "Can not open target chunking file: " << filePath << endl;
        return false;
    }
    uint64_t fileSize = 0;
    /*start chunking*/
    while (true) {
        memset((char*)waitingForChunkingBuffer_, 0, sizeof(u_char) * chunkObj->ReadSize_);
        fin.read((char*)waitingForChunkingBuffer_, sizeof(u_char) * chunkObj->ReadSize_);
        int len = fin.gcount();
        fileSize += len;
        memset(chunkBuffer_, 0, sizeof(u_char) * chunkObj->maxChunkSize_);
        for (int i = 0; i < len; i++) {

            chunkBuffer_[chunkBufferCnt] = waitingForChunkingBuffer_[i];

            /*full fill sliding window*/
            if (chunkBufferCnt < chunkObj->slidingWinSize_) {
                winFp = winFp + (chunkBuffer_[chunkBufferCnt] * chunkObj->powerLUT_[chunkObj->slidingWinSize_ - chunkBufferCnt - 1]) & chunkObj->polyMOD_; //Refer to doc/Chunking.md hash function:RabinChunker
                chunkBufferCnt++;
                continue;
            }
            winFp &= (chunkObj->polyMOD_);

            /*slide window*/
            unsigned short int v = chunkBuffer_[chunkBufferCnt - chunkObj->slidingWinSize_]; //queue
            winFp = ((winFp + chunkObj->removeLUT_[v]) * chunkObj->polyBase_ + chunkBuffer_[chunkBufferCnt]) & chunkObj->polyMOD_; //remove queue front and add queue tail
            chunkBufferCnt++;

            /*chunk's size less than minChunkSize_*/
            if (chunkBufferCnt < chunkObj->minChunkSize_) {
                continue;
            }

            /*find chunk pattern*/
            if ((winFp & chunkObj->anchorMask_) == chunkObj->anchorValue_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
                char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                    sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                }
                outputStream << chunkIDCnt << endl
                             << chunkHashHexBuffer << endl
                             << chunkBufferCnt << endl;
                getFeaturePrefix(featureGenObj, chunkBuffer_, chunkBufferCnt, outputStream);

                chunkIDCnt++;
                chunkBufferCnt = 0;
                winFp = 0;
            }

            /*chunk's size exceed chunkObj->maxChunkSize_*/
            if (chunkBufferCnt >= chunkObj->maxChunkSize_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);

                char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                    sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                }
                outputStream << chunkIDCnt << endl
                             << chunkHashHexBuffer << endl
                             << chunkBufferCnt << endl;
                getFeaturePrefix(featureGenObj, chunkBuffer_, chunkBufferCnt, outputStream);

                chunkIDCnt++;
                chunkBufferCnt = 0;
                winFp = 0;
            }
        }
        if (fin.eof()) {
            break;
        }
    }

    /*add final chunk*/
    if (chunkBufferCnt != 0) {
        u_char chunkHash[SYSTEM_CIPHER_SIZE];
        SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
        string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
        char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
        for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
            sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
        }
        outputStream << chunkIDCnt << endl
                     << chunkHashHexBuffer << endl
                     << chunkBufferCnt << endl;
        getFeaturePrefix(featureGenObj, chunkBuffer_, chunkBufferCnt, outputStream);

        chunkIDCnt++;
        chunkBufferCnt = 0;
        winFp = 0;
    }
    fin.close();
    free(waitingForChunkingBuffer_);
    free(chunkBuffer_);
    count = chunkIDCnt;
    cerr << "Total chunk number = " << chunkIDCnt << endl;
    return true;
}

int main(int argv, char* argc[])
{
    struct timeval timestart;
    struct timeval timeend;
    vector<string> chunkingFileList;
    Chunker* chunkObj = new Chunker();
    FeatureGen* featureGenObj = new FeatureGen();
    string fileName(argc[1]);
    ifstream fileListStream;
    fileListStream.open(fileName, ios::in | ios::binary);
    string currentLineStr;
    while (getline(fileListStream, currentLineStr)) {
        string newPath = currentLineStr;
        chunkingFileList.push_back(newPath);
    }
    fileListStream.close();
    fileListStream.clear();
    gettimeofday(&timestart, NULL);
    ifstream readID;
    readID.open("id.log", ios::in);
    uint64_t currentChunkID = 0;
    if (readID.is_open()) {
        readID >> currentChunkID;
        cerr << "currentChunkID = " << currentChunkID << endl;
        readID.close();
    } else {
        currentChunkID = 0;
        cerr << "currentChunkID = " << currentChunkID << endl;
    }
    ofstream fileInfo, chunkInfo;
    fileInfo.open(fileName + ".fileInfo", ios::out);
    chunkInfo.open(fileName + ".chunkInfo", ios::out);
    for (auto file : chunkingFileList) {
        uint64_t tempChunkID = currentChunkID;
        if (chunkingWithID(featureGenObj, chunkObj, file, currentChunkID, chunkInfo) == false) {
            cerr << "Error load file : " << file << endl;
            continue;
        }
        uint64_t chunkNumber = currentChunkID - tempChunkID;
        fileInfo << file << endl
                 << chunkNumber << endl;
    }
    fileInfo.close();
    ofstream writeID;
    writeID.open("id.log", ios::out);
    writeID << currentChunkID;
    gettimeofday(&timeend, NULL);
    long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    double second = diff / 1000000.0;
    cerr << "System : total work time = " << second << " s" << endl;
    return 0;
}
