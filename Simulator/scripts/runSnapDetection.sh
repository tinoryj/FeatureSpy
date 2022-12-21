#!/bin/bash
targetRunningTimes=10 # Total random tests number
zipfA='1.0' # The zipf distribution parameter a.
chunkSize="8192" # Uint: Byte. The target chunk size.
snapSize="1024" # Unit: MiB. The snapshot size.
windowsSet=('16000') # 'w' in paper. Use 'value' 'value' 'value'... to fill in the form, please refer to the text of the paper for specific values.
modifyPosSet=(2) # 'x' in paper. Use value value value... to fill in the form, please refer to the text of the paper for specific values.
modifylengthSet=('2') # 'y' in paper . Use 'value' 'value' 'value'... to fill in the form, please refer to the text of the paper for specific values.
modifyTimesSet=(1024) # 'n' in paper. Use value value value... to fill in the form, please refer to the text of the paper for specific values.

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

echo "Process results:"
for modifyPos in ${modifyPosSet[@]}; do
    for modifylength in ${modifylengthSet[@]}; do
        for modifyTimes in ${modifyTimesSet[@]}; do
            echo "Process result for x=${modifyPos},y=${modifylength},n=${modifyTimes}"
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
cd ../SYNDetectionResults
echo "---------- Results ----------"
echo "Output results:"
for modifyPos in ${modifyPosSet[@]}; do
    for modifylength in ${modifylengthSet[@]}; do
        for modifyTimes in ${modifyTimesSet[@]}; do
            target="Snapshot size = ${snapSize} MiB, fake file number (n) = ${modifyTimes}, modify pos (x) = ${modifyPos}, modify length (y) = ${modifylength}"
            echo $target
            for windowSize in ${windowsSet[@]}; do
                ./genDetectionRate Detection-x-${modifyPos}-y-${modifylength}-n-${modifyTimes}-Window-${windowSize}.csv 0.01
            done 
        done
    done
done