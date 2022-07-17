#!/bin/bash
resultPath="./falsePositive"
modifylengthSet=('1' '2' '4' '8' '16' '32' '64')
zipfSet=('1.0')
snapSizeSet=('1024')
# zipfSet=('0.8' '0.85' '0.9' '0.95' '1.0' '1.05' '1.1' '1.15' '1.2')
swapRatioSet=('0.01' '0.0125' '0.015' '0.0175' '0.02')
windowsSet=('16000')
# windowsSet=('1000' '4000' '16000' '64000' '256000')
modifyPosSet=('1' '2' '3' '4' '5' '6' '7' '8')
chunkSize="8192"

for zipfA in ${zipfSet[@]}; do
        for randomSeed in $(seq 1 100); do
            echo "Random seed : $randomSeed"
            for modifyPos in ${modifyPosSet[@]}; do
                for modifylength in ${modifylengthSet[@]}; do
                    for swapRatio in ${swapRatioSet[@]}; do
                        target="syn_zipf_${zipfA}-Seed-${randomSeed}-Ratio-${swapRatio}-Pos-${modifyPos}-Len-${modifylength}"
                        for windowSize in ${windowsSet[@]}; do
                            ./processWindow ${resultPath}/"${windowSize}-${target}-swaped-logical.csv" ${targetPrefixID} ${windowSize} >> merged_syn_zipf_${zipfA}-Pos-${modifyPos}-Len-${modifylength}-Times-${swapRatio}-Window-${windowSize}.csv
                        done 
                    done
                done
            done
        done
done