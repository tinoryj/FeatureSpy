#!/bin/bash
mkdir fakeFile
for ((index = 30; index < 61; index += 1)); do
    for ((i = 204; i < 810; i += 6)); do
        python modifyFile.py offer-base fakeFile/ $i ${index}
    done
done

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

rm -rf fakeFile.fileList
read_dir fakeFile >> fakeFile.fileList