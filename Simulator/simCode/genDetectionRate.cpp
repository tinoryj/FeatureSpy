#include "featureExtraction.hpp"
#include <bits/stdc++.h>
#include <sys/time.h>
using namespace std;

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
    string thresholdStr(argv[2]);
    double threshold = stod(thresholdStr);
    int mixedPrefixOverThresholdCounter[featureNumber];
    string currentLine;
    int snapshotNumber = 0;
    for (int i = 0; i < featureNumber; i++) {
        mixedPrefixOverThresholdCounter[i] = 0;
    }
    while (getline(mixedStream, currentLine)) {
        vector<string> currentLineSplit = split(currentLine, ",");
        snapshotNumber++;
        for (int i = 0; i < featureNumber; i++) {
            double currentFreq = stod(currentLineSplit[i]);
            if (currentFreq >= threshold) {
                mixedPrefixOverThresholdCounter[i]++;
            }
        }
    }
    mixedStream.close();
    for (int i = 0; i < featureNumber; i++) {
        cout << setprecision(10) << "Detection ratio with " << i + 1 << " feature = " << (double)mixedPrefixOverThresholdCounter[i] / (double)snapshotNumber << endl;
    }
    return 0;
}
