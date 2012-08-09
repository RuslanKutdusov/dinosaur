CC = g++
CFLAGS = -O0 -Wall -c -fmessage-length=0 -D_FILE_OFFSET_BITS=64 `pkg-config --cflags gtk+-2.0`
DEBUG = -DBITTORRENT_DEBUG -DFS_DEBUG -DPEER_DEBUG
LDFLAGS = -L/usr/local/lib `pkg-config --libs gtk+-2.0` -lrt -lboost_serialization -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lglib-2.0 -pthread -D_FILE_OFFSET_BITS=64 -export-dynamic
SOURCES = utils/bencode.cpp utils/sha1.cpp utils/utils.cpp utils/dir_tree.cpp torrent/interfaces.cpp torrent/metafile.cpp torrent/piece_manager.cpp torrent/torrent_utils.cpp torrent/peer.cpp torrent/torrent.cpp torrent/torrentfile.cpp torrent/tracker.cpp network/domain_name_resolver.cpp network/network.cpp gui/gui.cpp gui/cfg.cpp fs/cache.cpp fs/fd_lru_cache.cpp fs/file.cpp fs/filemanager.cpp exceptions/exceptions.cpp cfg/glob_cfg.cpp dinosaur.cpp bittorrent.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE=dinosaur

all:  $(SOURCES) dinosaur

debug: CFLAGS += $(DEBUG)
debug:	CFLAGS += -g3
debug:	$(OBJECTS)
	$(CC) $(OBJECTS) -o dinosaur $(LDFLAGS)
	rm $(OBJECTS)

dinosaur: $(OBJECTS)
	$(CC) $(OBJECTS) -o dinosaur $(LDFLAGS)
	rm $(OBJECTS)

.cpp.o: 
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm $(OBJECTS)
