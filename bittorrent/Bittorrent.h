/*
 * Bittorrent.h
 *
 *  Created on: 09.04.2012
 *      Author: ruslan
 */

#pragma once
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pwd.h>
#include <sys/stat.h>
#include "../network/network.h"
#include "../utils/bencode.h"
#include "../torrent/torrent.h"
#include "../cfg/glob_cfg.h"
#include "../fs/fs.h"
#include "../block_cache/Block_cache.h"
#include <set>
#include <map>
#include <vector>
#include <string>

namespace bittorrent {

class Bittorrent : public network::sock_event {
private:
	typedef std::map<std::string, torrent::Torrent *> torrent_map;
	typedef std::map<std::string, torrent::Torrent *>::iterator torrent_map_iter;
	network::NetworkManager m_nm;
	cfg::Glob_cfg m_gcfg;
	fs::FileManager m_fm;
	block_cache::Block_cache m_bc;
	network::socket_ * m_sock;
	torrent_map m_torrents;
	std::string m_directory;
	std::string m_error;
	pthread_t m_thread;
	pthread_mutex_t m_mutex;
	bool m_thread_stop;
	bool m_thread_pause;
	static void * thread(void * arg);
	void bin2hex(unsigned char * bin, char * hex, int len);
	void add_error_mes(std::string & mes);
	int load_our_torrents();
	int init_torrent(std::string & filename, std::string * hash, bool is_new);
	int init_torrent(const char * filename, std::string * hash, bool is_new);
public:
	Bittorrent();
	torrent::Torrent * OpenTorrent(char * filename);
	int AddTorrent(torrent::Torrent * torrent, std::string * hash);
	int StartTorrent(std::string & hash, std::string & download_directory);
	int StopTorrent(std::string & hash);
	int PauseTorrent(std::string & hash);
	int ContinueTorrent(std::string & hash);
	int CheckTorrent(std::string & hash);
	int DeleteTorrent(std::string & hash);
	int Torrent_info(std::string & hash, torrent::torrent_info * info);
	int get_TorrentList(std::list<std::string> * list);
	int get_DownloadDirectory(std::string * dir);
	//uint16_t Torrent_peers(std::string & hash, torrent::peer_info ** peers);
	int event_sock_ready2read(network::socket_ * sock);
	int event_sock_closed(network::socket_ * sock);
	int event_sock_sended(network::socket_ * sock);
	int event_sock_connected(network::socket_ * sock);
	int event_sock_accepted(network::socket_ * sock);
	int event_sock_timeout(network::socket_ * sock);
	int event_sock_unresolved(network::socket_ * sock);
	std::string get_error()
	{
		return m_error;
	}
	~Bittorrent();
};

} /* namespace Bittorrent */

