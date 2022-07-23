#!/bin/bash

read_dir() {
    for file in $(ls -a $1); do
        if [ -d $1"/"$file ]; then
            if [[ $file != '.' && $file != '..' ]]; then
                read_dir $1"/"$file
            fi
        else
            echo $(realpath $1"/"$file)
        fi
    done
}

rm -rf $1.fileList
read_dir $1 >>$2
