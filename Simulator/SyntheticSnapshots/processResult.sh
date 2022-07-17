#!/bin/bash
resultPath="./Detection"
zipfA='1.0'
targetPrefixID="1" # 0-16B,1-32B,2-48B,3-64B
chunkSize="8192"
windowsSet=('16000')
modifyPosSet=('2' '3' '4' '5')
modifylengthSet=('2' '4' '8' '16' '32' '64')
modifyTimesSet=('1024' '1536' '2048' '2560' '3072')

for modifyPos in ${modifyPosSet[@]}; do
    for modifylength in ${modifylengthSet[@]}; do
        for modifyTimes in ${modifyTimesSet[@]}; do
            echo "${modifyPos}-${modifylength}-${modifyTimes}"
            for randomSeed in $(seq 1 100); do
                for windowSize in ${windowsSet[@]}; do
                    if [ -f "${resultPath}/syn_zipf_${zipfA}-Chunk-${chunkSize}-Pos-${modifyPos}-Len-${modifylength}-Number-${modifyTimes}-Seed-${randomSeed}-mixed-Window-${windowSize}.csv" ]; then
                        ./processWindow ${resultPath}/syn_zipf_${zipfA}-Chunk-${chunkSize}-Pos-${modifyPos}-Len-${modifylength}-Number-${modifyTimes}-Seed-${randomSeed}-mixed-Window-${windowSize}.csv ${windowSize} >> merged_syn_zipf_${zipfA}-Chunk-${chunkSize}-Pos-${modifyPos}-Len-${modifylength}-Number-${modifyTimes}-Window-${windowSize}.csv
                    fi
                done
            done
        done
    done
done