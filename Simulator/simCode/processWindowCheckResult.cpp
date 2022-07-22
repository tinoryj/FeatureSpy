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
    string mixedFilePath(argv[1]);
    ifstream mixedStream;
    mixedStream.open(mixedFilePath, ios::binary | ios::in);
    if (mixedStream.is_open() == false) {
        cerr << "Error load mixed info list: " << mixedFilePath << endl;
        exit(0);
    }
    vector<uint64_t> windowCountVec, mixedFeatureVec[featureNumber], mixedPrefixVec[featureNumber];
    string currentLine;
    while (getline(mixedStream, currentLine)) {
        vector<string> currentLineSplit = split(currentLine, ",");
        uint64_t number = stoi(currentLineSplit[0]);
        windowCountVec.push_back(number);
        for (int i = 0; i < featureNumber; i++) {
            uint64_t matchNumber = stoi(currentLineSplit[4 + i * 2]);
            mixedFeatureVec[i].push_back(matchNumber);
        }
        for (int i = 0; i < featureNumber; i++) {
            uint64_t matchNumber = stoi(currentLineSplit[10 + i * 2]);
            mixedPrefixVec[i].push_back(matchNumber);
        }
    }
    mixedStream.close();
    sort(windowCountVec.begin(), windowCountVec.end(), greater<uint64_t>());
    for (int i = 0; i < featureNumber - 1; i++) {
        sort(mixedFeatureVec[i].begin(), mixedFeatureVec[i].end(), greater<uint64_t>());
        sort(mixedPrefixVec[i].begin(), mixedPrefixVec[i].end(), greater<uint64_t>());
        cout << setprecision(10) << (double)mixedPrefixVec[i][0] / (double)windowCountVec[0] << " , ";
    }
    sort(mixedPrefixVec[featureNumber - 1].begin(), mixedPrefixVec[featureNumber - 1].end(), greater<uint64_t>());
    cout << setprecision(10) << (double)mixedPrefixVec[featureNumber - 1][0] / (double)windowCountVec[0] << endl;
    return 0;
}
