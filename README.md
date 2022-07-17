# FeatureSpy: Detecting Learning-Content Attacks via Feature Inspection in Secure Deduplicated Storage

## Introduction

Secure deduplicated storage is a critical paradigm for cloud storage outsourcing to achieve both operational cost savings via deduplication and outsourced data confidentiality via encryption.  However, existing secure deduplicated storage designs are vulnerable to learning-content attacks, in which malicious clients attempt to infer the sensitive contents of outsourced data by monitoring the deduplication pattern.  We show via a simple example that learning-content attacks are indeed feasible and can infer sensitive information in short time under a real cloud setting.  To this end, we present FeatureSpy, a secure deduplicated storage system that effectively detects learning-content attacks based on the observation that such attacks often generate a large volume of similar data. FeatureSpy builds on two core design elements, namely (i) similarity-preserving encryption that supports similarity detection on encrypted chunks and (ii) shielded attack detection that leverages Intel SGX to accurately detect learning-content attacks without being readily evaded by adversaries.  Trace-driven experiments on real-world and synthetic datasets show that our FeatureSpy prototype achieves  high accuracy and low performance overhead in attack detection.

## Prerequisites

FeatureSpy is tested on a machine that equips with a Gigabyte B460M-DS3H motherboard and an Intel i7-10700 CPU and runs Ubuntu 20.04.3 LTS.

