#include "keyClient.hpp"
#include "openssl/rsa.h"
#include <sys/time.h>

extern Configure config;

struct timeval timestartKey;
struct timeval timeendKey;

void PRINT_BYTE_ARRAY_KEY_CLIENT(
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

KeyClient::KeyClient(Encoder* encoderObjTemp, u_char* keyExchangeKey)
{
    inputMQ_ = new messageQueue<Data_t>;
    encoderObj_ = encoderObjTemp;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    memcpy(keyExchangeKey_, keyExchangeKey, SYSTEM_CIPHER_SIZE);
    keySecurityChannel_ = new ssl(config.getKeyServerIP(), config.getKeyServerPort(), CLIENTSIDE);
    sslConnection_ = keySecurityChannel_->sslConnect().second;
    clientID_ = config.getClientID();
}

KeyClient::~KeyClient()
{
    delete cryptoObj_;
    inputMQ_->~messageQueue();
    delete inputMQ_;
}

#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR

bool KeyClient::initClientCTRInfo()
{
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey, NULL);
#endif // SYSTEM_BREAK_DOWN
    //read old counter
    string keyGenFileName = ".keyGenStore";
    ifstream keyGenStoreIn;
    keyGenStoreIn.open(keyGenFileName, std::ifstream::in | std::ifstream::binary);
    if (keyGenStoreIn.is_open()) {
        keyGenStoreIn.seekg(0, ios_base::end);
        int counterFileSize = keyGenStoreIn.tellg();
        keyGenStoreIn.seekg(0, ios_base::beg);
        if (counterFileSize != 16) {
            cerr << "KeyClient : stored old counter file size error" << endl;
            return false;
        } else {
            char readBuffer[16];
            keyGenStoreIn.read(readBuffer, 16);
            keyGenStoreIn.close();
            if (keyGenStoreIn.gcount() != 16) {
                cerr << "KeyClient : read old counter file size error" << endl;
            } else {
                memcpy(nonce_, readBuffer, 12);
                memcpy(&counter_, readBuffer + 12, sizeof(uint32_t));
#if SYSTEM_DEBUG_FLAG == 1
                cerr << "KeyClient : Read old counter file : " << keyGenFileName << " success, the original counter = " << counter_ << ", nonce = " << endl;
                PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, nonce_, 12);
#endif
            }
        }
    } else {
    nonceUsedRetry:
#if MULTI_CLIENT_UPLOAD_TEST == 1
        memset(nonce_, clientID_, 12);
#else
        srand(time(NULL));
        for (int i = 0; i < 12 / sizeof(int); i++) {
            int randomNumber = rand();
            memcpy(nonce_ + i * sizeof(int), &randomNumber, sizeof(int));
        }
#endif
#if SYSTEM_DEBUG_FLAG == 1
        cerr << "KeyClient : Can not open old counter file : \"" << keyGenFileName << "\", Directly reset counter to 0, generate nonce = " << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, nonce_, 12);
