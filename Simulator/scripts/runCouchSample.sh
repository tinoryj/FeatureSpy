#!/bin/bash
windows=('16000')
datasetPath='../../traceDownload/packed-couch/'
dataSet=('2.5.2')

rm -rf SnapshotBasedPrefixDistribution-Couch.csv
rm -rf mixedFakeOffersPrefixFreq-Couch-WindowSize-${window}.csv
rm -rf orignalPrefixFreq-Couch-WindowSize-${window}.csv

rm -rf couchCaseStudyResult
mkdir couchCaseStudyResult
rm -rf couchCaseStudyRuntime
mkdir couchCaseStudyRuntime
cd couchCaseStudyRuntime
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
    mkdir -p ${target}
    cp ${datasetPath}${target}.tar ./
    rm -rf ${target}.fileList
    tar -xvf ${target}.tar -C ${target}
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
echo "Generate fake offers"
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
    ./processWindowCheckResult ./${target}-origin-logical.csv >> ../couchCaseStudyResult/SnapshotBasedPrefixDistribution-Couch.csv
    for window in ${windowSize[@]}; do
        ./processWindowCheckResult "./${target}-mixed-${window}-logical.csv" >> ../couchCaseStudyResult/mixedFakeOffersPrefixFreq-Couch-WindowSize-${window}.csv
        ./processWindowCheckResult "./${target}-origin-${window}-logical.csv" >> ../couchCaseStudyResult/orignalPrefixFreq-Couch-WindowSize-${window}.csv
    done
done

cp genDetectionRate ../couchCaseStudyResult/
cd ../couchCaseStudyResult/
echo "---------- Results ----------"
echo "Output results:"
echo "Detection ratio with CouchDB dataset:"
./genDetectionRate mixedFakeOffersPrefixFreq-Couch-WindowSize-16000.csv 0.01
echo "False positive with CouchDB dataset:"
./genDetectionRate orignalPrefixFreq-Couch-WindowSize-16000.csv 0.01