Before running FeatureSpy, check if your machine supports SGX. If there is an option as `SGX` or `Intel Software Guard Extensions` in BIOS, then enable the option; otherwise, your machine does not support SGX. We strongly recommend finding the SGX-supported device in the [SGX hardware list](https://github.com/ayeks/SGX-hardware).

### Registration

FeatureSpy uses EPID-based remote attestation, and you need to register at the [EPID attestation page](https://api.portal.trustedservices.intel.com/EPID-attestation). Then, you can find your SPID and the corresponding subscription keys (both the primary and the secondary keys) at the [products page](https://api.portal.trustedservices.intel.com/products). Our test uses the `DEV Intel® Software Guard Extensions Attestation Service (Unlinkable)` product.


### Dependencies

FeatureSpy depends on the following packages that need to be installed manually or by package management tools.

1. Intel® Software Guard Extensions (Intel® SGX) driver version 2.11.0_2d2b795 [Download Link](https://download.01.org/intel-sgx/sgx-linux/2.15.1/distro/ubuntu20.04-server/sgx_linux_x64_driver_2.11.0_2d2b795.bin)
2. Intel® SGX SDK version 2.15.101.1 [Download Link](https://download.01.org/intel-sgx/sgx-linux/2.15.1/distro/ubuntu20.04-server/sgx_linux_x64_sdk_2.15.101.1.bin)
3. Intel® SGX SSL version lin_2.15.1_1.1.1l [Download Link](https://github.com/intel/intel-sgx-ssl/archive/refs/tags/lin_2.15.1_1.1.1l.zip)
4. OpenSSL version 1.1.1l [Donwload Link](https://www.openssl.org/source/old/1.1.1/openssl-1.1.1l.tar.gz)
5. SGX packages including `libsgx-epid` `libsgx-quote-ex` `libsgx-dcap-ql` `ibsgx-urts` `libsgx-launch` `libsgx-enclave-common-dev` `libsgx-uae-service`... etc (See SGX Installation Guide [Download Link](https://download.01.org/intel-sgx/sgx-linux/2.15.1/docs/Intel_SGX_SW_Installation_Guide_for_Linux.pdf) for detail).
6. libssl-dev (For FeatureSpy encryption algorithm)
7.  libboost-all-dev (For FeatureSpy multithreading, message transmission, etc.)
8.  libleveldb-dev (For FeatureSpy deduplication index based on LevelDB)
9.  libsnappy-dev (Required by LevelDB)
10. build-essential (Basic program compilation environment)
11. cmake (CMake automated build framework)
12. wget (System components used for remote attestation requests)

We now provide a one-step script to automatically install and configure the dependencies. We have tested the script on Ubuntu 20.04.3 LTS. Note that the script will ask for a password for sudo operations if necessary

```shell
chmod +x Scripts/environmentInstall.sh
./Scripts/environmentInstall.sh
```

Restart is required after the installation is finished. Then, check whether `sgx_enclave` and `sgx_provision` are in `/dev`. If it is not in the directory (i.e., SGX driver is not successfully installed), reinstall the SGX driver manually and restart the machine until `sgx_enclave` and `sgx_provision` are in `/dev`. We strongly recommend that you refer to the instructions of [SGX Installation Guide: Download Link](https://download.01.org/intel-sgx/sgx-linux/2.15.1/docs/Intel_SGX_SW_Installation_Guide_for_Linux.pdf) and [SGX SSL README: Link](https://github.com/intel/intel-sgx-ssl) during manual or automatic installation for troubleshooting.


## FeatureSpy Running Guide

### Configuration

FeatureSpy is configured based on JSON. You can change its configuration without rebuilding. We show the default configuration (`./config.json`) of FeatureSpy as follows.

```json
{
    "ChunkerConfig": {
        "_chunkingType": "VariableSize", // FixedSize: fixed size chunking; VariableSize: variable size chunking; TraceFSL: FSL dataset hash list; TraceMS: MS dataset hash list
        "_minChunkSize": 4096, // The smallest chunk size in variable size chunking, Uint: Byte (Maximum size 16KB)
        "_avgChunkSize": 8192, // The average chunk size in variable size chunking and chunk size in fixed size chunking, Uint: Byte (Maximum size 16KB)
        "_maxChunkSize": 16384, // The biggest chunk size in variable size chunking, Uint: Byte (Maximum size 16KB)
        "_slidingWinSize": 48, // The sliding window size for rabin fingerprinting in variable size chunking, Uint: Byte
        "_ReadSize": 256 // System read input file size every I/O operation, Uint: MB
    },
    "KeyServerConfig": {
        "_keyBatchSize": 4096, // Maximum number of keys obtained per communication
        "_keyServerRArequestPort": 1559, // Key server host port for receive key enclave remote attestation request 
        "_keyServerIP": [
            "127.0.0.1"
        ], // Key server host IP ()
        "_keyServerPort": [
            6666
        ], // Key server host port for client key generation
        "_keyRegressionMaxTimes": 1048576, // Key regression maximum numbers `n`
        "_keyRegressionIntervals": 25920000 // Time interval for key regression (Unit: seconds), used for key enclave. Should be consistent with "server._keyRegressionIntervals"
    },
    "SPConfig": {
        "_storageServerIScriptsP": [
            "127.0.0.1"
        ], // Storage server host IP
        "_storageServerPort": [
            6668
        ], // Storage server host port for client upload or download files
        "_maxContainerSize": 8192 // Maximum space for one-time persistent chunk storage, Uint: KiB (Maximum size 8MB)
    },
    "pow": {
        "_quoteType": 0, // Enclave quote type, do not modify it 
        "_iasVersion": 3, // Enclave IAS version, do not modify it 
        "_iasServerType": 0, // Server IAS version, do not modify it
        "_batchSize": 4096, // POW enclave batch size (Unit: chunks)
        "_ServerPort": 6669, // The port on storage server for remote attestation
        "_enclave_name": "pow_enclave.signed.so", // The enclave library name to create the target enclave
        "_SPID": "", // Your SPID for remote attseation service
        "_PriSubscriptionKey": "", // Your Intel remote attestation service primary subscription key
        "_SecSubscriptionKey": "" // Your Intel remote attestation service secondary subscription key
    },
    "km": {
        "_quoteType": 0, // Enclave quote type, do not modify it 
        "_iasVersion": 3, // Enclave IAS version, do not modify it 
        "_iasServerType": 0, // Server IAS version, do not modify it
        "_ServerPort": 6676, // The port on storage server for remote attestation
        "_enclave_name": "km_enclave.signed.so", // The enclave library name to create the target enclave
        "_SPID": "", // Your SPID for remote attseation service
        "_PriSubscriptionKey": "", // Your Intel remote attestation service primary subscription key
        "_SecSubscriptionKey": "" // Your Intel remote attestation service secondary subscription key
    },
    "server": {
        "_RecipeRootPath": "Recipes/", // Path to the file recipe storage directory
        "_containerRootPath": "Containers/", // Path to the unique chunk storage directory
        "_fp2ChunkDBName": "db1", // Path to the chunk database directory
        "_fp2MetaDBame": "db2" // Path to the file recipe database directory
        "_raSessionKeylifeSpan": 259200000 // Time interval for key regression (Unit: seconds), used for storage server. Should be consistent with "KeyServerConfig._keyRegressionIntervals"
    },
    "client": {
        "_clientID": 1, // Current client ID 
        "_sendChunkBatchSize": 1000, // Maximum number of chunks sent per communication
        "_sendRecipeBatchSize": 100000 // Maximum number of file recipe entry sent per communication
    },
    "FeatureSpy": {
        "_keySeedFeatureNumber": 3, // Total number of features generated in FBE/SPE. Default to 3
        "_keySeedGenFeatureThreadNumber": 3 // Thread number of FBE/SPE, default to 3
    }
}
```

Before starting, you need to fill the SPID and subscription keys in `./config.json` based on your registration information in Intel.

```json
...
"pow": {
    ...
    "_SPID": "", // Your SPID for remote attseation service
    "_PriSubscriptionKey": "", // Your Intel remote attestation service primary subscription key
    "_SecSubscriptionKey": "" // Your Intel remote attestation service secondary subscription key
},
"km": {
    ...
    "_SPID": "", // Your SPID for remote attseation service
    "_PriSubscriptionKey": "", // Your Intel remote attestation service primary subscription key
    "_SecSubscriptionKey": "" // Your Intel remote attestation service secondary subscription key
},
...
```

If you need to adjust FeatureSpy's detection indicator length (L), window size (W), threshold (T), and `firstFeature`
, `minFeature`, and `allFeature` three schemes, please modify the values of the following parameters in the `include/systemSettings.hpp` file.

```c++
...
/* FBE Key Generation method Settings */
#define SUPER_FEATURE_GEN_METHOD FIRST_FEATURE // FIRST_FEATURE will only generate first 4 sub-features and corresponding feaure; MIN_FEATURE will generate '_keySeedFeatureNumber' sub-features and '_keySeedSuperFeatureNumber' then select the minimal feature; ALL_FEATURE will generate '_keySeedFeatureNumber' sub-features and '_keySeedSuperFeatureNumber' then generate the target allFeature;

/* FBE Key Generation method Settings */
#define INDICATOR_LENGTH 32 // Default to 2 encryption blocks
#define PREFIX_FREQUENCY_THRESHOLD 15 // Here is the frequency value, please set it according to T*W in the paper
#define COUNT_WINDOW_SIZE 5000 // Default window size W = 5000
...
```

### Build

Compile FeatureSpy as follows.

```shell
mkdir -p bin && mkdir -p build && mkdir -p lib && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && make

cd ..
cp lib/*.a bin/
cp ./lib/pow_enclave.signed.so ./bin
cp ./lib/km_enclave.signed.so ./bin
cp config.json bin/
cp -r key/ bin/
mkdir -p bin/Containers && mkdir -p bin/Recipes
```

Alternatively, we provide a script for a quick build and clean-up, and you can use it.

```shell
chmod +x ./Scripts/*.sh
# Build FeatureSpy in release mode
./Scripts/buildReleaseMode.sh
# Build FeatureSpy in debug mode
./Scripts/buildDebugMode.sh
# Clean up build result
./Scripts/cleanBuild.sh
```

Note that, the generated executable file and its required enclave dynamic library, keys, etc. are all stored in the `bin` directory. 

### Usage

You can test FeatureSpy in a single machine, and connect the key manager, server (e.g., the cloud in the ATC paper), and client instances via the local loopback interface in `bin` directory.

```shell
# start cloud
cd bin
./server-sgx

# start key manager
cd bin
./keymanager-sgx
```

Since the key enclave needs to be attested by the cloud before usage, you need to start the cloud (`server-sgx`) first, then start the key manager (`keymanager-sgx`), and wait for the message `KeyServer : keyServer session key update done` that indicates a successful attestation.

FeatureSpy automatically verifies the PoW enclave by remote attestation in the first startup of the client, and by unsealing in the following startups. FeatureSpy provides store and restores interfaces to clients.

```shell
# store file
cd bin
./client-sgx -s file

# restore file
cd bin
./client-sgx -r file
```

Note that we do not provide any command-line interface for renewing the blinded key. Instead, you can configure `KeyServerConfig._keyRegressionIntervals` and `server._raSessionKeylifeSpan` (in the unit of a second) in `config.json` to set the periods for the key renewing cycle in the key manager and the cloud, respectively. Also, you can configure `_keyRegressionMaxTimes` to control the maximum number of key regression (2^20 by default).