#endif
    }
    // done
    NetworkHeadStruct_t initHead, responseHead;
    initHead.clientID = clientID_;
    initHead.dataSize = 48;
    initHead.messageType = KEY_GEN_UPLOAD_CLIENT_INFO;
    char initInfoBuffer[sizeof(NetworkHeadStruct_t) + initHead.dataSize]; // clientID & nonce & counter
    char responseBuffer[sizeof(NetworkHeadStruct_t)];
    memcpy(initInfoBuffer, &initHead, sizeof(NetworkHeadStruct_t));
    u_char tempCipherBuffer[16], tempPlaintBuffer[16];
    memcpy(tempPlaintBuffer, &counter_, sizeof(uint32_t));
    memcpy(tempPlaintBuffer + sizeof(uint32_t), nonce_, 16 - sizeof(uint32_t));
    cryptoObj_->keyExchangeEncrypt(tempPlaintBuffer, 16, keyExchangeKey_, keyExchangeKey_, tempCipherBuffer);
    memcpy(initInfoBuffer + sizeof(NetworkHeadStruct_t), tempCipherBuffer, 16);
    cryptoObj_->sha256Hmac(tempCipherBuffer, 16, (u_char*)initInfoBuffer + sizeof(NetworkHeadStruct_t) + 16, keyExchangeKey_, 32);
    if (!keySecurityChannel_->send(sslConnection_, initInfoBuffer, sizeof(NetworkHeadStruct_t) + initHead.dataSize)) {
        cerr << "KeyClient: send init information error" << endl;
        return false;
    } else {
        int recvSize;
        if (!keySecurityChannel_->recv(sslConnection_, responseBuffer, recvSize)) {
            cerr << "KeyClient: recv init information status error" << endl;
            return false;
        } else {
            memcpy(&responseHead, responseBuffer, sizeof(NetworkHeadStruct_t));
#if SYSTEM_DEBUG_FLAG == 1
            cerr << "KeyClient : recv key server response, message type = " << responseHead.messageType << endl;
            PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, responseBuffer, sizeof(NetworkHeadStruct_t));
#endif
            if (responseHead.messageType == CLIENT_COUNTER_REST) {
                cerr << "KeyClient : key server counter error, reset client counter to 0" << endl;
                counter_ = 0;
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendKey, NULL);
                int diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
                double second = diff / 1000000.0;
                cout << "KeyClient : init ctr mode key exchange time = " << setprecision(8) << second << " s" << endl;
#endif // SYSTEM_BREAK_DOWN
                return true;
            } else if (responseHead.messageType == NONCE_HAS_USED) {
                cerr << "KeyClient: nonce has used, goto retry" << endl;
                goto nonceUsedRetry;
            } else if (responseHead.messageType == ERROR_RESEND) {
                cerr << "KeyClient: hmac error, goto retry" << endl;
                goto nonceUsedRetry;
            } else if (responseHead.messageType == SUCCESS) {
#if SYSTEM_DEBUG_FLAG == 1
                cerr << "KeyClient : init information success, start key generate" << endl;
#endif

#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendKey, NULL);
                int diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
                double second = diff / 1000000.0;
                cout << "KeyClient : init ctr mode key exchange time = " << setprecision(8) << second << " s" << endl;
#endif // SYSTEM_BREAK_DOWN
                return true;
            }
        }
    }
    return true;
}

bool KeyClient::saveClientCTRInfo()
{
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey, NULL);
#endif // SYSTEM_BREAK_DOWN
    string keyGenFileName = ".keyGenStore";
    ofstream counterOut;
    counterOut.open(keyGenFileName, std::ofstream::out | std::ofstream::binary);
    if (!counterOut.is_open()) {
        cerr << "KeyClient : Can not open counter store file : " << keyGenFileName << endl;
        return false;
    } else {
        char writeBuffer[16];
        memcpy(writeBuffer, nonce_, 12);
        memcpy(writeBuffer + 12, &counter_, sizeof(uint32_t));
        counterOut.write(writeBuffer, 16);
        counterOut.close();
#if SYSTEM_DEBUG_FLAG == 1
        cerr << "KeyClient : Stored current counter file : " << keyGenFileName << endl;
#endif

#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendKey, NULL);
        int diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
        double second = diff / 1000000.0;
        cout << "KeyClient : save ctr mode status time = " << setprecision(8) << second << " s" << endl;
#endif // SYSTEM_BREAK_DOWN
        return true;
    }
}

#endif

void KeyClient ::run()
{

#if SYSTEM_BREAK_DOWN == 1
    double keyGenTime = 0;
    double chunkHashTime = 0;
    long diff;
    double second;
#endif // SYSTEM_BREAK_DOWN
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
    bool initStatus = initClientCTRInfo();
    if (initStatus != true) {
        cerr << "KeyClient : init to key server error, client exit" << endl;
        exit(0);
    }
#if SYSTEM_DEBUG_FLAG == 1
    else {
        cerr << "KeyClient : init to key server success" << endl;
    }
#endif
#endif
    vector<Data_t> batchList;
    unordered_map<uint32_t, int> tempIDMap;
    int batchNumber = 0;
    u_char chunkKeyRespondBuffer[SYSTEM_CIPHER_SIZE * keyBatchSize_];
    u_char chunkKeySeedBuffer[SYSTEM_CIPHER_SIZE * keyBatchSize_];
    bool JobDoneFlag = false;
    NetworkHeadStruct_t dataHead;
    dataHead.clientID = clientID_;
    dataHead.messageType = KEY_GEN_UPLOAD_CHUNK_HASH;
    // uint32_t currentChunkID = 0;
    while (true) {
        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            JobDoneFlag = true;
        }
        if (extractMQ(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                encoderObj_->insertMQ(tempChunk);
                continue;
            }
            batchList.push_back(tempChunk);
            tempIDMap.insert(make_pair(tempChunk.chunk.ID, batchNumber / 2));
            // feature (key) + hash(hash) key
            memcpy(chunkKeySeedBuffer + batchNumber * SYSTEM_CIPHER_SIZE, tempChunk.chunk.chunkHash, SYSTEM_CIPHER_SIZE);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartKey, NULL);
#endif
            cryptoObj_->generateHash(tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize, chunkKeySeedBuffer + batchNumber * SYSTEM_CIPHER_SIZE + SYSTEM_CIPHER_SIZE);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendKey, NULL);
            diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
            second = diff / 1000000.0;
            chunkHashTime += second;
