#!/bin/bash
zipfA='1.0'
snapSizeSet=('1024')
swapRatioSet=('0.01' '0.0125' '0.015' '0.0175' '0.02')
windowsSet=('16000')
modifyPosSet=('2' '3' '4' '5')
modifylengthSet=('2' '4' '8' '16' '32' '64')
chunkSize="8192"

for snapSize in ${snapSizeSet[@]}; do
    python3 GenZipf.py -o syn_zipf_${zipfA} -c ${zipfA}  -s ${snapSize}
    ./snapGen syn_zipf_${zipfA} ${chunkSize}
    for randomSeed in $(seq 1 10); do
        echo "Random seed : $randomSeed"
        for modifyPos in ${modifyPosSet[@]}; do
            for modifylength in ${modifylengthSet[@]}; do
                for swapRatio in ${swapRatioSet[@]}; do
                    echo "${modifyPos}-${modifylength}-${randomSeed}-${swapRatio}"
                    ./swapSnap syn_zipf_${zipfA} ${randomSeed} ${swapRatio} ${chunkSize} ${modifyPos} ${modifylength}
                    target="syn_zipf_${zipfA}-Seed-${randomSeed}-Ratio-${swapRatio}-Pos-${modifyPos}-Len-${modifylength}"
                    ./snapCheck syn_zipf_${zipfA}.swap.chunkInfo > "${target}-swaped-logical.csv"
                    for windowSize in ${windowsSet[@]}; do
                        ./windowCheck syn_zipf_${zipfA}.swap.chunkInfo $windowSize > "${windowSize}-${target}-swaped-logical.csv"
                    done 
                    rm -rf syn_zipf_${zipfA}.swap.chunkInfo
                done
            done
        done
    done
done