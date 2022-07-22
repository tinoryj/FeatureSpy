# FeatureSpy: Detecting Learning-Content Attacks via Feature Inspection in Secure Deduplicated Storage

## Introduction

FeatureSpy is a secure deduplicated storage system that effectively detects learning-content attacks based on the observation that such attacks often generate a large volume of similar data.

This repo contains the implementation of the FeatureSpy prototype and a trace analysis tool used in our paper.

* **./Prototype**: includes the implementation of prototype.
* **./Simulator**: includes a trace analysis tool to measure the detection and false-positive rate of FeatureSpy.

Note that each folder has a separate README file to introduce the build instructions and usage.

## Simulator

The simulator is used to run the experiments in §VI.B in our paper. It includes `SyntheticSnapshots` and `CaseStudy` used for both Exp#1 and Exp#2, respectively.
### Dependencies

FeatureSpy simulator is developed under Ubuntu 20.04.3 LTS and depends on the following packages that need to be installed by the default `apt` package management tool.

```shell
sudo apt install -y build-essential openssl libssl-dev clang llvm python python3 curl git golang jq
```

### Exp#1: SyntheticSnapshots

### Exp#2: CaseStudy

#### Build

Compile and cleanup the simulator as follows.

```shell
cd ./Simulator/CaseStudy
# compile
make
# cleanup
make clean
```

#### Usage

We provide a quick way to analyze the detection effectiveness. You can use `runCouch.sh` and `runLinux.sh` to test FeatureSpy with CouchDB and Linux, respectively, and use `processResult.sh` to output the summary of results. Note that you can modify the parameters in `runCouch.sh` and `runLinux.sh` to test different window sizes and similarity indicator lengths. Alternatively, you can follow [FeatureSpy/README.pdf](FeatureSpy/README.pdf) to manually run the simulator.

```shell
# download trace
cd FeatureSpy/traceDownload
chmod +x *.sh
bash downloadTraceCouch.sh
bash downloadTraceLinux.sh
# generate fake offers
cd FeatureSpy/SimulateOfferGenerator
chmod +x generateFakeOffers.sh
bash generateFakeOffers.sh
# test with CouchDB and Linux trace
cd FeatureSpy/
chmod +x *.sh
bash runCouch.sh
bash runLinux.sh
bash processResult.sh
```

In the running of `runLinux.sh`/`runCouch.sh`, the program processes each snapshot and outputs the detection results in the command line (via `stderr`).


```shell
firstFeature: not detected
minFeature: detected
allFeature: not detected
```

Also, it saves the feature distribution information of each window of the processing snapshot in `FeatureSpy/linuxResult/` and  `FeatureSpy/couchResult/` for Linux and CouchDB datasets, respectively. The file name of each raw snapshot is `${snpashotID}-origin-${windowSize}-${indicatorLength}.csv`, while that of each attack snapshot (i.e., adversarially injected with fake offers) is `${snpashotID}-mixed-${windowSize}-${indicatorLength}.csv`. An example file is shown as follows, where each row represents the fraction of the most number of chunks that have the same similarity indicator in a window.

| firstFeature | minFeature | allFeature |
| ------------ | ---------- | ---------- |
| 0.08         | 0.08       | 0.08       |
| 0.001        | 0.002      | 0.001      |
| 0.003        | 0.003      | 0.003      |


Furthermore, the script `processResult.sh` generates the file `mergedLinuxResult-${windowSize}-${indicatorLength}.csv`/`mergedCouchResult-${windowSize}-${indicatorLength}.csv` to summarize the final results of attack detection for all snapshots (see below). Each row represents the maximum fraction of the most number of chunks that have the same similarity indicator among all windows in each snapshot. Note that the raw (mixed) here means the snapshot without (with) injected faked offers.

