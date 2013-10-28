#-------------------------------------------------
#
# Project created by QtCreator 2013-10-28T23:33:18
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dinosaur
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    libdinosaur/sha1_hash.cpp \
    libdinosaur/dinosaur.cpp \
    libdinosaur/cfg/glob_cfg.cpp \
    libdinosaur/dht/searcher.cpp \
    libdinosaur/dht/routing_table.cpp \
    libdinosaur/dht/node_id.cpp \
    libdinosaur/dht/dht.cpp \
    libdinosaur/exceptions/exceptions.cpp \
    libdinosaur/fs/filemanager.cpp \
    libdinosaur/fs/file.cpp \
    libdinosaur/fs/fd_lru_cache.cpp \
    libdinosaur/fs/cache.cpp \
    libdinosaur/log/log.cpp \
    libdinosaur/network/network.cpp \
    libdinosaur/network/domain_name_resolver.cpp \
    libdinosaur/torrent/tracker.cpp \
    libdinosaur/torrent/torrentfile.cpp \
    libdinosaur/torrent/torrent.cpp \
    libdinosaur/torrent/torrent_utils.cpp \
    libdinosaur/torrent/piece_manager.cpp \
    libdinosaur/torrent/peer.cpp \
    libdinosaur/torrent/metafile.cpp \
    libdinosaur/torrent/interfaces.cpp \
    libdinosaur/utils/utils.cpp \
    libdinosaur/utils/sha1.cpp \
    libdinosaur/utils/dir_tree.cpp \
    libdinosaur/utils/bencode.cpp

HEADERS  += mainwindow.hpp \
    libdinosaur/types.h \
    libdinosaur/dinosaur.h \
    libdinosaur/consts.h \
    libdinosaur/block_cache/Block_cache.h \
    libdinosaur/cfg/glob_cfg.h \
    libdinosaur/dht/dht.h \
    libdinosaur/err/err_code.h \
    libdinosaur/exceptions/exceptions.h \
    libdinosaur/fs/fs.h \
    libdinosaur/fs/fs_tests.h \
    libdinosaur/log/log.h \
    libdinosaur/lru_cache/lru_cache.h \
    libdinosaur/network/network.h \
    libdinosaur/torrent/torrent.h \
    libdinosaur/torrent/torrent_types.h \
    libdinosaur/torrent/state_serializator.h \
    libdinosaur/torrent/metafile.h \
    libdinosaur/utils/utils.h \
    libdinosaur/utils/sha1.h \
    libdinosaur/utils/dir_tree.h \
    libdinosaur/utils/bencode.h

FORMS    += mainwindow.ui

QMAKE_CXXFLAGS += -std=c++11 -D_FILE_OFFSET_BITS=64
QMAKE_LFLAGS += -pthread -lpcrecpp -lpcre -lboost_date_time -lboost_serialization
