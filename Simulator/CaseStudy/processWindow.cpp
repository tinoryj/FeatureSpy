#include "chunker.hpp"
#include "featureExtraction.hpp"
#include <bits/stdc++.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/time.h>
using namespace std;

#define PREFIX_UNIT 16 // block unit
vector<string> split(string s, string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }
    res.push_back(s.substr(pos_start));
    return res;
}

int main(int argc, char* argv[])
{
    cout.setf(ios_base::fixed, ios_base::floatfield);
    // int targetNumber = stoi(argv[1]);
    // for (int i = 1; i < targetNumber + 1; i++) {
    //     cout << i << "\t=0.005*I"
    //          << i
    //          << "\t=AVERAGE(IF(A1:A"
    //          << targetNumber
    //          << "-J"
    //          << i
    //          << ">0,1,0))\t=AVERAGE(IF(B1:B"
    //          << targetNumber
    //          << "-J"
    //          << i
    //          << ">0,1,0))\t=AVERAGE(IF(C1:C"
    //          << targetNumber
    //          << "-J"
    //          << i
    //          << ">0,1,0))\t=AVERAGE(IF(D1:D"
    //          << targetNumber
    //          << "-J"
    //          << i
    //          << ">0,1,0))\t=AVERAGE(IF(E1:E"
    //          << targetNumber
    //          << "-J"
    //          << i
    //          << ">0,1,0))\t=AVERAGE(IF(F1:F"
    //          << targetNumber
    //          << "-J"
    //          << i
    //          << ">0,1,0))" << endl;
    // }
    // exit(0);
    string mixedFilePath(argv[1]);
    string originFilePath(argv[2]);
    string prefixIDStr(argv[3]);
    string windowSizeStr(argv[4]);
    int prefixID = stoi(prefixIDStr);
    int windowSize = stoi(windowSizeStr);
    ifstream mixedStream, originStream;
    mixedStream.open(mixedFilePath, ios::binary | ios::in);
    if (mixedStream.is_open() == false) {
        cerr << "Error load mixed info list: " << mixedFilePath << endl;
        exit(0);
    }
    originStream.open(originFilePath, ios::binary | ios::in);
    if (originStream.is_open() == false) {
        cerr << "Error load mixed info list: " << originFilePath << endl;
        exit(0);
    }
    vector<uint64_t> mixedLogical, mixedDupVec, mixedFirstVec, mixedMinVec, mixedAllVec, mixedFirstPrefixVec[3], mixedMinPrefixVec[3], mixedAllPrefixVec[3];
    vector<uint64_t> originLogical, originDupVec, originFirstVec, originMinVec, originAllVec, originFirstPrefixVec[3], originMinPrefixVec[3], originAllPrefixVec[3];
    string currentLine;
    while (getline(mixedStream, currentLine)) {
        vector<string> currentLineSplit = split(currentLine, ",");
        uint64_t logicNumber = stoi(currentLineSplit[0]);
        mixedLogical.push_back(logicNumber);
        uint64_t dupNumber = stoi(currentLineSplit[2]);
        mixedDupVec.push_back(dupNumber);
        uint64_t firstNumber = stoi(currentLineSplit[4]);
        mixedFirstVec.push_back(firstNumber);
        uint64_t minNumber = stoi(currentLineSplit[6]);
        mixedMinVec.push_back(minNumber);
        uint64_t allNumber = stoi(currentLineSplit[8]);
        mixedAllVec.push_back(allNumber);
        for (int i = 0; i < 3; i++) {
            uint64_t firstPrefixNumber = stoi(currentLineSplit[10 + i * 6]);
            mixedFirstPrefixVec[i].push_back(firstPrefixNumber);
            uint64_t minPrefixNumber = stoi(currentLineSplit[12 + i * 6]);
            mixedMinPrefixVec[i].push_back(minPrefixNumber);
            uint64_t allPrefixNumber = stoi(currentLineSplit[14 + i * 6]);
            mixedAllPrefixVec[i].push_back(allPrefixNumber);
        }
    }
    mixedStream.close();
    while (getline(originStream, currentLine)) {
        vector<string> currentLineSplit = split(currentLine, ",");
        uint64_t logicNumber = stoi(currentLineSplit[0]);
        originLogical.push_back(logicNumber);
        uint64_t dupNumber = stoi(currentLineSplit[2]);
        originDupVec.push_back(dupNumber);
        uint64_t firstNumber = stoi(currentLineSplit[4]);
        originFirstVec.push_back(firstNumber);
        uint64_t minNumber = stoi(currentLineSplit[6]);
        originMinVec.push_back(minNumber);
        uint64_t allNumber = stoi(currentLineSplit[8]);
        originAllVec.push_back(allNumber);
        for (int i = 0; i < 3; i++) {
            uint64_t firstPrefixNumber = stoi(currentLineSplit[10 + i * 6]);
            originFirstPrefixVec[i].push_back(firstPrefixNumber);
            uint64_t minPrefixNumber = stoi(currentLineSplit[12 + i * 6]);
            originMinPrefixVec[i].push_back(minPrefixNumber);
            uint64_t allPrefixNumber = stoi(currentLineSplit[14 + i * 6]);
            originAllPrefixVec[i].push_back(allPrefixNumber);
        }
    }
    originStream.close();
    sort(mixedLogical.begin(), mixedLogical.end(), greater<uint64_t>());
    sort(mixedDupVec.begin(), mixedDupVec.end(), greater<uint64_t>());
    sort(mixedFirstVec.begin(), mixedFirstVec.end(), greater<uint64_t>());
    sort(mixedMinVec.begin(), mixedMinVec.end(), greater<uint64_t>());
    sort(mixedAllVec.begin(), mixedAllVec.end(), greater<uint64_t>());
    cout << mixedLogical[0] << " , " << mixedDupVec[0] << " , " << mixedFirstVec[0] << " , " << mixedMinVec[0] << " , " << mixedAllVec[0] << " , ";

    for (int i = prefixID; i < prefixID + 1; i++) {
        sort(mixedFirstPrefixVec[i].begin(), mixedFirstPrefixVec[i].end(), greater<uint64_t>());
        sort(mixedMinPrefixVec[i].begin(), mixedMinPrefixVec[i].end(), greater<uint64_t>());
        sort(mixedAllPrefixVec[i].begin(), mixedAllPrefixVec[i].end(), greater<uint64_t>());
        // cout << mixedFirstPrefixVec[i][0] << " , " << mixedMinPrefixVec[i][0] << " , " << mixedAllPrefixVec[i][0] << " , ";
        cout << setprecision(10) << (double)mixedFirstPrefixVec[i][0] / (double)windowSize << " , " << (double)mixedMinPrefixVec[i][0] / (double)windowSize << " , " << (double)mixedAllPrefixVec[i][0] / (double)windowSize << " , ";
    }
    sort(originLogical.begin(), originLogical.end(), greater<uint64_t>());
    sort(originDupVec.begin(), originDupVec.end(), greater<uint64_t>());
    sort(originFirstVec.begin(), originFirstVec.end(), greater<uint64_t>());
    sort(originMinVec.begin(), originMinVec.end(), greater<uint64_t>());
    sort(originAllVec.begin(), originAllVec.end(), greater<uint64_t>());
    cout << originLogical[0] << " , " << originDupVec[0] << " , " << originFirstVec[0] << " , " << originMinVec[0] << " , " << originAllVec[0] << " , ";
    for (int i = prefixID; i < prefixID + 1; i++) {
        sort(originFirstPrefixVec[i].begin(), originFirstPrefixVec[i].end(), greater<uint64_t>());
        sort(originMinPrefixVec[i].begin(), originMinPrefixVec[i].end(), greater<uint64_t>());
        sort(originAllPrefixVec[i].begin(), originAllPrefixVec[i].end(), greater<uint64_t>());
        // cout << originFirstPrefixVec[i][0] << " , " << originMinPrefixVec[i][0] << " , " << originAllPrefixVec[i][0] << " , ";
        cout << setprecision(10) << (double)originFirstPrefixVec[i][0] / (double)windowSize << " , " << (double)originMinPrefixVec[i][0] / (double)windowSize << " , " << (double)originAllPrefixVec[i][0] / (double)windowSize << " , ";
    }
    cout << "0" << endl;
    return 0;
}
