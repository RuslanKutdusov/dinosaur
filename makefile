CC = g++
CFLAGS = -O0 -Wall -c -fmessage-length=0 -D_FILE_OFFSET_BITS=64 `pkg-config --cflags gtk+-2.0`
LDFLAGS = -L/usr/local/lib `pkg-config --libs gtk+-2.0` -lrt -pthread -D_FILE_OFFSET_BITS=64 -export-dynamic
SOURCES = utils/bencode.cpp utils/sha1.cpp utils/utils.cpp utils/dir_tree.cpp torrent/peer.cpp torrent/torrent.cpp torrent/torrentfile.cpp torrent/tracker.cpp network/network.cpp gui/gui.cpp fs/cache.cpp fs/fd_lru_cache.cpp fs/file.cpp fs/filemanager.cpp exceptions/exceptions.cpp cfg/glob_cfg.cpp bittorrent/Bittorrent.cpp bittorrent.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE=dinosaur

all:  $(SOURCES) dinosaur

dinosaur: $(OBJECTS)
	$(CC) $(OBJECTS) -o dinosaur $(LDFLAGS)
	rm $(OBJECTS)

.cpp.o: 
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm $(OBJECTS)
