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
    bool chunking(string filePath);
    bool chunkingWithCount(string filePath);
    bool chunkingWithCountID(string filePath, uint64_t& ID);
    bool fixedSizeChunking(string filePath);
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

bool Chunker::chunking(string filePath)
{
    u_char *waitingForChunkingBuffer_, *chunkBuffer_;
    waitingForChunkingBuffer_ = (u_char*)malloc(sizeof(u_char) * ReadSize_);
    chunkBuffer_ = (u_char*)malloc(sizeof(u_char) * maxChunkSize_);
#if SYSTEM_BREAK_DOWN == 1
    double insertTime = 0;
    double readFileTime = 0;
    long diff;
    double second;
#endif
    uint16_t winFp = 0;
    uint64_t chunkBufferCnt = 0, chunkIDCnt = 0;
    ifstream fin;
    fin.open(filePath, ios::in | ios::binary);
    if (fin.is_open() == false) {
        cerr << "Can not open target chunking file: " << filePath << endl;
        return false;
    }
    uint64_t fileSize = 0;
    /*start chunking*/
    while (true) {
        memset((char*)waitingForChunkingBuffer_, 0, sizeof(u_char) * ReadSize_);
        fin.read((char*)waitingForChunkingBuffer_, sizeof(u_char) * ReadSize_);
        int len = fin.gcount();
        fileSize += len;
        memset(chunkBuffer_, 0, sizeof(u_char) * maxChunkSize_);
        for (int i = 0; i < len; i++) {

            chunkBuffer_[chunkBufferCnt] = waitingForChunkingBuffer_[i];

            /*full fill sliding window*/
            if (chunkBufferCnt < slidingWinSize_) {
                winFp = winFp + (chunkBuffer_[chunkBufferCnt] * powerLUT_[slidingWinSize_ - chunkBufferCnt - 1]) & polyMOD_; //Refer to doc/Chunking.md hash function:RabinChunker
                chunkBufferCnt++;
                continue;
            }
            winFp &= (polyMOD_);

            /*slide window*/
            unsigned short int v = chunkBuffer_[chunkBufferCnt - slidingWinSize_]; //queue
            winFp = ((winFp + removeLUT_[v]) * polyBase_ + chunkBuffer_[chunkBufferCnt]) & polyMOD_; //remove queue front and add queue tail
            chunkBufferCnt++;

            /*chunk's size less than minChunkSize_*/
            if (chunkBufferCnt < minChunkSize_) {
                continue;
            }

            /*find chunk pattern*/
            if ((winFp & anchorMask_) == anchorValue_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
                if (chunkHashList.find(chunkHashStr) == chunkHashList.end()) {
                    chunkHashList.insert(chunkHashStr);
                    char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                    for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                        sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                    }
                    cout << chunkIDCnt << endl
                         << chunkHashHexBuffer << endl
                         << chunkBufferCnt << endl;
                    ofstream chunkContentStream;
                    chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
                    chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
                    chunkContentStream.close();
                }
                chunkIDCnt++;
                chunkBufferCnt = 0;
                winFp = 0;
            }

            /*chunk's size exceed maxChunkSize_*/
            if (chunkBufferCnt >= maxChunkSize_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
                if (chunkHashList.find(chunkHashStr) == chunkHashList.end()) {
                    chunkHashList.insert(chunkHashStr);
                    char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                    for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                        sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                    }
                    cout << chunkIDCnt << endl
                         << chunkHashHexBuffer << endl
                         << chunkBufferCnt << endl;
                    ofstream chunkContentStream;
                    chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
                    chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
                    chunkContentStream.close();
                }
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
        if (chunkHashList.find(chunkHashStr) == chunkHashList.end()) {
            chunkHashList.insert(chunkHashStr);
            char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
            for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
            }
            cout << chunkIDCnt << endl
                 << chunkHashHexBuffer << endl
                 << chunkBufferCnt << endl;
            ofstream chunkContentStream;
            chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
            chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
            chunkContentStream.close();
        }
        chunkIDCnt++;
        chunkBufferCnt = 0;
        winFp = 0;
    }
    fin.close();
    free(waitingForChunkingBuffer_);
    free(chunkBuffer_);
    return true;
}

