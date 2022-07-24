#!/bin/bash
git clone https://github.com/torvalds/linux.git
targetGit="./linux"
mkdir packed-linux
cd $targetGit
git tag > ../tags-linux.log
tagListSize=`sed -n '$=' ../tags-linux.log`
echo $tagListSize
for index in `seq 1 $tagListSize`; do
    tag=`cat ../tags-linux.log | sed -n "${index},${index}p"`
    echo $tag
    # git archive --format=zip --output=../packed-linux/$tag.zip $tag
    if [[ $tag = *rc* ]]
    then
        echo "skip rc"
    else 
            if [[ $tag = *tree* ]]
            then
                echo "skip tree"
            else 
                echo "Packing $tag"
                git archive --format=zip --output=../packed-linux/$tag.zip $tag
            fi
    fi
done