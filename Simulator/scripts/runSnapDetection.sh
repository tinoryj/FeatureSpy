#!/bin/bash
targetRunningTimes=100 # Total random tests number
zipfA='1.0'
chunkSize="8192" # Uint: Byte
snapSize="1024" # Unit: MiB
windowsSet=('16000') # 'w' in paper 
modifyPosSet=(2 3 4 5) # 'x' in paper 
modifylengthSet=('2' '4' '8' '16' '32' '64') # 'y' in paper 
modifyTimesSet=(1024 1536 2048 2560 3072) # 'n' in paper 

rm -rf SYNDetectionResults
mkdir SYNDetectionResults
rm -rf SYNDetectionRuntime
mkdir SYNDetectionRuntime
cd SYNDetectionRuntime
cp ../../simCode/* ./
make

python3 GenZipf.py -o syn_zipf_${zipfA} -c ${zipfA} -s ${snapSize}
./baselineSnapGen syn_zipf_${zipfA} ${chunkSize}

for modifyPos in ${modifyPosSet[@]}; do
    for modifylength in ${modifylengthSet[@]}; do
        for modifyTimes in ${modifyTimesSet[@]}; do
            for randomSeed in $(seq 1 $targetRunningTimes); do
                echo "Modify pos = ${modifyPos}; Modify length = ${modifylength}; modify file number = ${modifyTimes}; random seed = ${randomSeed}."
                ./mixedFilesGenSYN syn_zipf_${zipfA}.fileInfo syn_zipf_${zipfA}.chunkInfo ${chunkSize} ${modifyPos} ${modifylength} ${modifyTimes} ${randomSeed}
                target="${chunkSize}-${modifyPos}-${modifylength}-${modifyTimes}-${randomSeed}.chunkInfo"
                # ./snapCheck ${target} > "./syn_zipf_${zipfA}-Chunk-${chunkSize}-Pos-${modifyPos}-Len-${modifylength}-Number-${modifyTimes}-Seed-${randomSeed}-mixed.csv"
                for windowSize in ${windowsSet[@]}; do
                    ./windowCheck ${target} $windowSize > "./syn_zipf_${zipfA}-Chunk-${chunkSize}-Pos-${modifyPos}-Len-${modifylength}-Number-${modifyTimes}-Seed-${randomSeed}-mixed-Window-${windowSize}.csv"
                done
                rm -rf ${target}
            done
        done
    done
done

for modifyPos in ${modifyPosSet[@]}; do
    for modifylength in ${modifylengthSet[@]}; do
        for modifyTimes in ${modifyTimesSet[@]}; do
            echo "${modifyPos}-${modifylength}-${modifyTimes}"
            for randomSeed in $(seq 1 $targetRunningTimes); do
                for windowSize in ${windowsSet[@]}; do
                    if [ -f "./syn_zipf_${zipfA}-Chunk-${chunkSize}-Pos-${modifyPos}-Len-${modifylength}-Number-${modifyTimes}-Seed-${randomSeed}-mixed-Window-${windowSize}.csv" ]; then
                        ./processWindowCheckResult ./syn_zipf_${zipfA}-Chunk-${chunkSize}-Pos-${modifyPos}-Len-${modifylength}-Number-${modifyTimes}-Seed-${randomSeed}-mixed-Window-${windowSize}.csv ${windowSize} >> ../SYNDetectionResults/Detection-x-${modifyPos}-y-${modifylength}-n-${modifyTimes}-Window-${windowSize}.csv
                    fi
                done
            done
        done
    done
done

cp genDetectionRate ../SYNDetectionResults/