#endif
            batchNumber += 2;
        }
        if (batchNumber == keyBatchSize_ || JobDoneFlag) {
            // cerr << "KeyClient : generate key for " << batchNumber << " chunks" << endl;
            if (batchNumber == 0) {
                bool editJobDoneFlagStatus = encoderObj_->editJobDoneFlag();
                if (!editJobDoneFlagStatus) {
                    cerr << "KeyClient : error to set job done flag for encoder" << endl;
                }
                break;
            }
            int batchedKeySize = 0;
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartKey, NULL);
#endif
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
            dataHead.dataSize = batchNumber * SYSTEM_CIPHER_SIZE;
            bool keyExchangeStatus = keyExchange(chunkKeySeedBuffer, batchNumber, chunkKeyRespondBuffer, batchedKeySize, dataHead);
            counter_ += batchNumber * 4;
#else
            bool keyExchangeStatus = keyExchange(chunkKeySeedBuffer, batchNumber, chunkKeyRespondBuffer, batchedKeySize);
#endif

#if SYSTEM_DEBUG_FLAG == 1
            cerr << "KeyClient : chunk key buffer = " << endl;
            PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, chunkKeyRespondBuffer, batchNumber * SYSTEM_CIPHER_SIZE);
#endif

#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendKey, NULL);
            diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
            second = diff / 1000000.0;
            keyGenTime += second;
#endif
            if (!keyExchangeStatus) {
                cerr << "KeyClient : error get key for " << fixed << batchNumber << " chunks" << endl;
                return;
            } else {
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartKey, NULL);
#endif
                sort(batchList.begin(), batchList.end(), [=](Data_t& a, Data_t& b) { return a.chunk.ID < b.chunk.ID; });
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendKey, NULL);
                diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
                second = diff / 1000000.0;
                keyGenTime += second;
#endif
                // cout << "Batch size = " << batchList.size();
                // for (int i = 0; i < batchList.size(); i++) {
                //     cout << "Current chunk ID  = " << batchList[i].chunk.ID << endl;
                // }
                for (int i = 0; i < batchList.size(); i++) {
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timestartKey, NULL);
#endif
                    int chunkKeyOffset = tempIDMap.at(batchList[i].chunk.ID) * SYSTEM_CIPHER_SIZE * 2;
                    // u_char prefixHash[SYSTEM_CIPHER_SIZE], prefixKeySeedBuffer[SYSTEM_CIPHER_SIZE * 2];
                    // if (batchList[i].chunk.logicDataSize <= INDICATOR_LENGTH) {
                    //     cryptoObj_->generateHash(batchList[i].chunk.logicData, batchList[i].chunk.logicDataSize, prefixHash);
                    // } else {
                    //     cryptoObj_->generateHash(batchList[i].chunk.logicData, INDICATOR_LENGTH, prefixHash);
                    // }
                    // memcpy(prefixKeySeedBuffer, chunkKeyRespondBuffer + chunkKeyOffset, SYSTEM_CIPHER_SIZE);
                    // memcpy(prefixKeySeedBuffer + SYSTEM_CIPHER_SIZE, batchList[i].chunk.logicData, SYSTEM_CIPHER_SIZE);
                    // cryptoObj_->generateHash(prefixKeySeedBuffer, SYSTEM_CIPHER_SIZE * 2, batchList[i].chunk.encryptKeyFeature);
                    memcpy(batchList[i].chunk.encryptKeyFeature, chunkKeyRespondBuffer + chunkKeyOffset, SYSTEM_CIPHER_SIZE);
                    memcpy(batchList[i].chunk.encryptKeyMLE, chunkKeyRespondBuffer + chunkKeyOffset + SYSTEM_CIPHER_SIZE, SYSTEM_CIPHER_SIZE);
                    // memset(batchList[i].chunk.encryptKeyMLE, 0, SYSTEM_CIPHER_SIZE);
                    // if (batchList[i].chunk.ID != currentChunkID) {
                    //     cout << "Chunk Sequence error, ID = " << batchList[i].chunk.ID << ",\t current = " << currentChunkID << endl;
                    // }
                    // currentChunkID++;
