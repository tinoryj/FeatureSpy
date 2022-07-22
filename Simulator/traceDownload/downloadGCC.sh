#!/bin/bash
git clone git://gcc.gnu.org/git/gcc.git
targetGit="./gcc"
mkdir gcc-packed/releases
cd $targetGit
git tag > ../tags-gcc.log
tagListSize=`sed -n '$=' ../tags-gcc.log`
echo $tagListSize
for index in `seq 1 $tagListSize`; do
    tag=`cat ../tags-gcc.log | sed -n "${index},${index}p"`
    if [[ $tag = *prereleases/gcc* ]]
    then
        echo "skip pre-releases"
    else 
        if [[ $tag = *releases/gcc* ]]
        then
            echo $tag
            git archive --format=zip --output=../gcc-packed/$tag.zip $tag
        fi
    fi
done
cd ..
mv gcc-packed/releases/* gcc-packed/
rm -rf gcc-packed/releases