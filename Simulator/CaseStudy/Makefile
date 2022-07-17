CC = clang++
CFLAGS = -O3
LIBS = -lcrypto -lssl
INCLUDES =-I./
MAIN_OBJS = windowCheck chunking snapCheck mixedChunksGen processWindow mixedFilesGen

all: windowCheck chunking snapCheck mixedChunksGen processWindow mixedFilesGen

processWindow: ./processWindow.cpp
	$(CC) $(CFLAGS) -o processWindow processWindow.cpp $(INCLUDES) $(LIBS)
mixedChunksGen: ./mixedChunksGen.cpp
	$(CC) $(CFLAGS) -o mixedChunksGen mixedChunksGen.cpp $(INCLUDES) $(LIBS)
mixedFilesGen: ./mixedFilesGen.cpp
	$(CC) $(CFLAGS) -o mixedFilesGen mixedFilesGen.cpp $(INCLUDES) $(LIBS)
windowCheck: ./windowCheck.cpp
	$(CC) $(CFLAGS) -o windowCheck windowCheck.cpp $(INCLUDES) $(LIBS)
snapCheck: ./snapCheck.cpp
	$(CC) $(CFLAGS) -o snapCheck snapCheck.cpp $(INCLUDES) $(LIBS)
chunking: ./chunking.cpp
	$(shell ! test -d "Content" && mkdir Content)
	$(CC) $(CFLAGS) -o chunking chunking.cpp $(INCLUDES) $(LIBS)

clean:
	@rm -f $(MAIN_OBJS)
	@rm -rf Content Result
	@rm -f *.chunkInfo