#if SYSTEM_DEBUG_FLAG == 1
                    cerr << "KeyClient : key = " << endl;
                    PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, batchList[i].chunk.encryptKeyFeature, SYSTEM_CIPHER_SIZE);
#endif
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timeendKey, NULL);
                    diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
                    second = diff / 1000000.0;
                    keyGenTime += second;
#endif
                    encoderObj_->insertMQ(batchList[i]);
                }
                batchList.clear();
                tempIDMap.clear();
                memset(chunkKeySeedBuffer, 0, SYSTEM_CIPHER_SIZE * keyBatchSize_);
                memset(chunkKeyRespondBuffer, 0, SYSTEM_CIPHER_SIZE * keyBatchSize_);
                batchNumber = 0;
            }
        }
        if (JobDoneFlag) {
            bool editJobDoneFlagStatus = encoderObj_->editJobDoneFlag();
            if (!editJobDoneFlagStatus) {
                cerr << "KeyClient : error to set job done flag for encoder" << endl;
            }
            break;
        }
    }
#if SYSTEM_BREAK_DOWN == 1
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
    cout << "KeyClient : key exchange mask generate work time = " << keyExchangeMaskGenerateTime << " s" << endl;
#endif
    cout << "KeyClient : key exchange encrypt/decrypt work time = " << keyExchangeEncTime << " s" << endl;
    cout << "KeyClient : key generate total work time = " << keyGenTime << " s" << endl;
    cout << "KeyClient : chunk hash work time = " << chunkHashTime << " s" << endl;
#endif
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
    bool saveStatus = saveClientCTRInfo();
    if (saveStatus != true) {
        cerr << "KeyClient : save ctr mode information error" << endl;
        exit(0);
    }
#if SYSTEM_DEBUG_FLAG == 1
    else {
        cerr << "KeyClient : save ctr mode information success" << endl;
    }
#endif
#endif
    return;
}

#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CFB
bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber)
{
    u_char sendHash[SYSTEM_CIPHER_SIZE * batchNumber + 32];
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartKey_enc;
    struct timeval timeendKey_enc;
    gettimeofday(&timestartKey_enc, NULL);
#endif
    cryptoObj_->keyExchangeEncrypt(batchHashList, batchNumber * SYSTEM_CIPHER_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
    cryptoObj_->sha256Hmac(sendHash, SYSTEM_CIPHER_SIZE * batchNumber, sendHash + SYSTEM_CIPHER_SIZE * batchNumber, keyExchangeKey_, 32);
#if SYSTEM_DEBUG_FLAG == 1
    cerr << "KeyClient : send key exchange hmac = " << endl;
    PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, sendHash + SYSTEM_CIPHER_SIZE * batchNumber, 32);
#endif
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    long diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    double second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    if (!keySecurityChannel_->send(sslConnection_, (char*)sendHash, SYSTEM_CIPHER_SIZE * batchNumber + 32)) {
        cerr << "KeyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[SYSTEM_CIPHER_SIZE * batchNumber + 32];
    int recvSize;
    if (!keySecurityChannel_->recv(sslConnection_, (char*)recvBuffer, recvSize)) {
        cerr << "KeyClient: recv socket error" << endl;
        return false;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char hmac[32];
    cryptoObj_->sha256Hmac(recvBuffer, SYSTEM_CIPHER_SIZE * batchNumber, hmac, keyExchangeKey_, 32);
    if (memcmp(hmac, recvBuffer + batchNumber * SYSTEM_CIPHER_SIZE, 32) != 0) {
        cerr << "KeyClient : recved keys hmac error" << endl;
#if SYSTEM_DEBUG_FLAG == 1
        cerr << "KeyClient : recv key exchange hmac = " << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, recvBuffer + SYSTEM_CIPHER_SIZE * batchNumber, 32);
        cerr << "KeyClient : client computed key exchange hmac = " << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, hmac, 32);
#endif
        return false;
    }
    batchkeyNumber = batchNumber;
    cryptoObj_->keyExchangeDecrypt(recvBuffer, batchkeyNumber * SYSTEM_CIPHER_SIZE, keyExchangeKey_, keyExchangeKey_, batchKeyList);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    return true;
}