bool Chunker::chunkingWithCount(string filePath)
{
    u_char *waitingForChunkingBuffer_, *chunkBuffer_;
    waitingForChunkingBuffer_ = (u_char*)malloc(sizeof(u_char) * ReadSize_);
    chunkBuffer_ = (u_char*)malloc(sizeof(u_char) * maxChunkSize_);
#if SYSTEM_BREAK_DOWN == 1
    double insertTime = 0;
    double readFileTime = 0;
    long diff;
    double second;
#endif
    uint16_t winFp = 0;
    uint64_t chunkBufferCnt = 0, chunkIDCnt = 0;
    ifstream fin;
    fin.open(filePath, ios::in | ios::binary);
    if (fin.is_open() == false) {
        cerr << "Can not open target chunking file: " << filePath << endl;
        return false;
    }
    uint64_t fileSize = 0;
    /*start chunking*/
    while (true) {
        memset((char*)waitingForChunkingBuffer_, 0, sizeof(u_char) * ReadSize_);
        fin.read((char*)waitingForChunkingBuffer_, sizeof(u_char) * ReadSize_);
        int len = fin.gcount();
        fileSize += len;
        memset(chunkBuffer_, 0, sizeof(u_char) * maxChunkSize_);
        for (int i = 0; i < len; i++) {

            chunkBuffer_[chunkBufferCnt] = waitingForChunkingBuffer_[i];

            /*full fill sliding window*/
            if (chunkBufferCnt < slidingWinSize_) {
                winFp = winFp + (chunkBuffer_[chunkBufferCnt] * powerLUT_[slidingWinSize_ - chunkBufferCnt - 1]) & polyMOD_; //Refer to doc/Chunking.md hash function:RabinChunker
                chunkBufferCnt++;
                continue;
            }
            winFp &= (polyMOD_);

            /*slide window*/
            unsigned short int v = chunkBuffer_[chunkBufferCnt - slidingWinSize_]; //queue
            winFp = ((winFp + removeLUT_[v]) * polyBase_ + chunkBuffer_[chunkBufferCnt]) & polyMOD_; //remove queue front and add queue tail
            chunkBufferCnt++;

            /*chunk's size less than minChunkSize_*/
            if (chunkBufferCnt < minChunkSize_) {
                continue;
            }

            /*find chunk pattern*/
            if ((winFp & anchorMask_) == anchorValue_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
                char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                    sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                }
                cout << chunkIDCnt << endl
                     << chunkHashHexBuffer << endl
                     << chunkBufferCnt << endl;
                ofstream chunkContentStream;
                chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
                chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
                chunkContentStream.close();

                chunkIDCnt++;
                chunkBufferCnt = 0;
                winFp = 0;
            }

            /*chunk's size exceed maxChunkSize_*/
            if (chunkBufferCnt >= maxChunkSize_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);

                char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                    sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                }
                cout << chunkIDCnt << endl
                     << chunkHashHexBuffer << endl
                     << chunkBufferCnt << endl;
                ofstream chunkContentStream;
                chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
                chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
                chunkContentStream.close();

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
        cout << chunkIDCnt << endl
             << chunkHashHexBuffer << endl
             << chunkBufferCnt << endl;
        ofstream chunkContentStream;
        chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
        chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
        chunkContentStream.close();
        chunkIDCnt++;
        chunkBufferCnt = 0;
        winFp = 0;
    }
    cerr << "Total chunk number = " << chunkIDCnt << ", unique chunk number = " << chunkHashMap.size() << endl;
    fin.close();
    free(waitingForChunkingBuffer_);
    free(chunkBuffer_);
    return true;
}

