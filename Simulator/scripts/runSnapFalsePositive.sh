#!/bin/bash
targetRunningTimes=10 # Total random tests number
zipfA='1.0' # The zipf distribution parameter a.
chunkSize="8192" # Uint: Byte. The target chunk size.
snapSize="1024" # Unit: MiB. The snapshot size.
swapRatioSet=('0.01') # 'r' in paper. Use 'value' 'value' 'value'... to fill in the form, please refer to the text of the paper for specific values. 
windowsSet=('16000') # 'w' in paper. Use 'value' 'value' 'value'... to fill in the form, please refer to the text of the paper for specific values. 
modifyPosSet=('2') # 'x' in paper. Use 'value' 'value' 'value'... to fill in the form, please refer to the text of the paper for specific values. 
modifylengthSet=('2') # 'y' in paper. Use 'value' 'value' 'value'... to fill in the form, please refer to the text of the paper for specific values. 


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
                ./swappedSnapGen syn_zipf_${zipfA} ${chunkSize} ${modifyPos} ${modifylength} ${swapRatio} ${randomSeed}  
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

echo "Process results:"
for modifyPos in ${modifyPosSet[@]}; do
    for modifylength in ${modifylengthSet[@]}; do
        for swapRatio in ${swapRatioSet[@]}; do
            echo "Process result for x=${modifyPos},y=${modifylength},r=${swapRatio}"
            for randomSeed in $(seq 1 $targetRunningTimes); do
                target="syn_zipf_${zipfA}-Seed-${randomSeed}-Ratio-${swapRatio}-Pos-${modifyPos}-Len-${modifylength}"
                for windowSize in ${windowsSet[@]}; do
                    ./processWindowCheckResult "./${windowSize}-${target}-swaped-logical.csv">> ../SYNFalseResults/FalsePositive-x-${modifyPos}-y-${modifylength}-r-${swapRatio}-Window-${windowSize}.csv
                done 
            done
        done
    done
done

cp genDetectionRate ../SYNFalseResults/
cd ../SYNFalseResults
echo "---------- Results ----------"
echo "Output results:"
for modifyPos in ${modifyPosSet[@]}; do
    for modifylength in ${modifylengthSet[@]}; do
        for swapRatio in ${swapRatioSet[@]}; do
            target="Snapshot size = ${snapSize} MiB, swapped ratio (r) = ${swapRatio}, modify pos (x) = ${modifyPos}, modify length (y) = ${modifylength}"
            echo $target
            for windowSize in ${windowsSet[@]}; do
                ./genDetectionRate FalsePositive-x-${modifyPos}-y-${modifylength}-r-${swapRatio}-Window-${windowSize}.csv 0.01
            done 
        done
    done
done