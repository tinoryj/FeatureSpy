# FeatureSpy Simulator

The simulator is used to run the experiments in §VI.B in our paper.

## Dependencies

FeatureSpy simulator is developed under Ubuntu 20.04.3 LTS and depends on the following packages that need to be installed manually or by the default `apt` package management tool.

1. OpenSSL version 1.1.1l [Donwload Link](https://www.openssl.org/source/old/1.1.1/openssl-1.1.1l.tar.gz)
2. libssl-dev (For FeatureSpy encryption algorithm)
3. clang/llvm (For compiling source code)
4. python3 (For faking offer file generator)
5. git (System components used for downloading Linux datasets)
6. curl/golang/jq (System components used for downloading docker-based datasets)
7. tqdm (For ZipF distribution generator)

The above packages can be installed via the `apt` and `pip` package management tool with the following commands on Ubuntu.

```txt
sudo apt install -y build-essential openssl libssl-dev clang llvm python3 curl git golang jq python3-pip
pip3 install tqdm
```

## Build

Compile and clean up the FeatureSpy simulator as follows.

```txt
cd Simulator/simCode
# compile
make
# cleanup
make clean
```

## Trace download

Download Linux, GCC, CouchDB, and Gitlab datasets, respectively, as follows. Note that the snapshots will be downloaded to the `packed-linux`, `packed-gcc`, `packed-couch`, and `packed-gitlab` directories under the `Simulator/traceDownload` directory, respectively.

```shell
# download trace
cd Simulator/traceDownload
chmod +x *.sh

# download Linux dataset
bash downloadLinux.sh

# download GCC dataset
bash downloadGCC.sh

# download CouchDB dataset
python3 downloadCouchDB.py

# download Gitlab dataset
python3 downloadGitlabCE.py
```

## Exp#1: Trade-off study

### Automatic scripts

We provide a quick way to analyze the trade-off between detection accuracy and false positive. You can use `runSnapDetection.sh` , `runSnapFalsePositive.sh` to test FeatureSpy's detection ratio and false positive under synthetic snapshots, respectively. Note that you can modify the parameters in the scripts to test different window sizes.

```txt
# test with detection ratio or false positive
cd Simulator/scripts
chmod +x *.sh
bash runSnapDetection.sh
bash runSnapFalsePositive.sh
```

For `runSnapDetection.sh`, the following messages will be output after the script is executed, including each condition's information and the detection ratio of the three schemes under that condition.

```txt
Snapshot size = 1024 MiB, fake file number (n) = 1024, modify pos (x) = 2, modify length (y) = 2
Detection ratio with 1 feature = 0.0000000000
Detection ratio with 2 feature = 0.0000000000
Detection ratio with 3 feature = 0.0000000000
...
```

For `runSnapFalsePositive.sh`, the following messages will be output after the script is executed, including each condition's information and the false positive ratio of the three schemes under that condition.

```txt
Snapshot size = 1024 MiB, swapped ratio (r) = 0.01, modify pos (x) = 2, modify length (y) = 2
Detection ratio with 1 feature = 0.0000000000
Detection ratio with 2 feature = 0.0000000000
Detection ratio with 3 feature = 0.0000000000
...
```

In addition to the direct output results, you can further calculate detection accuracy and false positive of FeatureSpy under different thresholds according to the intermediate result files in the `Simulator/scripts/SYNDetectionResults` and `Simulator/scripts/SYNFalseResults` directory for detection ratio and false positive, respectively. An example file is shown as follows, where each row represents the fraction of the highest frequency similar chunk in the snapshots detected by FeatureSpy based on the window under three schemes.

| Fraction of s=1 | Fraction of s=2 | Fraction of s=3  |
| ------------ | ---------- | ---------- |
| 0.008         | 0.008       | 0.008       |
| 0.001        | 0.002      | 0.001      |
| 0.003        | 0.003      | 0.003      |


You can refer to the following instructions to obtain the detection ratio or false positive of the three schemes under different thresholds T.

```txt 
# param 1: Processed raw dataset result or mixed fake files result (${x} indicate the parameter in the paper).
# param 2: Threshold T 

# For detection ratio:
cd Simulator/scripts/SYNDetectionResults
./genDetectionRate Detection-x-${x}-y-${y}-n-${n}-Window-${w}.csv 0.01

# For false positive:
cd Simulator/scripts/SYNFalseResults
./genDetectionRate FalsePositive-x-${x}-y-${y}-r-${r}-Window-${w}.csv 0.01

# The output:
Detection ratio with 1 feature = 1.0000000000
Detection ratio with 2 feature = 1.0000000000
Detection ratio with 3 feature = 1.0000000000
```

### Manual

**Step 1:** Generate the baseline synthetic snapshots.

```txt
cd Simulator/simCode
chmod +x *.sh

# Generate the size information for each file in the target snapshot.
# param 1: -c 1.0 indicates the parameter for Zipf distribution is 1.0
# param 2: -s 1024 indicates the target snapshot size is 1024 MiB 
python3 GenZipf.py -o syn_zipf_1.0 -c 1.0 -s 1024

# Generate the chunks' information (ID, size, hash, feature, and indicator) for each file in the target snapshot.
# param 1: the file size information list generated by the GenZipf.py script
# param 2: 8192 indicates the chunk size of the generated snapshot will be 8 KiB
./baselineSnapGen syn_zipf_1.0 8192
```

**Step 2:** Generate synthetic snapshots based on the baseline snapshot.

For the detection ratio, refer to the following script to generate the synthetic snapshot, which is mixed with fake files.

```txt
cd Simulator/simCode
# param 1: the file information list generated by Step 1.
# param 2: the chunk information list generated by Step 1.
# param 3: the inject file size (default to 8 KiB)
# param 4,5,6,7: x,y,n,random seed.
# Output: it will generate the file and chunk information list "${chunkSize}-${x}-${y}-${n}-${randomSeed}.chunkInfo" of the mixed fake files' snapshot (the content of the fake files are modified based on one random baseline file)
./mixedFilesGenSYN syn_zipf_1.0.fileInfo syn_zipf_1.0.chunkInfo 8192 ${x} ${y} ${n} ${randomSeed}
```

For false positives, refer to the following script to generate a synthetic snapshot with different similarities.

```txt
cd Simulator/simCode
# param 1: the file information list generated by Step 1.
# param 2: the chunk information list generated by Step 1.
# param 3: the swapped chunk size (default to 8 KiB)
# param 4,5,6,7: x,y,r,random seed.
# Output: it will generate the file and chunk information list "syn_zipf_1.0.swap.chunkInfo" of the mixed fake files' snapshot (the content of the fake files is modified based on one random baseline file). 
./swappedSnapGen syn_zipf_1.0 8192 ${x} ${y} ${r} ${randomSeed} 
```

**Step 3:** Detect the learning-content attack based on generated chunks' information.

```txt
# Process each window to count the max frequency of each window.
# param 1: Chunk info list `${chunkSize}-${x}-${y}-${n}-${randomSeed}.chunkInfo` generated by `mixedFilesGenSYN` or `syn_zipf_1.0.swap.chunkInfo` generated by `swappedSnapGen`
# param 2: Window size (W); use the default window size W=16000 as an example
# Output: Redirect `windowCheck`'s stdout to the specified file for recording the maximum frequency of each window under the different schemes; use result-mixed.csv as an example
./windowCheck syn_zipf_1.0.swap.chunkInfo 16000 > result-mixed.csv

# Process each window to count the max frequency of the snapshot.
# param 1: window check result generated by `windowCheck`; use result-mixed.csv as an example
# Output: Redirect processWindowCheckResult's stdout to the specified file for recording the maximum frequency in the target snapshot under the different schemes (use the Indicator); use window-mixed.csv as an example
./processWindowCheckResult result-mixed.csv >> window-mixed.csv
```

The `window-mixed.csv` file records a similar frequency in the window with the highest frequency in the target snapshot in the case of s=1, s=2, and s=3. The format of the content is as follows:

```txt
0.1860000000 , 0.1860000000 , 0.1860000000
```

You can directly compare the value with your preset threshold, and when the value is greater than the threshold, determine that `FeatureSpy` detects an attack in the snapshot. In addition, you can use `genDetectionRate` as follows to output the detection rate of different schemes.

```txt
# param 1: Processed window check result generated by processWindowCheckResult; use window-mixed.csv as an example
# param 2: Threshold T
./genDetectionRate window-mixed.csv 0.01
# Output:
Detection ratio with 1 feature = 1.0000000000
Detection ratio with 2 feature = 1.0000000000
Detection ratio with 3 feature = 1.0000000000
```

## Exp#2: Case study of attack detection

### Automatic scripts

We provide a quick way to analyze the detection effectiveness. You can use `runCouch.sh` , `runLinux.sh` , `runGCC.sh`, `runGitlab.sh` to test FeatureSpy with CouchDB, Linux, GCC, and Gitlab, respectively. Note that you can modify the parameters in the scripts to test different window sizes.

```txt
# download trace
cd Simulator/traceDownload
chmod +x *.sh
python3 downloadCouchDB.py
python3 downloadGitlabCE.py
bash downloadLinux.sh
bash downloadGCC.sh

# test with different trace
cd Simulator/scripts
chmod +x *.sh
bash runCouch.sh
bash runLinux.sh
bash runGCC.sh
bash runGitlab.sh
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

As in Exp#1, the simulator also saves the intermediate results in `Simulator/scripts/${dataset}StudyResult/` for manual analysis of the effects under different thresholds. Use CouchDB as an example, the `orignalPrefixFreq-Couch-WindowSize-16000.csv` stores the result for false positive calculation, and `mixedFakeOffersPrefixFreq-Couch-WindowSize-16000.csv` stores the result for detection ratio calculation. 

Here, you can use `genDetectionRate` in the `Simulator/scripts/couchCaseStudyResult/` directory to manually test the detection ratio and false positive of the dataset under different thresholds as follow (use the analysis of false positive as an example).

```txt 
# param 1: Processed raw dataset result or mixed fake offers result.
# param 2: Threshold T
./genDetectionRate orignalPrefixFreq-Couch-WindowSize-16000.csv 0.01
# Output:
Detection ratio with 1 feature = 1.0000000000
Detection ratio with 2 feature = 1.0000000000
Detection ratio with 3 feature = 1.0000000000
```

### Manual

**Step 1:** Generate faked offers that enumerate salary and sign-on bonus. We have packaged the original Google offer letter in `Simulator/fakeOfferGen`. Alternatively, it is feasible to download the original offer from [here](https://www.sec.gov/Archives/edgar/data/1288776/000119312508140342/dex101.htm). In addition to the default range in our paper, you can modify the parameters in `generateFakeOffers.sh` to change the ranges and cardinalities of the salary and sign-on bonus, respectively. The generated offers are stored in the `Simulator/fakeOfferGen/result` directory.

```txt
cd Simulator/fakeOfferGen
chmod +x generateFakeOffers.sh
bash generateFakeOffers.sh
```

**Step 2:** Generate the list of the faked offer files.

```txt
cd Simulator
chmod +x *.sh
# param 1: path to the folder where the target files are stored
# param 2: path to store the generated file list
bash generateFileList.sh fakeOfferGen/result/ fakeFile.fileList
```

An example of the file list is shown below. Each line corresponds to an adversarially faked file that enumerates the possible values of the salary and sign-on bonus.

```txt
/home/xxx/xxx/Simulator/fakeOfferGen/result/offer-base-204-30.html
/home/xxx/xxx/Simulator/fakeOfferGen/result/offer-base-204-31.html
/home/xxx/xxx/Simulator/fakeOfferGen/result/offer-base-204-32.html
/home/xxx/xxx/Simulator/fakeOfferGen/result/offer-base-204-33.html
/home/xxx/xxx/Simulator/fakeOfferGen/result/offer-base-204-34.html
...
```

**Step 3:** Generate the list of files in each Linux/CouchDB/GCC/Gitlab snapshot. Here we use CouchDB 2.5.2 as an example and store the untared contents in `Simulator/simCode/2.5.2`.

```txt
cd Simulator/simCode
chmod +x *.sh
tar -xvf ../traceDownload/packed-couch/2.5.2.tar -C 2.5.2

# param 1: path to the folder where the target files are stored
# param 2: path to store the generated file list
bash generateFileList.sh 2.5.2 2.5.2.fileList
```

**Step 4:** Perform chunking on each file of the plain snapshot and fake offers, which will output the chunk metadata list.

```txt
cd Simulator/simCode
make clean
make
# param 1: file list of target snapshots/fakeOffers (generated by steps 2 and 3); use 2.5.2.fileList and fakeFile.fileList as an example
# Function: chunking will perform file-by-file chunking according to the file list, and generate relevant information about all chunks contained in the corresponding file list.
./chunking 2.5.2.fileList
./chunking fakeFile.fileList
```

An example of the list of chunk metadata (`2.5.2.fileList.chunkInfo`) is shown below. The metadata contains the "Chunk ID, Chunk Hash, Chunk Size, Feature s=1, Feature s=2, Feature s=3, Indicator s=1, Indicator s=2, Indicator s=3" of each chunk.

```txt
0
5717E7C840171019A4EEAB5B79A7F894A4986EAFF93D04EC5B12C9A189F594BF
4
2B32DB6C2C0A6235FB1397E8225EA85E0F0E6E8C7B126D0016CCBDE0E667151E
2EEB74A6177F588D80C0C752B99556902DDF9682D0B906F5AA2ADBAF8466A4E9
A9CD9F5FD05ADC7A088CC6DDC45E86C55B8E776072B3C53612577A2209B59DB6
CF05C45E104F5C1CA51C3C175614465D5348735D19D0713665E97BBF0CB756BA
263E85D62E8A3AE52CFECB3E245DB0731361C3B38EF27DCA67114E44A14547EF
40F31DC7243AA48D460A04989CA2AEA1614E7C6F3272D1E6EB4031E3BB2E44C4
...
```

**Step 5:** Randomly insert the faked offers into the target snapshot to form an attack snapshot. Generate the chunk metadata list corresponding to the mixed snapshot based on `*.fileList` and `*.chunkInfo`.

```txt
# param 1: file list of inserted snapshots (generated by step 2); use `2.5.2.fileList` as an example
# param 2: file list of fake offers (generated by step 3); use `fakeFile.fileList` as an example
# Function: Mix files from the original snapshot and fake offers, yield `2.5.2.fileList.mixed.chunkInfo`
./mixedFilesGen 2.5.2.fileList fakeFile.fileList
```

**Step 6:** Detect the learning-content attack based on generated chunks' metadata list.

```txt
# param 1: Chunk info list `*.chunkInfo` generated by chunking or mixedFilesGen; use 2.5.2.fileList.mixed.chunkInfo as an example
# param 2: Window size (W); use the default window size W=16000 as an example
# param 3: Redirect `windowCheck`'s stdout to the specified file for recording the maximum frequency of each window under the different schemes; use result-mixed.csv as an example
# Function: process each window to count the max frequency of each window.
./windowCheck 2.5.2.fileList.mixed.chunkInfo 16000 > result-mixed.csv

# param 1: window check result generated by `windowCheck`; use result-mixed.csv as an example
# param 2: Redirect processWindowCheckResult's stdout to the specified file for recording the maximum frequency in the target snapshot under the different schemes (use the Indicator); use window-mixed.csv as an example
# Function: process each window to count the max frequency of the snapshot.
./processWindowCheckResult result-mixed.csv >> window-mixed.csv
```

The `window-mixed.csv` file records a similar frequency in the window with the highest frequency in the target snapshot in the case of s=1, s=2, and s=3. The format of the content is as follows:

```txt
0.1860000000 , 0.1860000000 , 0.1860000000
```

You can directly compare the value with your preset threshold, and when the value is greater than the threshold, determine that `FeatureSpy` detects an attack in the snapshot. In addition, you can use `genDetectionRate` as follows, and the output will give the detection rate of different schemes.

```txt
# param 1: Processed window check result generated by processWindowCheckResult; use window-mixed.csv as an example
# param 2: Threshold T
./genDetectionRate window-mixed.csv 0.01
# Output:
Detection ratio with 1 feature = 1.0000000000
Detection ratio with 2 feature = 1.0000000000
Detection ratio with 3 feature = 1.0000000000
```


**Step 7 (optional):** If you want to test whether `FeatureSpy` produces false positives on raw snapshots without inserting faked offers, you need to run `windowCheck`, `processWindowCheckResult`, and `genDetectionRate` directly on the chunking result of "2.5.2.fileList" to get the result as in Step 6. Note that the detection ratio here corresponds to the false positive of `FeatureSpy`.

```txt
./windowCheck 2.5.2.fileList.chunkInfo 16000 > result-raw.csv
./processWindowCheckResult result-raw.csv > window-raw.csv
./genDetectionRate window-raw.csv 0.01
# Output:
Detection ratio with 1 feature = 0.0000000000
Detection ratio with 2 feature = 0.0000000000
Detection ratio with 3 feature = 0.0000000000
```