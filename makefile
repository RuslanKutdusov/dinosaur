CC = g++
CFLAGS = -O0 -Wall -c -fmessage-length=0 -D_FILE_OFFSET_BITS=64 `pkg-config --cflags gtk+-2.0`
DEBUG = -DBITTORRENT_DEBUG -DFS_DEBUG -DPEER_DEBUG
LDFLAGS = -L/usr/local/lib `pkg-config --libs gtk+-2.0` -lpcrecpp -lpcre -lrt -lboost_serialization -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lglib-2.0 -pthread -D_FILE_OFFSET_BITS=64 -export-dynamic
SOURCES_SHARED = $(shell find libdinosaur/ | grep cpp)
OBJECTS_SHARED = $(SOURCES_SHARED:.cpp=.o)
SOURCE_EXEC = $(shell find . | grep cpp)
OBJECTS_EXEC = $(SOURCE_EXEC:.cpp=.o)
EXECUTABLE=dinosaur
LIB=libdinosaur.so

all:  $(SOURCE_EXEC) dinosaur


debug: CFLAGS += $(DEBUG)
debug:	CFLAGS += -g3
debug:	$(OBJECTS_EXEC)
	$(CC) $(OBJECTS_EXEC) -o $(EXECUTABLE) $(LDFLAGS)
	rm $(OBJECTS_EXEC)

dinosaur: $(OBJECTS_EXEC)
	$(CC) $(OBJECTS_EXEC) -o $(EXECUTABLE) $(LDFLAGS)
	rm $(OBJECTS_EXEC)
	
shared: $(OBJECTS_SHARED)
	$(CC) -shared $(OBJECTS_SHARED) -o $(LIB) $(LDFLAGS)
	rm $(OBJECTS_SHARED)

.cpp.o: 
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm $(OBJECTS_EXEC)
