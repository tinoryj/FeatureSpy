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
    string windowSizeStr(argv[2]);
    int windowSize = stoi(windowSizeStr);
    ifstream mixedStream;
    mixedStream.open(mixedFilePath, ios::binary | ios::in);
    if (mixedStream.is_open() == false) {
        cerr << "Error load mixed info list: " << mixedFilePath << endl;
        exit(0);
    }
    vector<uint64_t> mixedLogical, mixedDupVec, mixedFeatureVec[4], mixedPrefixVec[4];
    string currentLine;
    while (getline(mixedStream, currentLine)) {
        vector<string> currentLineSplit = split(currentLine, ",");
        uint64_t logicNumber = stoi(currentLineSplit[0]);
        mixedLogical.push_back(logicNumber);
        uint64_t dupNumber = stoi(currentLineSplit[2]);
        mixedDupVec.push_back(dupNumber);
        uint64_t firstNumber = stoi(currentLineSplit[4]);
        mixedFeatureVec[0].push_back(firstNumber);
        uint64_t minNumber = stoi(currentLineSplit[6]);
        mixedFeatureVec[1].push_back(minNumber);
        uint64_t allNumber = stoi(currentLineSplit[8]);
        mixedFeatureVec[2].push_back(allNumber);
        uint64_t feature4Number = stoi(currentLineSplit[10]);
        mixedFeatureVec[3].push_back(feature4Number);

        uint64_t p1Number = stoi(currentLineSplit[4]);
        mixedPrefixVec[0].push_back(p1Number);
        uint64_t p2Number = stoi(currentLineSplit[6]);
        mixedPrefixVec[1].push_back(p2Number);
        uint64_t p3Number = stoi(currentLineSplit[8]);
        mixedPrefixVec[2].push_back(p3Number);
        uint64_t p4Number = stoi(currentLineSplit[10]);
        mixedPrefixVec[3].push_back(p4Number);
    }
    mixedStream.close();
    sort(mixedLogical.begin(), mixedLogical.end(), greater<uint64_t>());
    sort(mixedDupVec.begin(), mixedDupVec.end(), greater<uint64_t>());
    for (int i = 0; i < 4; i++) {
        sort(mixedFeatureVec[i].begin(), mixedFeatureVec[i].end(), greater<uint64_t>());
        sort(mixedPrefixVec[i].begin(), mixedPrefixVec[i].end(), greater<uint64_t>());
    }

    cout << setprecision(10) << (double)mixedPrefixVec[0][0] / (double)windowSize << " , " << (double)mixedPrefixVec[1][0] / (double)windowSize << " , " << (double)mixedPrefixVec[2][0] / (double)windowSize << " , " << (double)mixedPrefixVec[3][0] / (double)windowSize << endl;
    return 0;
}
