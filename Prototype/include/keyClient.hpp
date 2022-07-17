#ifndef FEATURESPY_KEYCLIENT_HPP
#define FEATURESPY_KEYCLIENT_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "enclaveSession.hpp"
#include "encoder.hpp"
#include "messageQueue.hpp"
#include "ssl.hpp"

class KeyClient {
private:
    messageQueue<Data_t>* inputMQ_;
    Encoder* encoderObj_;
    CryptoPrimitive* cryptoObj_;
    int keyBatchSize_;
    ssl* keySecurityChannel_;
    SSL* sslConnection_;
    u_char keyExchangeKey_[SYSTEM_CIPHER_SIZE];
    int clientID_;
    std::mutex mutexkeyGenerateSimulatorEncTime_;
    std::mutex mutexkeyGenerateSimulatorStart_;
    vector<timeval> keyGenSimulatorStartTimeCounter_;
    vector<timeval> keyGenSimulatorEndTimeCounter_;
    int totalSimulatorThreadNumber_;
    int currentInitThreadNumber_;
    int batchNumber_;

public:
    double keyExchangeEncTime = 0;
    KeyClient(Encoder* encoderObjTemp, u_char* keyExchangeKey);
    ~KeyClient();
    void run();
    void runKeyGenSimulator(int clientID);
    bool insertMQ(Data_t& newChunk);
    bool extractMQ(Data_t& newChunk);
    bool editJobDoneFlag();
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber);
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
    double keyExchangeMaskGenerateTime = 0;
    u_char nonce_[CRYPTO_BLOCK_SIZE - sizeof(uint32_t)];
    uint32_t counter_ = 0;
    bool initClientCTRInfo();
    bool saveClientCTRInfo();
    bool keyExchangeXOR(u_char* result, u_char* input, u_char* xorBase, int batchNumber);
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, NetworkHeadStruct_t netHead);
#endif
};

#endif //FEATURESPY_KEYCLIENT_HPP
