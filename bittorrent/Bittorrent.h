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
#include "../torrent/metafile.h"
#include "../cfg/glob_cfg.h"
#include "../fs/fs.h"
#include "../block_cache/Block_cache.h"
#include <set>
#include <map>
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

namespace bittorrent {

class Bittorrent;
typedef boost::shared_ptr<Bittorrent> BittorrentPtr;

class Bittorrent : public network::SocketAssociation {
private:
	typedef std::map<std::string, torrent::TorrentInterfaceBasePtr> torrent_map;
	typedef torrent_map::iterator torrent_map_iter;
	network::NetworkManager m_nm;
	cfg::Glob_cfg m_gcfg;
	fs::FileManager m_fm;
	block_cache::Block_cache m_bc;
	network::Socket m_sock;
	torrent_map m_torrents;
	std::string m_directory;
	std::string m_error;
	pthread_t m_thread;
	pthread_mutex_t m_mutex;
	bool m_thread_stop;
	bool m_thread_pause;
	static void * thread(void * arg);
	void bin2hex(unsigned char * bin, char * hex, int len);
	void add_error_mes(const std::string & mes);
	int load_our_torrents();
	int init_torrent(const torrent::Metafile & metafile, const std::string & download_directory, std::string & hash);
	Bittorrent();
	void init_listen_socket();
public:
	int AddTorrent(torrent::Metafile & metafile, const std::string & download_directory, std::string & hash);
	int StartTorrent(const std::string & hash);
	int StopTorrent(const std::string & hash);
	int PauseTorrent(const std::string & hash);
	int ContinueTorrent(const std::string & hash);
	int CheckTorrent(const std::string & hash);
	int DeleteTorrent(const std::string & hash);
	int Torrent_info(const std::string & hash, torrent::torrent_info * info);
	int get_TorrentList(std::list<std::string> * list);
	const std::string & get_DownloadDirectory();
	//uint16_t Torrent_peers(std::string & hash, torrent::peer_info ** peers);
	int event_sock_ready2read(network::Socket sock);
	int event_sock_closed(network::Socket sock);
	int event_sock_sended(network::Socket sock);
	int event_sock_connected(network::Socket sock);
	int event_sock_accepted(network::Socket sock, network::Socket accepted_sock);
	int event_sock_timeout(network::Socket sock);
	int event_sock_unresolved(network::Socket sock);
	std::string get_error()
	{
		return m_error;
	}
	void DeleteSocket()
	{
		m_nm.Socket_delete(m_sock);
	}
	~Bittorrent();
	static void CreateBittorrent(BittorrentPtr & ptr)
	{
		try
		{
			ptr.reset(new Bittorrent());
			pthread_mutex_lock(&ptr->m_mutex);
			ptr->init_listen_socket();
			pthread_mutex_unlock(&ptr->m_mutex);
		}
		catch (Exception & e)
		{
			ptr.reset();
			throw e;
		}
	}
};

} /* namespace Bittorrent */

