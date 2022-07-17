CC = clang++
CFLAGS = -O3
LIBS = -lcrypto -lssl
INCLUDES =-I./
MAIN_OBJS = baselineSnapGen swappedSnapGen windowCheck snapCheck mixedFilesGen processWindowCheckResult

all: baselineSnapGen swappedSnapGen windowCheck snapCheck mixedFilesGen processWindowCheckResult

baselineSnapGen: ./baselineSnapGen.cpp
	$(CC) $(CFLAGS) -o baselineSnapGen baselineSnapGen.cpp $(INCLUDES) $(LIBS)
swappedSnapGen: ./swappedSnapGen.cpp
	$(CC) $(CFLAGS) -o swappedSnapGen swappedSnapGen.cpp $(INCLUDES) $(LIBS)
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