bool Chunker::chunkingWithCountID(string filePath, uint64_t& count)
{
    u_char *waitingForChunkingBuffer_, *chunkBuffer_;
    waitingForChunkingBuffer_ = (u_char*)malloc(sizeof(u_char) * ReadSize_);
    chunkBuffer_ = (u_char*)malloc(sizeof(u_char) * maxChunkSize_);
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
        memset((char*)waitingForChunkingBuffer_, 0, sizeof(u_char) * ReadSize_);
        fin.read((char*)waitingForChunkingBuffer_, sizeof(u_char) * ReadSize_);
        int len = fin.gcount();
        fileSize += len;
        memset(chunkBuffer_, 0, sizeof(u_char) * maxChunkSize_);
        for (int i = 0; i < len; i++) {

            chunkBuffer_[chunkBufferCnt] = waitingForChunkingBuffer_[i];

            /*full fill sliding window*/
            if (chunkBufferCnt < slidingWinSize_) {
                winFp = winFp + (chunkBuffer_[chunkBufferCnt] * powerLUT_[slidingWinSize_ - chunkBufferCnt - 1]) & polyMOD_; //Refer to doc/Chunking.md hash function:RabinChunker
                chunkBufferCnt++;
                continue;
            }
            winFp &= (polyMOD_);

            /*slide window*/
            unsigned short int v = chunkBuffer_[chunkBufferCnt - slidingWinSize_]; //queue
            winFp = ((winFp + removeLUT_[v]) * polyBase_ + chunkBuffer_[chunkBufferCnt]) & polyMOD_; //remove queue front and add queue tail
            chunkBufferCnt++;

            /*chunk's size less than minChunkSize_*/
            if (chunkBufferCnt < minChunkSize_) {
                continue;
            }

            /*find chunk pattern*/
            if ((winFp & anchorMask_) == anchorValue_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
                char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                    sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                }
                cout << chunkIDCnt << endl
                     << chunkHashHexBuffer << endl
                     << chunkBufferCnt << endl;
                ofstream chunkContentStream;
                chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
                chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
                chunkContentStream.close();

                chunkIDCnt++;
                chunkBufferCnt = 0;
                winFp = 0;
            }

            /*chunk's size exceed maxChunkSize_*/
            if (chunkBufferCnt >= maxChunkSize_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);

                char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                    sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                }
                cout << chunkIDCnt << endl
                     << chunkHashHexBuffer << endl
                     << chunkBufferCnt << endl;
                ofstream chunkContentStream;
                chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
                chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
                chunkContentStream.close();

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
        cout << chunkIDCnt << endl
             << chunkHashHexBuffer << endl
             << chunkBufferCnt << endl;
        ofstream chunkContentStream;
        chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
        chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
        chunkContentStream.close();
        chunkIDCnt++;
        chunkBufferCnt = 0;
        winFp = 0;
    }
    cerr << "Total chunk number = " << chunkIDCnt << ", unique chunk number = " << chunkHashMap.size() << endl;
    fin.close();
    free(waitingForChunkingBuffer_);
    free(chunkBuffer_);
    count = chunkIDCnt;
    return true;
}