#elif KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR

bool KeyClient::keyExchangeXOR(u_char* result, u_char* input, u_char* xorBase, int batchNumber)
{
    for (int i = 0; i < batchNumber * SYSTEM_CIPHER_SIZE; i++) {
        result[i] = input[i] ^ xorBase[i];
    }
    return true;
}

bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, NetworkHeadStruct_t netHead)
{
    int sendSize = sizeof(NetworkHeadStruct_t) + SYSTEM_CIPHER_SIZE * batchNumber + 32;
    u_char sendHash[sendSize];
    netHead.dataSize = batchNumber;
    memcpy(sendHash, &netHead, sizeof(NetworkHeadStruct_t));
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartKey_enc;
    struct timeval timeendKey_enc;
    long diff;
    double second;
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char keyExchangeXORBase[batchNumber * SYSTEM_CIPHER_SIZE * 2];
    cryptoObj_->keyExchangeCTRBaseGenerate(nonce_, counter_, batchNumber * 4, keyExchangeKey_, keyExchangeKey_, keyExchangeXORBase);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    second = diff / 1000000.0;
    keyExchangeMaskGenerateTime += second;
#endif
#if SYSTEM_DEBUG_FLAG == 1
    cerr << "key exchange mask = " << endl;
    PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, keyExchangeXORBase, batchNumber * SYSTEM_CIPHER_SIZE * 2);
#endif
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey_enc, NULL);
#endif
    keyExchangeXOR(sendHash + sizeof(NetworkHeadStruct_t), batchHashList, keyExchangeXORBase, batchNumber);
    cryptoObj_->sha256Hmac(sendHash + sizeof(NetworkHeadStruct_t), SYSTEM_CIPHER_SIZE * batchNumber, sendHash + sizeof(NetworkHeadStruct_t) + SYSTEM_CIPHER_SIZE * batchNumber, keyExchangeKey_, 32);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    if (!keySecurityChannel_->send(sslConnection_, (char*)sendHash, sendSize)) {
        cerr << "KeyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[SYSTEM_CIPHER_SIZE * batchNumber + 32];
    int recvSize;
    if (!keySecurityChannel_->recv(sslConnection_, (char*)recvBuffer, recvSize)) {
        cerr << "KeyClient: recv socket error" << endl;
        return false;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char hmac[32];
    cryptoObj_->sha256Hmac(recvBuffer, SYSTEM_CIPHER_SIZE * batchNumber, hmac, keyExchangeKey_, 32);
    if (memcmp(hmac, recvBuffer + batchNumber * SYSTEM_CIPHER_SIZE, 32) != 0) {
        cerr << "KeyClient : recved keys hmac error" << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, hmac, 32);
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, recvBuffer + batchNumber * SYSTEM_CIPHER_SIZE, 32);
        return false;
    }
    keyExchangeXOR(batchKeyList, recvBuffer, keyExchangeXORBase + batchNumber * SYSTEM_CIPHER_SIZE, batchNumber);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    return true;
}

#endif

bool KeyClient::insertMQ(Data_t& newChunk)
{
    return inputMQ_->push(newChunk);
}

bool KeyClient::extractMQ(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
}

bool KeyClient::editJobDoneFlag()
{
    inputMQ_->done_ = true;
    if (inputMQ_->done_) {
        return true;
    } else {
        return false;
    }
}
