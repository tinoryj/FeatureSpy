#!/bin/bash

prefixID=1
windowSize=('16000')

pathLinux="./LinuxResult"
pickedSetLinux=('v2.6.11' 'v2.6.12' 'v2.6.13' 'v2.6.14' 'v2.6.15' 'v2.6.16' 'v2.6.17' 'v2.6.18' 'v2.6.19' 'v2.6.20' 'v2.6.21' 'v2.6.22' 'v2.6.23' 'v2.6.24' 'v2.6.25' 'v2.6.26' 'v2.6.27' 'v2.6.28' 'v2.6.29' 'v2.6.30' 'v2.6.31' 'v2.6.32' 'v2.6.33' 'v2.6.34' 'v2.6.35' 'v2.6.36' 'v2.6.37' 'v2.6.38' 'v2.6.39' 'v3.0' 'v3.1' 'v3.2' 'v3.3' 'v3.4' 'v3.5' 'v3.6' 'v3.7' 'v3.8' 'v3.9' 'v3.10' 'v3.11' 'v3.12' 'v3.13' 'v3.14' 'v3.15' 'v3.16' 'v3.17' 'v3.18' 'v3.19' 'v4.0' 'v4.1' 'v4.2' 'v4.3' 'v4.4' 'v4.5' 'v4.6' 'v4.7' 'v4.8' 'v4.9' 'v4.10' 'v4.11' 'v4.12' 'v4.13' 'v4.14' 'v4.15' 'v4.16' 'v4.17' 'v4.18' 'v4.19' 'v4.20' 'v5.0' 'v5.1' 'v5.2' 'v5.3' 'v5.4' 'v5.5' 'v5.6' 'v5.7' 'v5.8' 'v5.9' 'v5.10' 'v5.11' 'v5.12' 'v5.13')

for target in ${pickedSetLinux[@]}; do
    cat $pathLinux/${target}-origin-logical.csv >> mergedSnapBased-Linux.csv
    for window in ${windowSize[@]}; do
        ./processWindow "$pathLinux/${target}-mixed-${window}-logical.csv" "$pathLinux/${target}-origin-${window}-logical.csv" ${prefixID} ${window} >> mergedLinux-${window}.csv
    done
done

pathCouch="./couchTarResult"
pickedSetCouch=('2.5.2' '3.0.2' '3.0.3' '3.1.0' '3.1.3' '3.1.5' '3.1.6' '4.0.0' '4.1.0' '4.1.1' '4.5.0' '4.5.1' '4.6.0' '4.6.1' '4.6.2' '4.6.3' '4.6.4' '4.6.5' '5.0.1' '5.1.0' '5.1.1' '5.5.0' '5.5.1' '5.5.2' '6.0.0' '6.0.1' '6.0.2' '6.0.3' '6.0.4' '6.0.5' '6.5.0' '6.5.1' '6.6.0' '6.6.1' '6.6.2' 'community-2.2.0' 'community-3.0.1' 'community-3.1.3' 'community-4.0.0' 'community-4.1.0' 'community-4.1.1' 'community-4.5.0' 'community-4.5.1' 'community-5.0.1' 'community-5.1.1' 'community-6.0.0' 'community-6.5.0' 'community-6.5.1' 'community-6.6.0' 'community' 'enterprise-2.5.2' 'enterprise-3.0.2' 'enterprise-3.0.3' 'enterprise-3.1.0' 'enterprise-3.1.3' 'enterprise-3.1.5' 'enterprise-3.1.6' 'enterprise-4.0.0' 'enterprise-4.1.0' 'enterprise-4.1.1' 'enterprise-4.5.0' 'enterprise-4.5.1' 'enterprise-4.6.0' 'enterprise-4.6.1' 'enterprise-4.6.2' 'enterprise-4.6.3' 'enterprise-4.6.4' 'enterprise-4.6.5' 'enterprise-5.0.1' 'enterprise-5.1.0' 'enterprise-5.1.1' 'enterprise-5.5.0' 'enterprise-5.5.1' 'enterprise-5.5.2' 'enterprise-6.0.0' 'enterprise-6.0.1' 'enterprise-6.0.2' 'enterprise-6.0.3' 'enterprise-6.0.4' 'enterprise-6.0.5' 'enterprise-6.5.0' 'enterprise-6.5.1' 'enterprise')

for target in ${pickedSetCouch[@]}; do
    cat $pathCouch/${target}-origin-logical.csv >> mergedSnapBased-Couch.csv
    for window in ${windowSize[@]}; do
        ./processWindow "$pathCouch/${target}-mixed-${window}-logical.csv" "$pathCouch/${target}-origin-${window}-logical.csv" ${prefixID} ${window} >> mergedCouch-${window}.csv
    done
done

