#ifndef FEATURESPY_CHUNK_HPP
#define FEATURESPY_CHUNK_HPP

#include "configure.hpp"
#include <bits/stdc++.h>

using namespace std;

typedef struct {
    u_char hash[SYSTEM_CIPHER_SIZE];
} Hash_t;
// system basic data structures
typedef struct {
    uint32_t ID;
    int type;
    int logicDataSize;
    u_char logicData[MAX_CHUNK_SIZE];
    u_char chunkHash[SYSTEM_CIPHER_SIZE];
    u_char encryptKeyMLE[SYSTEM_CIPHER_SIZE];
    u_char encryptKeyFeature[SYSTEM_CIPHER_SIZE];
} Chunk_t;

typedef struct {
    int logicDataSize;
    char logicData[MAX_CHUNK_SIZE];
    char chunkHash[SYSTEM_CIPHER_SIZE];
} StorageCoreData_t;

typedef struct {
    uint32_t ID;
    int logicDataSize;
    char logicData[MAX_CHUNK_SIZE];
} RetrieverData_t;

typedef struct {
    uint32_t chunkID;
    int chunkSize;
    u_char chunkHash[SYSTEM_CIPHER_SIZE];
    u_char chunkKeyMLE[SYSTEM_CIPHER_SIZE];
    u_char chunkKeyFeature[SYSTEM_CIPHER_SIZE];
} RecipeEntry_t;

typedef vector<Chunk_t> ChunkList_t;
typedef vector<RecipeEntry_t> RecipeList_t;

typedef struct {
    uint64_t fileSize;
    u_char fileNameHash[SYSTEM_CIPHER_SIZE];
    uint64_t totalChunkNumber;
} FileRecipeHead_t;

typedef struct {
    uint64_t fileSize;
    u_char fileNameHash[SYSTEM_CIPHER_SIZE];
    uint64_t totalChunkKeyNumber;
} KeyRecipeHead_t;

typedef struct {
    FileRecipeHead_t fileRecipeHead;
    KeyRecipeHead_t keyRecipeHead;
} Recipe_t;

typedef struct {
    union {
        Chunk_t chunk;
        Recipe_t recipe;
    };
    int dataType;
} Data_t;

typedef struct {
    u_char originHash[SYSTEM_CIPHER_SIZE];
    u_char key[SYSTEM_CIPHER_SIZE];
} KeyGenEntry_t;

typedef struct {
    int messageType;
    int clientID;
    int dataSize;
} NetworkHeadStruct_t;

// database data structures

typedef struct {
    u_char containerName[16];
    uint32_t offset;
    uint32_t length;
} keyForChunkHashDB_t;

typedef struct {
    char RecipeFileName[SYSTEM_CIPHER_SIZE];
    uint32_t version;
} keyForFilenameDB_t;

typedef struct {
    u_char hash_[SYSTEM_CIPHER_SIZE];
} chunkHash_t;

#endif //FEATURESPY_CHUNK_HPP
