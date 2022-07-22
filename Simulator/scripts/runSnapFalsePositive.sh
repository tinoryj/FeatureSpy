#!/bin/bash
targetRunningTimes=1 # Total random tests number
zipfA='1.0'
snapSize='1024' # Unit: MiB
swapRatioSet=('0.01' '0.0125' '0.015' '0.0175' '0.02') # 'r' in paper 
windowsSet=('16000') # 'w' in paper 
modifyPosSet=('2' '3' '4' '5') # 'x' in paper 
modifylengthSet=('2' '4' '8' '16' '32' '64') # 'y' in paper 
chunkSize="8192" # Unit: Byte

rm -rf SYNFalseResults
mkdir SYNFalseResults
rm -rf SYNFalseRuntime
mkdir SYNFalseRuntime
cd SYNFalseRuntime
cp ../../simCode/* ./
make

python3 GenZipf.py -o syn_zipf_${zipfA} -c ${zipfA}  -s ${snapSize}
./baselineSnapGen syn_zipf_${zipfA} ${chunkSize}

for modifyPos in ${modifyPosSet[@]}; do
    for modifylength in ${modifylengthSet[@]}; do
        for swapRatio in ${swapRatioSet[@]}; do
            for randomSeed in $(seq 1 $targetRunningTimes); do
                echo "x = ${modifyPos}; y = ${modifylength}; r = ${swapRatio}; random seed = ${randomSeed}."
                ./swappedSnapGen syn_zipf_${zipfA} ${randomSeed} ${swapRatio} ${chunkSize} ${modifyPos} ${modifylength}
                target="syn_zipf_${zipfA}-Seed-${randomSeed}-Ratio-${swapRatio}-Pos-${modifyPos}-Len-${modifylength}"
                # ./snapCheck syn_zipf_${zipfA}.swap.chunkInfo > "./${target}-swaped-logical.csv"
                for windowSize in ${windowsSet[@]}; do
                    ./windowCheck syn_zipf_${zipfA}.swap.chunkInfo $windowSize > "./${windowSize}-${target}-swaped-logical.csv"
                done 
                rm -rf syn_zipf_${zipfA}.swap.chunkInfo
            done
        done
    done
done

    
for modifyPos in ${modifyPosSet[@]}; do
    for modifylength in ${modifylengthSet[@]}; do
        for swapRatio in ${swapRatioSet[@]}; do
            for randomSeed in $(seq 1 $targetRunningTimes); do
                target="syn_zipf_${zipfA}-Seed-${randomSeed}-Ratio-${swapRatio}-Pos-${modifyPos}-Len-${modifylength}"
                for windowSize in ${windowsSet[@]}; do
                    ./processWindowCheckResult "./${windowSize}-${target}-swaped-logical.csv" ${targetPrefixID} >> ../SYNFalseResults/FalsePositive-x-${modifyPos}-y-${modifylength}-r-${swapRatio}-Window-${windowSize}.csv
                done 
            done
        done
    done
done

cp genDetectionRate ../SYNFalseResults/