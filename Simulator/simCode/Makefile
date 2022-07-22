CC = clang++
CFLAGS = -O3
LIBS = -lcrypto -lssl
INCLUDES =-I./
MAIN_OBJS = chunking baselineSnapGen swappedSnapGen  mixedFilesGenSYN mixedFilesGen windowCheck snapCheck processWindowCheckResult genDetectionRate
all: chunking baselineSnapGen swappedSnapGen mixedFilesGenSYN mixedFilesGen windowCheck snapCheck processWindowCheckResult genDetectionRate

genDetectionRate: ./genDetectionRate.cpp
	$(CC) $(CFLAGS) -o genDetectionRate genDetectionRate.cpp $(INCLUDES) $(LIBS)
chunking: ./chunking.cpp
	$(CC) $(CFLAGS) -o chunking chunking.cpp $(INCLUDES) $(LIBS)
baselineSnapGen: ./baselineSnapGen.cpp
	$(CC) $(CFLAGS) -o baselineSnapGen baselineSnapGen.cpp $(INCLUDES) $(LIBS)
swappedSnapGen: ./swappedSnapGen.cpp
	$(CC) $(CFLAGS) -o swappedSnapGen swappedSnapGen.cpp $(INCLUDES) $(LIBS)
mixedFilesGenSYN: ./mixedFilesGenSYN.cpp
	$(CC) $(CFLAGS) -o mixedFilesGenSYN mixedFilesGenSYN.cpp $(INCLUDES) $(LIBS)
mixedFilesGen: ./mixedFilesGen.cpp
	$(CC) $(CFLAGS) -o mixedFilesGen mixedFilesGen.cpp $(INCLUDES) $(LIBS)
windowCheck: ./windowCheck.cpp
	$(CC) $(CFLAGS) -o windowCheck windowCheck.cpp $(INCLUDES) $(LIBS)
snapCheck: ./snapCheck.cpp
	$(CC) $(CFLAGS) -o snapCheck snapCheck.cpp $(INCLUDES) $(LIBS)
processWindowCheckResult: ./processWindowCheckResult.cpp
	$(CC) $(CFLAGS) -o processWindowCheckResult processWindowCheckResult.cpp $(INCLUDES) $(LIBS)

clean:
	@rm -f $(MAIN_OBJS)
	@rm -f *.chunkInfo
	@rm -f *.fileInfo
	@rm -f *.targetSwapList
	@rm -f *.csv
