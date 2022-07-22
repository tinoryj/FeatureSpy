#!/bin/bash
git clone https://github.com/torvalds/linux.git
targetGit="./linux"
mkdir linux-packed
cd $targetGit
git tag > ../tags-linux.log
tagListSize=`sed -n '$=' ../tags-linux.log`
echo $tagListSize
for index in `seq 1 $tagListSize`; do
    tag=`cat ../tags-linux.log | sed -n "${index},${index}p"`
    echo $tag
    git archive --format=zip --output=../linux-packed/$tag.zip $tag
done