pathGCC="./gccResult"
pickedSetGCC=('gcc-4.0.0' 'gcc-4.0.1' 'gcc-4.0.2' 'gcc-4.0.3' 'gcc-4.0.4' 'gcc-4.1.0' 'gcc-4.1.1' 'gcc-4.1.2' 'gcc-4.2.0' 'gcc-4.2.1' 'gcc-4.2.2' 'gcc-4.2.3' 'gcc-4.2.4' 'gcc-4.3.0' 'gcc-4.3.1' 'gcc-4.3.2' 'gcc-4.3.3' 'gcc-4.3.4' 'gcc-4.3.5' 'gcc-4.3.6' 'gcc-4.4.0' 'gcc-4.4.1' 'gcc-4.4.2' 'gcc-4.4.3' 'gcc-4.4.4' 'gcc-4.4.5' 'gcc-4.4.6' 'gcc-4.4.7' 'gcc-4.5.0' 'gcc-4.5.1' 'gcc-4.5.2' 'gcc-4.5.3' 'gcc-4.5.4' 'gcc-4.6.0' 'gcc-4.6.1' 'gcc-4.6.2' 'gcc-4.6.3' 'gcc-4.6.4' 'gcc-4.7.0' 'gcc-4.7.1' 'gcc-4.7.2' 'gcc-4.7.3' 'gcc-4.7.4' 'gcc-4.8.0' 'gcc-4.8.1' 'gcc-4.8.2' 'gcc-4.8.3' 'gcc-4.8.4' 'gcc-4.8.5' 'gcc-4.9.0' 'gcc-4.9.1' 'gcc-4.9.2' 'gcc-4.9.3' 'gcc-4.9.4' 'gcc-5.1.0' 'gcc-5.2.0' 'gcc-5.3.0' 'gcc-5.4.0' 'gcc-5.5.0' 'gcc-6.1.0' 'gcc-6.2.0' 'gcc-6.3.0' 'gcc-6.4.0' 'gcc-6.5.0' 'gcc-7.1.0' 'gcc-7.2.0' 'gcc-7.3.0' 'gcc-7.4.0' 'gcc-7.5.0' 'gcc-8.1.0' 'gcc-8.2.0' 'gcc-8.3.0' 'gcc-8.4.0' 'gcc-8.5.0' 'gcc-9.1.0' 'gcc-9.2.0' 'gcc-9.3.0' 'gcc-9.4.0' 'gcc-10.1.0' 'gcc-10.2.0' 'gcc-10.3.0' 'gcc-11.1.0' 'gcc-11.2.0' 'gcc-11.3.0' 'gcc-12.1.0')

for target in ${pickedSetGCC[@]}; do
    cat $pathGCC/${target}-origin-logical.csv >> mergedSnapBased-GCC.csv
    for window in ${windowSize[@]}; do
        ./processWindow "$pathGCC/${target}-mixed-${window}-logical.csv" "$pathGCC/${target}-origin-${window}-logical.csv" ${prefixID} ${window} >> mergedGCC-${window}.csv
    done
done

pathGitlab="./gitlabTarResult"
pickedSetGitlab=('14.0.0-ce.0' '14.0.1-ce.0' '14.0.10-ce.0' '14.0.11-ce.0' '14.0.12-ce.0' '14.0.2-ce.0' '14.0.3-ce.0' '14.0.4-ce.0' '14.0.5-ce.0' '14.0.6-ce.0' '14.0.7-ce.0' '14.0.8-ce.0' '14.0.9-ce.0' '14.1.0-ce.0' '14.1.1-ce.0' '14.1.2-ce.0' '14.1.3-ce.0' '14.1.4-ce.0' '14.1.5-ce.0' '14.1.6-ce.0' '14.1.7-ce.0' '14.1.8-ce.0' '14.10.0-ce.0' '14.10.1-ce.0' '14.10.2-ce.0' '14.2.0-ce.0' '14.2.1-ce.0' '14.2.2-ce.0' '14.2.3-ce.0' '14.2.4-ce.0' '14.2.5-ce.0' '14.2.6-ce.0' '14.2.7-ce.0' '14.3.0-ce.0' '14.3.1-ce.0' '14.3.2-ce.0' '14.3.3-ce.0' '14.3.4-ce.0' '14.3.5-ce.0' '14.3.6-ce.0' '14.4.0-ce.0' '14.4.1-ce.0' '14.4.2-ce.0' '14.4.3-ce.0' '14.4.4-ce.0' '14.4.5-ce.0' '14.5.0-ce.0' '14.5.1-ce.0' '14.5.2-ce.0' '14.5.3-ce.0' '14.5.4-ce.0' '14.6.0-ce.0' '14.6.1-ce.0' '14.6.2-ce.0' '14.6.3-ce.0' '14.6.4-ce.0' '14.6.5-ce.0' '14.6.6-ce.0' '14.6.7-ce.0' '14.7.0-ce.0' '14.7.1-ce.0' '14.7.2-ce.0' '14.7.3-ce.0' '14.7.4-ce.0' '14.7.5-ce.0' '14.7.6-ce.0' '14.7.7-ce.0' '14.8.0-ce.0' '14.8.1-ce.0' '14.8.2-ce.0' '14.8.3-ce.0' '14.8.4-ce.0' '14.8.5-ce.0' '14.8.6-ce.0' '14.9.0-ce.0' '14.9.1-ce.0' '14.9.2-ce.0' '14.9.3-ce.0' '14.9.4-ce.0')

for target in ${pickedSetGitlab[@]}; do
    cat $pathGitlab/${target}-origin-logical.csv >> mergedSnapBased-GitLab.csv
    for window in ${windowSize[@]}; do
        ./processWindow "$pathGitlab/${target}-mixed-${window}-logical.csv" "$pathGitlab/${target}-origin-${window}-logical.csv" ${prefixID} ${window} >> mergedGitLab-${window}.csv
    done
done