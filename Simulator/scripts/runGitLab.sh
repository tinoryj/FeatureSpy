#!/bin/bash
windows=('16000')
datasetPath='../../traceDownload/packed-gitlab/'
dataSet=('14.0.0-ce.0' '14.0.1-ce.0' '14.0.10-ce.0' '14.0.11-ce.0' '14.0.12-ce.0' '14.0.2-ce.0' '14.0.3-ce.0' '14.0.4-ce.0' '14.0.5-ce.0' '14.0.6-ce.0' '14.0.7-ce.0' '14.0.8-ce.0' '14.0.9-ce.0' '14.1.0-ce.0' '14.1.1-ce.0' '14.1.2-ce.0' '14.1.3-ce.0' '14.1.4-ce.0' '14.1.5-ce.0' '14.1.6-ce.0' '14.1.7-ce.0' '14.1.8-ce.0' '14.10.0-ce.0' '14.10.1-ce.0' '14.10.2-ce.0' '14.2.0-ce.0' '14.2.1-ce.0' '14.2.2-ce.0' '14.2.3-ce.0' '14.2.4-ce.0' '14.2.5-ce.0' '14.2.6-ce.0' '14.2.7-ce.0' '14.3.0-ce.0' '14.3.1-ce.0' '14.3.2-ce.0' '14.3.3-ce.0' '14.3.4-ce.0' '14.3.5-ce.0' '14.3.6-ce.0' '14.4.0-ce.0' '14.4.1-ce.0' '14.4.2-ce.0' '14.4.3-ce.0' '14.4.4-ce.0' '14.4.5-ce.0' '14.5.0-ce.0' '14.5.1-ce.0' '14.5.2-ce.0' '14.5.3-ce.0' '14.5.4-ce.0' '14.6.0-ce.0' '14.6.1-ce.0' '14.6.2-ce.0' '14.6.3-ce.0' '14.6.4-ce.0' '14.6.5-ce.0' '14.6.6-ce.0' '14.6.7-ce.0' '14.7.0-ce.0' '14.7.1-ce.0' '14.7.2-ce.0' '14.7.3-ce.0' '14.7.4-ce.0' '14.7.5-ce.0' '14.7.6-ce.0' '14.7.7-ce.0' '14.8.0-ce.0' '14.8.1-ce.0' '14.8.2-ce.0' '14.8.3-ce.0' '14.8.4-ce.0' '14.8.5-ce.0' '14.8.6-ce.0' '14.9.0-ce.0' '14.9.1-ce.0' '14.9.2-ce.0' '14.9.3-ce.0' '14.9.4-ce.0')

rm -rf SnapshotBasedPrefixDistribution-Gitlab.csv
rm -rf mixedFakeOffersPrefixFreq-Gitlab-WindowSize-${window}.csv
rm -rf orignalPrefixFreq-Gitlab-WindowSize-${window}.csv

rm -rf gitlabCaseStudyResult
mkdir gitlabCaseStudyResult
rm -rf gitlabCaseStudyRuntime
mkdir gitlabCaseStudyRuntime
cd gitlabCaseStudyRuntime
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
    ./processWindowCheckResult ./${target}-origin-logical.csv >> ../gitlabCaseStudyResult/SnapshotBasedPrefixDistribution-Gitlab.csv
    for window in ${windowSize[@]}; do
        ./processWindowCheckResult "./${target}-mixed-${window}-logical.csv" >> ../gitlabCaseStudyResult/mixedFakeOffersPrefixFreq-Gitlab-WindowSize-${window}.csv
        ./processWindowCheckResult "./${target}-origin-${window}-logical.csv" >> ../gitlabCaseStudyResult/orignalPrefixFreq-Gitlab-WindowSize-${window}.csv
    done
done

cp genDetectionRate ../gitlabCaseStudyResult/
cd ../gitlabCaseStudyResult/
echo "---------- Results ----------"
echo "Output results:"
echo "Detection ratio with Gitlab dataset:"
./genDetectionRate mixedFakeOffersPrefixFreq-Gitlab-WindowSize-16000.csv 0.01
echo "False positive with Gitlab dataset:"
./genDetectionRate orignalPrefixFreq-Gitlab-WindowSize-16000.csv 0.01