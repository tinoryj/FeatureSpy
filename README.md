# FeatureSpy: Detecting Learning-Content Attacks via Feature Inspection in Secure Deduplicated Storage

## Introduction

FeatureSpy is a secure deduplicated storage system that effectively detects learning-content attacks based on the observation that such attacks often generate a large volume of similar data.

This repo contains the implementation of the FeatureSpy prototype and a trace analysis tool used in our paper.

* **./Prototype**: includes the implementation of prototype.
* **./Simulator**: includes a trace analysis tool to measure the detection and false-positive rate of FeatureSpy.

Note that each folder has a separate README file to introduce the detailed build and usage instructions.

## Simulator

The simulator is used to run the experiments in §VI.B in our paper. It includes `SyntheticSnapshots` and `CaseStudy` used for both Exp#1 and Exp#2, respectively. Note that these scripts we provide may need to run several days to complete the full test. If only simple verification is required, please modify them manually to reduce the parameters to be tested in the scripts.

### Dependencies

FeatureSpy simulator is developed under Ubuntu 20.04.3 LTS and depends on the following packages that need to be installed by the default `apt` package management tool.

```shell
sudo apt install -y build-essential openssl libssl-dev clang llvm python python3 curl git golang jq
```

## Build

Compile and cleanup the FeatureSpy simulator as follows.

```shell
cd Simulator/simCode
# compile
make
# cleanup
make clean
```

### Exp#1

Alternatively, we provide a quick way to analyze the detection effectiveness and false positive. You can use `runSnapDetection.sh` , `runSnapFalsePositive.sh` to test FeatureSpy's detection ratio and false positive under synthetic snapshots, respectively. Note that you can modify the parameters in the scripts to test different window sizes.

```txt
# test with detection ratio or false positive
cd Simulator/scripts
chmod +x *.sh
bash runSnapDetection.sh
bash runSnapFalsePositive.sh
```

For `runSnapDetection.sh`, the following messages will be output after the script is executed, including each condition and the detection ratio of the three schemes under that condition.

```txt
Snapshot size = 1024 MiB, fake file number (n) = 1024, modify pos (x) = 2, modify length (y) = 2
Detection ratio with 1 feature = 0.0000000000
Detection ratio with 2 feature = 0.0000000000
Detection ratio with 3 feature = 0.0000000000
...
```

For `runSnapFalsePositive.sh`, the following messages will be output after the script is executed, including each condition and the detection ratio of the three schemes under that condition.

```txt
Snapshot size = 1024 MiB, swapped ratio (r) = 0.01, modify pos (x) = 2, modify length (y) = 2
Detection ratio with 1 feature = 0.0000000000
Detection ratio with 2 feature = 0.0000000000
Detection ratio with 3 feature = 0.0000000000
...
```

### Exp#2: CaseStudy

We provide a quick way to analyze the detection effectiveness. You can use `runCouch.sh` , `runLinux.sh` , `runGCC.sh`, `runGitlab.sh` to test FeatureSpy with CouchDB, Linux, GCC and Gitlab, respectively. Note that you can modify the parameters in the scripts to test different window sizes. Here, we use CouchDB as an example.

```txt
# download trace
cd Simulator/traceDownload
chmod +x *.sh
python downloadCouchDB.py
# test with different trace
cd Simulator/scripts
chmod +x *.sh
bash runCouch.sh
```

In the running of the scripts, the program processes each snapshot and finally outputs the detection ratio and false positive results in the command line (via `stdout`).

```txt
Detection ratio with CouchDB dataset:
Detection ratio with 1 feature = 1.0000000000
Detection ratio with 2 feature = 1.0000000000
Detection ratio with 3 feature = 1.0000000000
False positive with CouchDB dataset:
Detection ratio with 1 feature = 0.0000000000
Detection ratio with 2 feature = 0.0000000000
Detection ratio with 3 feature = 0.0000000000
```

## Prototype

The FeatureSpy prototype augments the previous SGX-based encrypted deduplication system [SGXDedup](https://www.usenix.org/conference/atc21/presentation/ren-yanjing) with FeatureSpy, so as to detect the learning-content attack in a client-side trust-execution environment.

### Prerequisites

FeatureSpy prototype is tested on a machine that equips with a Gigabyte B460M-DS3H motherboard and an Intel i7-10700 CPU and runs Ubuntu 20.04.3 LTS.

Before running the prototype, check if your machine supports SGX. If there is an option such as `SGX` or `Intel Software Guard Extensions` in BIOS, then enable the option; otherwise, your machine does not support SGX. We strongly recommend finding the SGX-supported device in the [SGX hardware list](https://github.com/ayeks/SGX-hardware).

#### Dependencies

We now provide a one-step script to automatically install and configure the dependencies. The script will ask for a password for `sudo` operations if necessary. We have tested the script on Ubuntu 20.04.3 LTS.

```shell
chmod +x scripts/environmentInstall.sh
./scripts/environmentInstall.sh
```

Restart is required after the installation is finished. Then, check whether both `sgx_enclave` and `sgx_provision` are in `/dev`. If they are not in the directory (i.e., the SGX driver is not successfully installed), reinstall the SGX driver manually and restart the machine until `sgx_enclave` and `sgx_provision` are in `/dev`. We strongly recommend that you refer to the instructions of [SGX Installation Guide: Download Link](https://download.01.org/intel-sgx/sgx-linux/2.15.1/docs/Intel_SGX_SW_Installation_Guide_for_Linux.pdf) and [SGX SSL README: Link](https://github.com/intel/intel-sgx-ssl) during manual or automatic installation for troubleshooting.

### Configuration

FeatureSpy prototype is configured based on JSON. You can change its configuration without rebuilding it.

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

You can test the prototype in a single machine, and connect the key server, cloud, and client instances via the local loopback interface in `bin` directory. Since the key enclave needs to be attested by the cloud before usage, you need to start the cloud (`server-sgx`) first, then start the key server (`keymanager-sgx`), and wait for the message `KeyServerMain : key server remote attestation done, start to provide service` that indicates a successful attestation. **Currently, we have provided a set of Intel EPID-based remote attestation subscription keys for testing in `./Prototype/config.json`**

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

## Detailed Instructions

A detailed configuration of prototype can be found in [Prototype/README.md](Prototype/README.md), and [Simulator/README.md](Simulator/README.md) for simulator.