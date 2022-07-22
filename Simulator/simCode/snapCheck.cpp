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
    string chunkInfoPath(argc[1]);
    ifstream chunkInfoStream;
    chunkInfoStream.open(chunkInfoPath, ios::binary | ios::in);
    if (chunkInfoStream.is_open() == false) {
        cerr << "Error load chunk info list: " << chunkInfoPath << endl;
        return 0;
    }
    string currentLine;
    uint64_t currentWindowCount = 0;

    unordered_map<string, vector<uint64_t>> prefixCounter[featureNumber], dedupLicationMap, featureCounter[featureNumber];

    gettimeofday(&timestart, NULL);

    while (getline(chunkInfoStream, currentLine)) {
        int chunkID = stoi(currentLine);
        getline(chunkInfoStream, currentLine);
        string chunkHash = currentLine;
        getline(chunkInfoStream, currentLine);
        int chunkSize = stoi(currentLine);
        for (int i = 0; i < featureNumber; i++) {
            getline(chunkInfoStream, currentLine);
            string featureStr = currentLine;
            if (featureCounter[i].find(featureStr) == featureCounter[i].end()) {
                vector<uint64_t> chunkVec;
                chunkVec.push_back(chunkID);
                featureCounter[i].insert(make_pair(featureStr, chunkVec));
            } else {
                featureCounter[i].at(featureStr).push_back(chunkID);
            }
        }
        for (int i = 0; i < featureNumber; i++) {
            getline(chunkInfoStream, currentLine);
            string prefixStr = currentLine;
            if (prefixCounter[i].find(prefixStr) == prefixCounter[i].end()) {
                vector<uint64_t> chunkVec;
                chunkVec.push_back(chunkID);
                prefixCounter[i].insert(make_pair(prefixStr, chunkVec));
            } else {
                prefixCounter[i].at(prefixStr).push_back(chunkID);
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
        currentWindowCount++;
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
        for (int index = 0; index < featureNumber; index++) {
            maxFreq = 0;
#if OUTPUTID == 1
            flag.clear();
#endif
            for (auto i : featureCounter[index]) {
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
            cout << featureCounter[index].size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
            for (int i = 0; i < flag.size(); i++) {
                cout << flag[i] << " ";
            }
            cout << " , ";
#endif
            featureCounter[index].clear();
        }
        for (int index = 0; index < featureNumber; index++) {
            maxFreq = 0;
#if OUTPUTID == 1
            flag.clear();
#endif
            for (auto i : prefixCounter[index]) {
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
            cout << prefixCounter[index].size() << " , " << maxFreq << " , ";
#if OUTPUTID == 1
            for (int i = 0; i < flag.size(); i++) {
                cout << flag[i] << " ";
            }
            cout << " , ";
#endif
            prefixCounter[index].clear();
        }
        dedupLicationMap.clear();
        cout << 0 << endl;
        currentWindowCount = 0;
    }
    chunkInfoStream.close();
    gettimeofday(&timeend, NULL);
    long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    double second = diff / 1000000.0;
    cerr << "System : total work time = " << second << " s" << endl;
    return 0;
}