| firstFeature max freq (raw) | minFeature max freq (raw) | allFeature max freq (raw) | firstFeature max freq (mixed) | minFeature max freq (mixed) | allFeature max freq (mixed) |
| :-------------------------- | ------------------------- | ------------------------- | ----------------------------- | --------------------------- | --------------------------- |
| 0.0032                      | 0.0028                    | 0.0028                    | 0.1106                        | 0.1106                      | 0.1106                      |
| 0.003                       | 0.003                     | 0.003                     | 0.1022                        | 0.1022                      | 0.1022                      |
| 0.0026                      | 0.0024                    | 0.0024                    | 0.1026                        | 0.1026                      | 0.1026                      |
| 0.0026                      | 0.0022                    | 0.0022                    | 0.0972                        | 0.0972                      | 0.0972                      |


## Prototype

The FeatureSpy prototype augments the previous SGX-based encrypted deduplication system [SGXDedup](https://www.usenix.org/conference/atc21/presentation/ren-yanjing) with FeatureSpy, so as to detect the learning-content attack in a client-side trust-execution environment.

### Prerequisites

FeatureSpy prototype is tested on a machine that equips with a Gigabyte B460M-DS3H motherboard and an Intel i7-10700 CPU, and runs Ubuntu 20.04.3 LTS.

Before running the prototype, check if your machine supports SGX. If there is an option such as `SGX` or `Intel Software Guard Extensions` in BIOS, then enable the option; otherwise, your machine does not support SGX. We strongly recommend finding the SGX-supported device in the [SGX hardware list](https://github.com/ayeks/SGX-hardware).

#### Registration

FeatureSpy prototype uses EPID-based remote attestation, and you need to register at the [EPID attestation page](https://api.portal.trustedservices.intel.com/EPID-attestation). Then, you can find your SPID and the corresponding subscription keys (both the primary and the secondary keys) on the [products page](https://api.portal.trustedservices.intel.com/products). Our test uses the `DEV Intel® Software Guard Extensions Attestation Service (Unlinkable)` product. **Currently, we provide a set of Intel EPID-based remote attestation subscription keys for testing in `./Prototype/config.json`**


#### Dependencies

We now provide a one-step script to automatically install and configure the dependencies. The script will ask for a password for sudo operations if necessary. We have tested the script on Ubuntu 20.04.3 LTS.

```shell
chmod +x scripts/environmentInstall.sh
./scripts/environmentInstall.sh
```

Restart is required after the installation is finished. Then, check whether both `sgx_enclave` and `sgx_provision` are in `/dev`. If they are not in the directory (i.e., the SGX driver is not successfully installed), reinstall the SGX driver manually and restart the machine until `sgx_enclave` and `sgx_provision` are in `/dev`. We strongly recommend that you refer to the instructions of [SGX Installation Guide: Download Link](https://download.01.org/intel-sgx/sgx-linux/2.15.1/docs/Intel_SGX_SW_Installation_Guide_for_Linux.pdf) and [SGX SSL README: Link](https://github.com/intel/intel-sgx-ssl) during manual or automatic installation for troubleshooting.


### Configuration

FeatureSpy prototype is configured based on JSON. You can change its configuration without rebuilding it. A detailed configuration of FeatureSpy prototype can be found in [Prototype/README.md](Prototype/README.md).

### Build

We provide a script for a quick build and clean-up, and you can use it.

```shell
chmod +x ./scripts/*.sh
# Build FeatureSpy in release mode
./scripts/buildPrototype.sh
# Clean up build result
./scripts/cleanupPrototype.sh
```

The generated executable file and its required enclave dynamic library, keys are all stored in the `bin` directory.

### Usage

You can test the prototype in a single machine, and connect the key server, cloud, and client instances via the local loopback interface in `bin` directory. Since the key enclave needs to be attested by the cloud before usage, you need to start the cloud (`server-sgx`) first, then start the key server (`keymanager-sgx`), and wait for the message `KeyServerMain : key server remote attestation done, start to provide service` that indicates a successful attestation.

```shell
# start cloud
cd bin
./server-sgx

# start key server
cd bin
./keymanager-sgx
```

FeatureSpy prototype provides store and restores interfaces to clients.

```shell
# store file
cd bin
./client-sgx -s file

# restore file
cd bin
./client-sgx -r file
```
