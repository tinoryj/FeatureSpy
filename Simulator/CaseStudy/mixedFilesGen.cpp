#include "chunker.hpp"
#include "featureExtraction.hpp"
#include <bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/time.h>
using namespace std;
#define setbit(x, y) x |= (1 << y) //将X的第Y位置1
#define clrbit(x, y) x &= !(1 << y) //将X的第Y位清0
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
    string snapPathName(argc[1]);
    ifstream snapFileListStream, snapChunkListStream;
    snapFileListStream.open(snapPathName + ".fileInfo", ios::in | ios::binary);
    if (!snapFileListStream.is_open()) {
        cerr << "Error open file list" << endl;
        return 0;
    }
    snapChunkListStream.open(snapPathName + ".chunkInfo", ios::in | ios::binary);
    if (!snapChunkListStream.is_open()) {
        cerr << "Error open chunk list" << endl;
        return 0;
    }
    string fakePathName(argc[2]);
    ifstream fakeFileListStream, fakeChunkListStream;
    fakeFileListStream.open(fakePathName + ".fileInfo", ios::in | ios::binary);
    if (!fakeFileListStream.is_open()) {
        cerr << "Error open file list" << endl;
        return 0;
    }
    fakeChunkListStream.open(fakePathName + ".chunkInfo", ios::in | ios::binary);
    if (!fakeChunkListStream.is_open()) {
        cerr << "Error open chunk list" << endl;
        return 0;
    }
    vector<uint64_t> snapFileChunkNumberVec, fakeFileChunkNumberVec;
    string currentLineStr;
    while (getline(snapFileListStream, currentLineStr)) {
        getline(snapFileListStream, currentLineStr);
        uint64_t chunkNumber = stoi(currentLineStr);
        snapFileChunkNumberVec.push_back(chunkNumber);
    }
    cerr << "Target snapshot file numebr = " << snapFileChunkNumberVec.size() << endl;
    snapFileListStream.close();
    while (getline(fakeFileListStream, currentLineStr)) {
        getline(fakeFileListStream, currentLineStr);
        uint64_t chunkNumber = stoi(currentLineStr);
        fakeFileChunkNumberVec.push_back(chunkNumber);
    }
    cerr << "Target fake file numebr = " << fakeFileChunkNumberVec.size() << endl;
    fakeFileListStream.close();

    // generate base file done
    int rawFileNumber = snapFileChunkNumberVec.size();
    int fakeFileNumber = fakeFileChunkNumberVec.size();
    int fakeFileNumberPerSlot = fakeFileNumber / (rawFileNumber + 1);

    ofstream chunkInfo, fileInfo;
    chunkInfo.open(snapPathName + ".mixed.chunkInfo", ios::out);
    fileInfo.open(snapPathName + ".mixed.fileInfo", ios::out);

    if (fakeFileNumberPerSlot == 0) {
        cerr << "Raw files' number > fake files' number" << endl;
        int rawFileNumberPerSlot = rawFileNumber / (fakeFileNumber + 1);
        // output
        int chunkIDCnt = 0;
        int generatedFileNumber = 0;
        int rawFileIndex = 0;
        for (int i = 0; i < fakeFileNumber; i++) {
            for (int j = 0; j < rawFileNumberPerSlot; j++) {
                for (int k = 0; k < snapFileChunkNumberVec[rawFileIndex]; k++) {
                    for (int index = 0; index < 15; index++) {
                        string tempLine;
                        getline(snapChunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                fileInfo << "Raw : " << snapFileChunkNumberVec[rawFileIndex] << endl;
                rawFileIndex++;
                generatedFileNumber++;
            }
            for (int k = 0; k < fakeFileChunkNumberVec[i]; k++) {
                for (int index = 0; index < 15; index++) {
                    string tempLine;
                    getline(fakeChunkListStream, tempLine);
                    chunkInfo << tempLine << endl;
                }
            }
            fileInfo << "Fake : " << fakeFileChunkNumberVec[i] << endl;
        }
        if (rawFileNumber > generatedFileNumber) {
            for (int i = 0; i < rawFileNumber - generatedFileNumber; i++) {
                for (int j = 0; j < snapFileChunkNumberVec[rawFileIndex]; j++) {
                    for (int index = 0; index < 15; index++) {
                        string tempLine;
                        getline(snapChunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                fileInfo << "Raw : " << snapFileChunkNumberVec[rawFileIndex] << endl;
                rawFileIndex++;
            }
        }
    } else {
        // output
        int chunkIDCnt = 0;
        int generatedFileNumber = 0;
        int fakeFileIndex = 0;
        for (int i = 0; i < rawFileNumber; i++) {
            for (int j = 0; j < fakeFileNumberPerSlot; j++) {
                for (int k = 0; k < fakeFileChunkNumberVec[fakeFileIndex]; k++) {
                    for (int index = 0; index < 15; index++) {
                        string tempLine;
                        getline(fakeChunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                fileInfo << "Fake : " << fakeFileChunkNumberVec[fakeFileIndex] << endl;
                fakeFileIndex++;
                generatedFileNumber++;
            }
            for (int j = 0; j < snapFileChunkNumberVec[i]; j++) {
                for (int index = 0; index < 15; index++) {
                    string tempLine;
                    getline(snapChunkListStream, tempLine);
                    chunkInfo << tempLine << endl;
                }
            }
            fileInfo << "Raw : " << snapFileChunkNumberVec[i] << endl;
        }
        if (fakeFileNumber > generatedFileNumber) {
            for (int i = 0; i < fakeFileNumber - generatedFileNumber; i++) {
                for (int k = 0; k < fakeFileChunkNumberVec[fakeFileIndex]; k++) {
                    for (int index = 0; index < 15; index++) {
                        string tempLine;
                        getline(fakeChunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                fileInfo << "Fake : " << fakeFileChunkNumberVec[fakeFileIndex] << endl;
                fakeFileIndex++;
            }
        }
    }
    chunkInfo.close();
    fileInfo.close();
    snapChunkListStream.close();
    fakeChunkListStream.close();
    gettimeofday(&timeend, NULL);
    long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    double second = diff / 1000000.0;
    cerr << "System : total work time = " << second << " s" << endl;
    return 0;
}
