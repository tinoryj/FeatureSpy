#!/bin/bash
windows=('16000')
datasetPath='../../traceDownload/packed-gcc/'
dataSet=('gcc-4.0.0')
#  'gcc-4.0.1' 'gcc-4.0.2' 'gcc-4.0.3' 'gcc-4.0.4' 'gcc-4.1.0' 'gcc-4.1.1' 'gcc-4.1.2' 'gcc-4.2.0' 'gcc-4.2.1' 'gcc-4.2.2' 'gcc-4.2.3' 'gcc-4.2.4' 'gcc-4.3.0' 'gcc-4.3.1' 'gcc-4.3.2' 'gcc-4.3.3' 'gcc-4.3.4' 'gcc-4.3.5' 'gcc-4.3.6' 'gcc-4.4.0' 'gcc-4.4.1' 'gcc-4.4.2' 'gcc-4.4.3' 'gcc-4.4.4' 'gcc-4.4.5' 'gcc-4.4.6' 'gcc-4.4.7' 'gcc-4.5.0' 'gcc-4.5.1' 'gcc-4.5.2' 'gcc-4.5.3' 'gcc-4.5.4' 'gcc-4.6.0' 'gcc-4.6.1' 'gcc-4.6.2' 'gcc-4.6.3' 'gcc-4.6.4' 'gcc-4.7.0' 'gcc-4.7.1' 'gcc-4.7.2' 'gcc-4.7.3' 'gcc-4.7.4' 'gcc-4.8.0' 'gcc-4.8.1' 'gcc-4.8.2' 'gcc-4.8.3' 'gcc-4.8.4' 'gcc-4.8.5' 'gcc-4.9.0' 'gcc-4.9.1' 'gcc-4.9.2' 'gcc-4.9.3' 'gcc-4.9.4' 'gcc-5.1.0' 'gcc-5.2.0' 'gcc-5.3.0' 'gcc-5.4.0' 'gcc-5.5.0' 'gcc-6.1.0' 'gcc-6.2.0' 'gcc-6.3.0' 'gcc-6.4.0' 'gcc-6.5.0' 'gcc-7.1.0' 'gcc-7.2.0' 'gcc-7.3.0' 'gcc-7.4.0' 'gcc-7.5.0' 'gcc-8.1.0' 'gcc-8.2.0' 'gcc-8.3.0' 'gcc-8.4.0' 'gcc-8.5.0' 'gcc-9.1.0' 'gcc-9.2.0' 'gcc-9.3.0' 'gcc-9.4.0' 'gcc-10.1.0' 'gcc-10.2.0' 'gcc-10.3.0' 'gcc-11.1.0' 'gcc-11.2.0' 'gcc-11.3.0' 'gcc-12.1.0' )

rm -rf SnapshotBasedPrefixDistribution-GCC.csv
rm -rf mixedFakeOffersPrefixFreq-GCC-WindowSize-${window}.csv
rm -rf orignalPrefixFreq-GCC-WindowSize-${window}.csv

rm -rf gccCaseStudyResult
mkdir gccCaseStudyResult
rm -rf gccCaseStudyRuntime
mkdir gccCaseStudyRuntime
cd gccCaseStudyRuntime
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
    mkdir ${target}
    unzip -d ./${target} ${datasetPath}${target}.zip
    read_dir ${target} >> ${target}.fileList
    ./chunking ${target}.fileList
    ./snapCheck ${target}.fileList.chunkInfo > "./${target}-origin-logical.csv"
    for windowSize in ${windows[@]}; do
        ./windowCheck ${target}.fileList.chunkInfo $windowSize> "./${target}-origin-${windowSize}-logical.csv"
    done
    rm -rf ${target}
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
    rm -rf ${datasetPath}${target}.fileList
    rm -rf ${target}.fileList.mixed.*
done

for target in ${dataSet[@]}; do
    ./processWindowCheckResult ./${target}-origin-logical.csv >> ../gccCaseStudyResult/SnapshotBasedPrefixDistribution-GCC.csv
    for window in ${windowSize[@]}; do
        ./processWindowCheckResult "./${target}-mixed-${window}-logical.csv" >> ../gccCaseStudyResult/mixedFakeOffersPrefixFreq-GCC-WindowSize-${window}.csv
        ./processWindowCheckResult "./${target}-origin-${window}-logical.csv" >> ../gccCaseStudyResult/orignalPrefixFreq-GCC-WindowSize-${window}.csv
    done
done

cp genDetectionRate ../gccCaseStudyResult/
cd ../gccCaseStudyResult/
echo "---------- Results ----------"
echo "Output results:"
echo "Detection ratio with GCC dataset:"
./genDetectionRate mixedFakeOffersPrefixFreq-GCC-WindowSize-16000.csv 0.01
echo "False positive with GCC dataset:"
./genDetectionRate orignalPrefixFreq-GCC-WindowSize-16000.csv 0.01