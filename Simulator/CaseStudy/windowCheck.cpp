#include "chunker.hpp"
#include "featureExtraction.hpp"
#include <bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/time.h>
using namespace std;
#define OUTPUTID 0

int main(int argv, char* argc[])
{
    struct timeval timestart;
    struct timeval timeend;
    FeatureGen* featureGenObj = new FeatureGen();
    string chunkInfoPath(argc[1]);
    string windowSizeStr(argc[2]);
    int windowSize = stoi(windowSizeStr);
    ifstream chunkInfoStream;
    chunkInfoStream.open(chunkInfoPath, ios::binary | ios::in);
    if (chunkInfoStream.is_open() == false) {
        cerr << "Error load chunk info list: " << chunkInfoPath << endl;
        return 0;
    }
    string currentLine;
    uint64_t currentWindowCount = 0;

    unordered_map<string, vector<uint64_t>> prefixCounterFirst[3], prefixCounterMin[3], prefixCounterAll[3], dedupLicationMap, featureCounterFirst, featureCounterMin, featureCounterAll;

    gettimeofday(&timestart, NULL);

    while (getline(chunkInfoStream, currentLine)) {
        int chunkID = stoi(currentLine);
        getline(chunkInfoStream, currentLine);
        string chunkHash = currentLine;
        getline(chunkInfoStream, currentLine);
        int chunkSize = stoi(currentLine);
        getline(chunkInfoStream, currentLine);
        string firstFStr = currentLine;
        getline(chunkInfoStream, currentLine);
        string minFStr = currentLine;
        getline(chunkInfoStream, currentLine);
        string allFStr = currentLine;
        for (int i = 0; i < 3; i++) {
            getline(chunkInfoStream, currentLine);
            string firstPrefixStr = currentLine;
            getline(chunkInfoStream, currentLine);
            string minPrefixStr = currentLine;
            getline(chunkInfoStream, currentLine);
            string allPrefixStr = currentLine;
            //prefix-first
            if (prefixCounterFirst[i].find(firstPrefixStr) == prefixCounterFirst[i].end()) {
                vector<uint64_t> chunkVec;
                chunkVec.push_back(chunkID);
                prefixCounterFirst[i].insert(make_pair(firstPrefixStr, chunkVec));
            } else {
                prefixCounterFirst[i].at(firstPrefixStr).push_back(chunkID);
            }
            //prefix-min
            if (prefixCounterMin[i].find(minPrefixStr) == prefixCounterMin[i].end()) {
                vector<uint64_t> chunkVec;
                chunkVec.push_back(chunkID);
                prefixCounterMin[i].insert(make_pair(minPrefixStr, chunkVec));
            } else {
                prefixCounterMin[i].at(minPrefixStr).push_back(chunkID);
            }
            //prefix-all
            if (prefixCounterAll[i].find(allPrefixStr) == prefixCounterAll[i].end()) {
                vector<uint64_t> chunkVec;
                chunkVec.push_back(chunkID);
                prefixCounterAll[i].insert(make_pair(allPrefixStr, chunkVec));
            } else {
                prefixCounterAll[i].at(allPrefixStr).push_back(chunkID);
            }
        }
        //dedup
        if (dedupLicationMap.find(chunkHash) == dedupLicationMap.end()) {
            vector<uint64_t> chunkVec;
            chunkVec.push_back(chunkID);
            dedupLicationMap.insert(make_pair(chunkHash, chunkVec));
        } else {
            dedupLicationMap.at(chunkHash).push_back(chunkID);
        }
        //count feature
        if (featureCounterFirst.find(firstFStr) == featureCounterFirst.end()) {
            vector<uint64_t> chunkVec;
            chunkVec.push_back(chunkID);
            featureCounterFirst.insert(make_pair(firstFStr, chunkVec));
        } else {
            featureCounterFirst.at(firstFStr).push_back(chunkID);
        }
        if (featureCounterMin.find(minFStr) == featureCounterMin.end()) {
            vector<uint64_t> chunkVec;
            chunkVec.push_back(chunkID);
            featureCounterMin.insert(make_pair(minFStr, chunkVec));
        } else {
            featureCounterMin.at(minFStr).push_back(chunkID);
        }
        if (featureCounterAll.find(allFStr) == featureCounterAll.end()) {
            vector<uint64_t> chunkVec;
            chunkVec.push_back(chunkID);
            featureCounterAll.insert(make_pair(allFStr, chunkVec));
        } else {
            featureCounterAll.at(allFStr).push_back(chunkID);
        }
        currentWindowCount++;
        //output
        if (currentWindowCount == windowSize) {
            cout << currentWindowCount << " , ";
            int maxFreq = 0;
#if OUTPUTID == 1
            vector<uint64_t> flag;
#endif
            for (auto i : dedupLicationMap) {
                if (i.second.size() > maxFreq) {
                    maxFreq = i.second.size();
#if OUTPUTID == 1
                    flag.clear();
                    for (int j = 0; j < maxFreq; j++) {
                        flag.push_back(i.second[j]);
                    }
#endif
                }
            }
            cout << dedupLicationMap.size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
            for (int i = 0; i < flag.size(); i++) {
                cout << flag[i] << " ";
            }
            cout << " , ";
#endif
            // featurtes
            maxFreq = 0;
#if OUTPUTID == 1
            flag.clear();
#endif
            for (auto i : featureCounterFirst) {
                if (i.second.size() > maxFreq) {
                    maxFreq = i.second.size();
#if OUTPUTID == 1
                    flag.clear();
                    for (int j = 0; j < maxFreq; j++) {
                        flag.push_back(i.second[j]);
                    }
#endif
                }
            }
            cout << featureCounterFirst.size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
            for (int i = 0; i < flag.size(); i++) {
                cout << flag[i] << " ";
            }
            cout << " , ";
#endif
            maxFreq = 0;
#if OUTPUTID == 1
            flag.clear();
#endif
            for (auto i : featureCounterMin) {
                if (i.second.size() > maxFreq) {
                    maxFreq = i.second.size();
#if OUTPUTID == 1
                    flag.clear();
                    for (int j = 0; j < maxFreq; j++) {
                        flag.push_back(i.second[j]);
                    }
#endif
                }
            }
            cout << featureCounterMin.size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
            for (int i = 0; i < flag.size(); i++) {
                cout << flag[i] << " ";
            }
            cout << " , ";
#endif
            maxFreq = 0;
#if OUTPUTID == 1
            flag.clear();
#endif
            for (auto i : featureCounterAll) {
                if (i.second.size() > maxFreq) {
                    maxFreq = i.second.size();
#if OUTPUTID == 1
                    flag.clear();
                    for (int j = 0; j < maxFreq; j++) {
                        flag.push_back(i.second[j]);
                    }
#endif
                }
            }
            cout << featureCounterAll.size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
            for (int i = 0; i < flag.size(); i++) {
                cout << flag[i] << " ";
            }
            cout << " , ";
#endif
            for (int index = 0; index < 3; index++) {
                maxFreq = 0;
#if OUTPUTID == 1
                flag.clear();
#endif
                for (auto i : prefixCounterFirst[index]) {
                    if (i.second.size() > maxFreq) {
                        maxFreq = i.second.size();
#if OUTPUTID == 1
                        flag.clear();
                        for (int j = 0; j < maxFreq; j++) {
                            flag.push_back(i.second[j]);
                        }
#endif
                    }
                }
                cout << prefixCounterFirst[index].size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
                for (int i = 0; i < flag.size(); i++) {
                    cout << flag[i] << " ";
                }
                cout << " , ";
#endif
                maxFreq = 0;
#if OUTPUTID == 1
                flag.clear();
#endif
                for (auto i : prefixCounterMin[index]) {
                    if (i.second.size() > maxFreq) {
                        maxFreq = i.second.size();
#if OUTPUTID == 1
                        flag.clear();
                        for (int j = 0; j < maxFreq; j++) {
                            flag.push_back(i.second[j]);
                        }
#endif
                    }
                }
                cout << prefixCounterMin[index].size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
                for (int i = 0; i < flag.size(); i++) {
                    cout << flag[i] << " ";
                }
                cout << " , ";
#endif
                maxFreq = 0;
#if OUTPUTID == 1
                flag.clear();
#endif
                for (auto i : prefixCounterAll[index]) {
                    if (i.second.size() > maxFreq) {
                        maxFreq = i.second.size();
#if OUTPUTID == 1
                        flag.clear();
                        for (int j = 0; j < maxFreq; j++) {
                            flag.push_back(i.second[j]);
                        }
#endif
                    }
                }
                cout << prefixCounterAll[index].size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
                for (int i = 0; i < flag.size(); i++) {
                    cout << flag[i] << " ";
                }
                cout << " , ";
#endif
                prefixCounterAll[index].clear();
                prefixCounterFirst[index].clear();
                prefixCounterMin[index].clear();
            }
            featureCounterAll.clear();
            featureCounterFirst.clear();
            featureCounterMin.clear();
            dedupLicationMap.clear();
            cout << 0 << endl;
            currentWindowCount = 0;
        }
    }
    //output
    if (currentWindowCount != 0) {
        cout << currentWindowCount << " , ";
        int maxFreq = 0;
#if OUTPUTID == 1
        vector<uint64_t> flag;
#endif
        for (auto i : dedupLicationMap) {
            if (i.second.size() > maxFreq) {
                maxFreq = i.second.size();
#if OUTPUTID == 1
                flag.clear();
                for (int j = 0; j < maxFreq; j++) {
                    flag.push_back(i.second[j]);
                }
#endif
            }
        }
        cout << dedupLicationMap.size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
        for (int i = 0; i < flag.size(); i++) {
            cout << flag[i] << " ";
        }
        cout << " , ";
#endif
        // featurtes
        maxFreq = 0;
#if OUTPUTID == 1
        flag.clear();
#endif
        for (auto i : featureCounterFirst) {
            if (i.second.size() > maxFreq) {
                maxFreq = i.second.size();
#if OUTPUTID == 1
                flag.clear();
                for (int j = 0; j < maxFreq; j++) {
                    flag.push_back(i.second[j]);
                }
#endif
            }
        }
        cout << featureCounterFirst.size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
        for (int i = 0; i < flag.size(); i++) {
            cout << flag[i] << " ";
        }
        cout << " , ";
#endif
        maxFreq = 0;
#if OUTPUTID == 1
        flag.clear();
#endif
        for (auto i : featureCounterMin) {
            if (i.second.size() > maxFreq) {
                maxFreq = i.second.size();
#if OUTPUTID == 1
                flag.clear();
                for (int j = 0; j < maxFreq; j++) {
                    flag.push_back(i.second[j]);
                }
#endif
            }
        }
        cout << featureCounterMin.size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
        for (int i = 0; i < flag.size(); i++) {
            cout << flag[i] << " ";
        }
        cout << " , ";
#endif
        maxFreq = 0;
#if OUTPUTID == 1
        flag.clear();
#endif
        for (auto i : featureCounterAll) {
            if (i.second.size() > maxFreq) {
                maxFreq = i.second.size();
#if OUTPUTID == 1
                flag.clear();
                for (int j = 0; j < maxFreq; j++) {
                    flag.push_back(i.second[j]);
                }
#endif
            }
        }
        cout << featureCounterAll.size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
        for (int i = 0; i < flag.size(); i++) {
            cout << flag[i] << " ";
        }
        cout << " , ";
#endif
        for (int index = 0; index < 3; index++) {
            maxFreq = 0;
#if OUTPUTID == 1
            flag.clear();
#endif
            for (auto i : prefixCounterFirst[index]) {
                if (i.second.size() > maxFreq) {
                    maxFreq = i.second.size();
#if OUTPUTID == 1
                    flag.clear();
                    for (int j = 0; j < maxFreq; j++) {
                        flag.push_back(i.second[j]);
                    }
#endif
                }
            }
            cout << prefixCounterFirst[index].size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
            for (int i = 0; i < flag.size(); i++) {
                cout << flag[i] << " ";
            }
            cout << " , ";
#endif
            maxFreq = 0;
#if OUTPUTID == 1
            flag.clear();
#endif
            for (auto i : prefixCounterMin[index]) {
                if (i.second.size() > maxFreq) {
                    maxFreq = i.second.size();
#if OUTPUTID == 1
                    flag.clear();
                    for (int j = 0; j < maxFreq; j++) {
                        flag.push_back(i.second[j]);
                    }
#endif
                }
            }
            cout << prefixCounterMin[index].size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
            for (int i = 0; i < flag.size(); i++) {
                cout << flag[i] << " ";
            }
            cout << " , ";
#endif
            maxFreq = 0;
#if OUTPUTID == 1
            flag.clear();
#endif
            for (auto i : prefixCounterAll[index]) {
                if (i.second.size() > maxFreq) {
                    maxFreq = i.second.size();
#if OUTPUTID == 1
                    flag.clear();
                    for (int j = 0; j < maxFreq; j++) {
                        flag.push_back(i.second[j]);
                    }
#endif
                }
            }
            cout << prefixCounterAll[index].size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
            for (int i = 0; i < flag.size(); i++) {
                cout << flag[i] << " ";
            }
            cout << " , ";
#endif
            prefixCounterAll[index].clear();
            prefixCounterFirst[index].clear();
            prefixCounterMin[index].clear();
        }
        featureCounterAll.clear();
        featureCounterFirst.clear();
        featureCounterMin.clear();
        dedupLicationMap.clear();
        cout << 0 << endl;
    }
    chunkInfoStream.close();
    delete featureGenObj;
    gettimeofday(&timeend, NULL);
    long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    double second = diff / 1000000.0;
    cerr << "System : total work time = " << second << " s" << endl;
    return 0;
}