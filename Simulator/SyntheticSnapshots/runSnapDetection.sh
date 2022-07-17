#!/bin/bash
zipfA='1.0'
chunkSize="8192"
snapSize="1024"
windowsSet=('16000')
modifyPosSet=(2 3 4 5)
modifylengthSet=('2' '4' '8' '16' '32' '64')
modifyTimesSet=(1024 1536 2048 2560 3072)

for zipfA in ${zipfSet[@]}; do
    python3 GenZipf.py -o syn_zipf_${zipfA} -c ${zipfA} -s ${snapSize}
    ./snapGen syn_zipf_${zipfA} ${chunkSize}
    for randomSeed in $(seq 1 10); do
        echo "Random seed : $randomSeed"
        for modifyPos in ${modifyPosSet[@]}; do
            for modifylength in ${modifylengthSet[@]}; do
                for modifyTimes in ${modifyTimesSet[@]}; do
                    let targetFileNumber=modifyTimes/modifyPos
                    echo $targetFileNumber
                    echo "${modifyPos}-${modifylength}-${modifyTimes}-${randomSeed}"
                    ./mixedChunksGen syn_zipf_${zipfA}.fileInfo syn_zipf_${zipfA}.chunkInfo ${chunkSize} ${modifyPos} ${modifylength} ${targetFileNumber} ${randomSeed}
                    target="${chunkSize}-${modifyPos}-${modifylength}-${modifyTimes}-${randomSeed}.chunkInfo"
                    ./snapCheck ${target} > "syn_zipf_${zipfA}-Chunk-${chunkSize}-Pos-${modifyPos}-Len-${modifylength}-Number-${modifyTimes}-Seed-${randomSeed}-mixed.csv"
                    for windowSize in ${windowsSet[@]}; do
                        ./windowCheck ${target} $windowSize > "syn_zipf_${zipfA}-Chunk-${chunkSize}-Pos-${modifyPos}-Len-${modifylength}-Number-${modifyTimes}-Seed-${randomSeed}-mixed-Window-${windowSize}.csv"
                    done
                    rm -rf ${target}
                done
            done
        done
    done
done