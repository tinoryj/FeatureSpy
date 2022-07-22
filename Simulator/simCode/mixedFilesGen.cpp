#include "featureExtraction.hpp"
#include <bits/stdc++.h>
#include <sys/time.h>
using namespace std;

int main(int argv, char* argc[])
{
    struct timeval timestart;
    struct timeval timeend;
    gettimeofday(&timestart, NULL);
    string snapPathName(argc[1]);
    ifstream snapFileListStream, snapChunkListStream;
    snapFileListStream.open(snapPathName + ".fileInfo", ios::in | ios::binary);
    if (!snapFileListStream.is_open()) {
        cerr << "Error open file list" << endl;
        return 0;
    }
    snapChunkListStream.open(snapPathName + ".chunkInfo", ios::in | ios::binary);
    if (!snapChunkListStream.is_open()) {
        cerr << "Error open chunk list" << endl;
        return 0;
    }
    string fakePathName(argc[2]);
    ifstream fakeFileListStream, fakeChunkListStream;
    fakeFileListStream.open(fakePathName + ".fileInfo", ios::in | ios::binary);
    if (!fakeFileListStream.is_open()) {
        cerr << "Error open file list" << endl;
        return 0;
    }
    fakeChunkListStream.open(fakePathName + ".chunkInfo", ios::in | ios::binary);
    if (!fakeChunkListStream.is_open()) {
        cerr << "Error open chunk list" << endl;
        return 0;
    }
    vector<uint64_t> snapFileChunkNumberVec, fakeFileChunkNumberVec;
    string currentLineStr;
    while (getline(snapFileListStream, currentLineStr)) {
        getline(snapFileListStream, currentLineStr);
        uint64_t chunkNumber = stoi(currentLineStr);
        snapFileChunkNumberVec.push_back(chunkNumber);
    }
    cerr << "Target snapshot file numebr = " << snapFileChunkNumberVec.size() << endl;
    snapFileListStream.close();
    while (getline(fakeFileListStream, currentLineStr)) {
        getline(fakeFileListStream, currentLineStr);
        uint64_t chunkNumber = stoi(currentLineStr);
        fakeFileChunkNumberVec.push_back(chunkNumber);
    }
    cerr << "Target fake file numebr = " << fakeFileChunkNumberVec.size() << endl;
    fakeFileListStream.close();

    // generate base file done
    int rawFileNumber = snapFileChunkNumberVec.size();
    int fakeFileNumber = fakeFileChunkNumberVec.size();
    int fakeFileNumberPerSlot = fakeFileNumber / (rawFileNumber + 1);

    ofstream chunkInfo, fileInfo;
    chunkInfo.open(snapPathName + ".mixed.chunkInfo", ios::out);
    fileInfo.open(snapPathName + ".mixed.fileInfo", ios::out);

    if (fakeFileNumberPerSlot == 0) {
        cerr << "Raw files' number > fake files' number" << endl;
        int rawFileNumberPerSlot = rawFileNumber / (fakeFileNumber + 1);
        // output
        int chunkIDCnt = 0;
        int generatedFileNumber = 0;
        int rawFileIndex = 0;
        for (int i = 0; i < fakeFileNumber; i++) {
            for (int j = 0; j < rawFileNumberPerSlot; j++) {
                for (int k = 0; k < snapFileChunkNumberVec[rawFileIndex]; k++) {
                    for (int index = 0; index < 3 + featureNumber * 2; index++) {
                        string tempLine;
                        getline(snapChunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                fileInfo << "Raw : " << snapFileChunkNumberVec[rawFileIndex] << endl;
                rawFileIndex++;
                generatedFileNumber++;
            }
            for (int k = 0; k < fakeFileChunkNumberVec[i]; k++) {
                for (int index = 0; index < 3 + featureNumber * 2; index++) {
                    string tempLine;
                    getline(fakeChunkListStream, tempLine);
                    chunkInfo << tempLine << endl;
                }
            }
            fileInfo << "Fake : " << fakeFileChunkNumberVec[i] << endl;
        }
        if (rawFileNumber > generatedFileNumber) {
            for (int i = 0; i < rawFileNumber - generatedFileNumber; i++) {
                for (int j = 0; j < snapFileChunkNumberVec[rawFileIndex]; j++) {
                    for (int index = 0; index < 3 + featureNumber * 2; index++) {
                        string tempLine;
                        getline(snapChunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                fileInfo << "Raw : " << snapFileChunkNumberVec[rawFileIndex] << endl;
                rawFileIndex++;
            }
        }
    } else {
        // output
        int chunkIDCnt = 0;
        int generatedFileNumber = 0;
        int fakeFileIndex = 0;
        for (int i = 0; i < rawFileNumber; i++) {
            for (int j = 0; j < fakeFileNumberPerSlot; j++) {
                for (int k = 0; k < fakeFileChunkNumberVec[fakeFileIndex]; k++) {
                    for (int index = 0; index < 3 + featureNumber * 2; index++) {
                        string tempLine;
                        getline(fakeChunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                fileInfo << "Fake : " << fakeFileChunkNumberVec[fakeFileIndex] << endl;
                fakeFileIndex++;
                generatedFileNumber++;
            }
            for (int j = 0; j < snapFileChunkNumberVec[i]; j++) {
                for (int index = 0; index < 3 + featureNumber * 2; index++) {
                    string tempLine;
                    getline(snapChunkListStream, tempLine);
                    chunkInfo << tempLine << endl;
                }
            }
            fileInfo << "Raw : " << snapFileChunkNumberVec[i] << endl;
        }
        if (fakeFileNumber > generatedFileNumber) {
            for (int i = 0; i < fakeFileNumber - generatedFileNumber; i++) {
                for (int k = 0; k < fakeFileChunkNumberVec[fakeFileIndex]; k++) {
                    for (int index = 0; index < 3 + featureNumber * 2; index++) {
                        string tempLine;
                        getline(fakeChunkListStream, tempLine);
                        chunkInfo << tempLine << endl;
                    }
                }
                fileInfo << "Fake : " << fakeFileChunkNumberVec[fakeFileIndex] << endl;
                fakeFileIndex++;
            }
        }
    }
    chunkInfo.close();
    fileInfo.close();
    snapChunkListStream.close();
    fakeChunkListStream.close();
    gettimeofday(&timeend, NULL);
    long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    double second = diff / 1000000.0;
    cerr << "System : total work time = " << second << " s" << endl;
    return 0;
}
