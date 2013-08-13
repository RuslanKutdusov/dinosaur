CC = clang++
#CFLAGS = -std=c++11 -O0 -Wall -c
#
#all: cfg dht exceptions fs log network torrent utils types
#
#cfg:
#	$(CC) $(CFLAGS) libdinosaur/cfg/cfg.cpp

CFLAGS = -std=c++11 -O0 -Wall -c -fmessage-length=0 -D_FILE_OFFSET_BITS=64 `pkg-config --cflags gtk+-2.0`
DEBUG = -DBITTORRENT_DEBUG -DFS_DEBUG -DPEER_DEBUG -DDHT_DEBUG
LDFLAGS = -L/usr/local/lib `pkg-config --libs gtk+-2.0` -lpcrecpp -lpcre -lboost_date_time -lboost_serialization -pthread -D_FILE_OFFSET_BITS=64 -Wl,-export-dynamic
SOURCES_SHARED = $(shell find libdinosaur/ | grep cpp) api/jni_export.cpp api/conversions.cpp
OBJECTS_SHARED = $(SOURCES_SHARED:.cpp=.o)
SOURCE_EXEC = $(shell find libdinosaur/ | grep cpp) gui/gui.cpp gui/cfg.cpp bittorrent.cpp
OBJECTS_EXEC = $(SOURCE_EXEC:.cpp=.o)
EXECUTABLE=dinosaur
LIB=libdinosaur.so

all:  $(SOURCE_EXEC) dinosaur


debug: CFLAGS += $(DEBUG)
debug:	CFLAGS += -g3
debug:	$(OBJECTS_EXEC)
	$(CC) $(OBJECTS_EXEC) -o $(EXECUTABLE) $(LDFLAGS)
        #rm $(OBJECTS_EXEC)

dinosaur: $(OBJECTS_EXEC)
	$(CC) $(OBJECTS_EXEC) -o $(EXECUTABLE) $(LDFLAGS)
        #rm $(OBJECTS_EXEC)
	
shared: $(OBJECTS_SHARED)
	$(CC) -shared $(OBJECTS_SHARED) -o $(LIB) $(LDFLAGS)
	
shared_debug: CFLAGS += $(DEBUG)
shared_debug: CFLAGS += -g3
shared_debug: $(OBJECTS_SHARED)
	$(CC) -shared $(OBJECTS_SHARED) -o $(LIB) $(LDFLAGS)

.cpp.o: 
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS_EXEC)
	
clean_shared:
	rm $(OBJECTS_SHARED)


	

