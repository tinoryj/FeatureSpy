#!/bin/bash
windows=('16000')
datasetPath='../../traceDownload/packed-linux/'
dataSet=('v2.6.11' 'v2.6.12' 'v2.6.13' 'v2.6.14' 'v2.6.15' 'v2.6.16' 'v2.6.17' 'v2.6.18' 'v2.6.19' 'v2.6.20' 'v2.6.21' 'v2.6.22'  'v2.6.23' 'v2.6.24' 'v2.6.25' 'v2.6.26' 'v2.6.27' 'v2.6.28' 'v2.6.29' 'v2.6.30' 'v2.6.31' 'v2.6.32' 'v2.6.33' 'v2.6.34' 'v2.6.35' 'v2.6.36' 'v2.6.37' 'v2.6.38' 'v2.6.39' 'v3.0' 'v3.1' 'v3.2' 'v3.3' 'v3.4' 'v3.5' 'v3.6' 'v3.7' 'v3.8' 'v3.9' 'v3.10' 'v3.11' 'v3.12' 'v3.13' 'v3.14' 'v3.15' 'v3.16' 'v3.17' 'v3.18' 'v3.19' 'v4.0' 'v4.1' 'v4.2' 'v4.3' 'v4.4' 'v4.5' 'v4.6' 'v4.7' 'v4.8' 'v4.9' 'v4.10' 'v4.11' 'v4.12' 'v4.13' 'v4.14' 'v4.15' 'v4.16' 'v4.17' 'v4.18' 'v4.19' 'v4.20' 'v5.0' 'v5.1' 'v5.2' 'v5.3' 'v5.4' 'v5.5' 'v5.6' 'v5.7' 'v5.8' 'v5.9' 'v5.10' 'v5.11' 'v5.12' 'v5.13')

rm -rf SnapshotBasedPrefixDistribution-Linux.csv
rm -rf mixedFakeOffersPrefixFreq-Linux-WindowSize-${window}.csv
rm -rf orignalPrefixFreq-Linux-WindowSize-${window}.csv

rm -rf linuxCaseStudyResult
mkdir linuxCaseStudyResult
rm -rf linuxCaseStudyRuntime
mkdir linuxCaseStudyRuntime
cd linuxCaseStudyRuntime
cp ../../simCode/* ./
make

read_dir(){
    for file in `ls -a $1`
    do
        if [ -d $1"/"$file ]
        then
            if [[ $file != '.' && $file != '..' ]]
            then
                read_dir $1"/"$file
            fi
        else
            echo `realpath $1"/"$file`
        fi
    done
}

for target in ${dataSet[@]}; do
    rm -rf ${target}.fileList
    mkdir -p ${target}
    unzip -d ./${target} ${datasetPath}${target}.zip
    read_dir ${target} >> ${target}.fileList
    ./chunking ${target}.fileList
    ./snapCheck ${target}.fileList.chunkInfo > "./${target}-origin-logical.csv"
    for windowSize in ${windows[@]}; do
        ./windowCheck ${target}.fileList.chunkInfo $windowSize> "./${target}-origin-${windowSize}-logical.csv"
    done
    rm -rf ${target}
    rm -rf ${target}.tar
done

cp -r ../../fakeOfferGen ./
cd fakeOfferGen
./generateFakeOffers.sh
./generateFileList.sh ./result fakeFile.fileList
cp ./fakeFile.fileList ../
cd ..
./chunking fakeFile.fileList
for target in ${dataSet[@]}; do
    ./mixedFilesGen ${target}.fileList fakeFile.fileList
    ./snapCheck ${target}.fileList.mixed.chunkInfo > "./${target}-mixed-logical.csv"
    for windowSize in ${windows[@]}; do
        ./windowCheck ${target}.fileList.mixed.chunkInfo $windowSize> "./${target}-mixed-${windowSize}-logical.csv"
    done
    rm -rf ${target}.fileList
    rm -rf ${target}.fileList.mixed.*
done

for target in ${dataSet[@]}; do
    ./processWindowCheckResult ./${target}-origin-logical.csv >> ../linuxCaseStudyResult/SnapshotBasedPrefixDistribution-Linux.csv
    for window in ${windowSize[@]}; do
        ./processWindowCheckResult "./${target}-mixed-${window}-logical.csv" >> ../linuxCaseStudyResult/mixedFakeOffersPrefixFreq-Linux-WindowSize-${window}.csv
        ./processWindowCheckResult "./${target}-origin-${window}-logical.csv" >> ../linuxCaseStudyResult/orignalPrefixFreq-Linux-WindowSize-${window}.csv
    done
done

cp genDetectionRate ../linuxCaseStudyResult/
cd ../linuxCaseStudyResult/
echo "---------- Results ----------"
echo "Output results:"
echo "Detection ratio with Linux dataset:"
./genDetectionRate mixedFakeOffersPrefixFreq-Linux-WindowSize-16000.csv 0.01
echo "False positive with Linux dataset:"
./genDetectionRate orignalPrefixFreq-Linux-WindowSize-16000.csv 0.01