bool Chunker::fixedSizeChunking(string filePath)
{
    u_char *waitingForChunkingBuffer_, *chunkBuffer_;
    waitingForChunkingBuffer_ = (u_char*)malloc(sizeof(u_char) * ReadSize_);
    chunkBuffer_ = (u_char*)malloc(sizeof(u_char) * maxChunkSize_);
#if SYSTEM_BREAK_DOWN == 1
    double insertTime = 0;
    double readFileTime = 0;
    long diff;
    double second;
#endif
    uint16_t winFp = 0;
    uint64_t chunkBufferCnt = 0, chunkIDCnt = 0;
    ifstream fin;
    fin.open(filePath, ios::in | ios::binary);
    if (fin.is_open() == false) {
        cerr << "Can not open target chunking file: " << filePath << endl;
        return false;
    }
    uint64_t fileSize = 0;
    /*start chunking*/
    while (true) {
        memset((char*)waitingForChunkingBuffer_, 0, sizeof(u_char) * ReadSize_);
        fin.read((char*)waitingForChunkingBuffer_, sizeof(u_char) * ReadSize_);
        int len = fin.gcount();
        fileSize += len;
        memset(chunkBuffer_, 0, sizeof(u_char) * maxChunkSize_);
        for (int i = 0; i < len; i++) {

            chunkBuffer_[chunkBufferCnt] = waitingForChunkingBuffer_[i];

            /*full fill sliding window*/
            if (chunkBufferCnt < slidingWinSize_) {
                winFp = winFp + (chunkBuffer_[chunkBufferCnt] * powerLUT_[slidingWinSize_ - chunkBufferCnt - 1]) & polyMOD_; //Refer to doc/Chunking.md hash function:RabinChunker
                chunkBufferCnt++;
                continue;
            }
            winFp &= (polyMOD_);

            /*slide window*/
            unsigned short int v = chunkBuffer_[chunkBufferCnt - slidingWinSize_]; //queue
            winFp = ((winFp + removeLUT_[v]) * polyBase_ + chunkBuffer_[chunkBufferCnt]) & polyMOD_; //remove queue front and add queue tail
            chunkBufferCnt++;

            /*chunk's size less than minChunkSize_*/
            if (chunkBufferCnt < minChunkSize_) {
                continue;
            }

            /*find chunk pattern*/
            if ((winFp & anchorMask_) == anchorValue_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
                if (chunkHashList.find(chunkHashStr) == chunkHashList.end()) {
                    chunkHashList.insert(chunkHashStr);
                    char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                    for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                        sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                    }
                    cout << chunkIDCnt << endl
                         << chunkHashHexBuffer << endl
                         << chunkBufferCnt << endl;
                    ofstream chunkContentStream;
                    chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
                    chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
                    chunkContentStream.close();
                }
                chunkIDCnt++;
                chunkBufferCnt = 0;
                winFp = 0;
            }

            /*chunk's size exceed maxChunkSize_*/
            if (chunkBufferCnt >= maxChunkSize_) {
                u_char chunkHash[SYSTEM_CIPHER_SIZE];
                SHA256(chunkBuffer_, chunkBufferCnt, chunkHash);
                string chunkHashStr((char*)chunkHash, SYSTEM_CIPHER_SIZE);
                if (chunkHashList.find(chunkHashStr) == chunkHashList.end()) {
                    chunkHashList.insert(chunkHashStr);
                    char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
                    for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                        sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
                    }
                    cout << chunkIDCnt << endl
                         << chunkHashHexBuffer << endl
                         << chunkBufferCnt << endl;
                    ofstream chunkContentStream;
                    chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
                    chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
                    chunkContentStream.close();
                }
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
        if (chunkHashList.find(chunkHashStr) == chunkHashList.end()) {
            chunkHashList.insert(chunkHashStr);
            char chunkHashHexBuffer[SYSTEM_CIPHER_SIZE * 2 + 1];
            for (int i = 0; i < SYSTEM_CIPHER_SIZE; i++) {
                sprintf(chunkHashHexBuffer + 2 * i, "%02X", chunkHash[i]);
            }
            cout << chunkIDCnt << endl
                 << chunkHashHexBuffer << endl
                 << chunkBufferCnt << endl;
            ofstream chunkContentStream;
            chunkContentStream.open("./Content/" + to_string(chunkIDCnt) + ".chunk", ios::binary | ios::out);
            chunkContentStream.write((char*)chunkBuffer_, chunkBufferCnt);
            chunkContentStream.close();
        }
        chunkIDCnt++;
        chunkBufferCnt = 0;
        winFp = 0;
    }
    fin.close();
    free(waitingForChunkingBuffer_);
    free(chunkBuffer_);
    return true;
}
