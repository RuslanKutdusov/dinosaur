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
#include "../hash_checker/hash_checker.h"
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
	HashChecker::HashChecker m_hc;
	block_cache::Block_cache m_bc;
	network::socket_ * m_sock;
	torrent_map m_torrents;
	std::string m_directory;
	std::string m_error;
	pthread_t m_thread;
	pthread_mutex_t m_mutex;
	bool m_thread_stop;
	bool m_thread_pause;
	static void * thread(void * arg)
	{
		int ret = 1;
		Bittorrent * bt = (Bittorrent*)arg;
		//printf("MAIN_THREAD started\n");
		while(!bt->m_thread_stop)
		{
			if (bt->m_thread_pause)
			{
				sleep(1);
				continue;
			}
			pthread_mutex_lock(&bt->m_mutex);
			bt->m_nm.Wait();
					//nm.test_view_socks();

			network::socket_ * sock = NULL;

			while(bt->m_nm.event_accepted_sock(bt->m_sock, &sock))
			{
				std::cout<<"ACCEPTED\n";
				//network::sock_event * t = (network::sock_event *)bt->m_nm.Socket_get_assoc(sock);
				//std::cout<<t<<" "<<bt<<std::endl;
				//if (t != NULL)
				bt->m_nm.Socket_set_assoc(sock, bt);
			}
			while(bt->m_nm.event_connected_sock(&sock))
			{
				//std::cout<<"CONNECTED: "<<sock->get_fd()<<std::endl;
				network::sock_event * t = (network::sock_event *)bt->m_nm.Socket_get_assoc(sock);
				if (t != NULL)
					t->event_sock_connected(sock);
			}
			while(bt->m_nm.event_ready2read_sock(&sock))
			{
				//std::cout<<"READY2READ: "<<inet_ntoa(sock->m_peer.sin_addr)<<std::endl;
				network::sock_event * t = (network::sock_event *)bt->m_nm.Socket_get_assoc(sock);
				if (t != NULL)
					t->event_sock_ready2read(sock);

			}
			while(bt->m_nm.event_closed_sock(&sock))
			{
				//std::cout<<"CLOSED: "<<sock->get_fd()<<std::endl;
				network::sock_event * t = (network::sock_event *)bt->m_nm.Socket_get_assoc(sock);
				if (t != NULL)
					t->event_sock_closed(sock);
			}
			while(bt->m_nm.event_sended_socks(&sock))
			{
				//std::cout<<"SENDED "<<sock->get_fd()<<std::endl;
				network::sock_event * t = (network::sock_event *)bt->m_nm.Socket_get_assoc(sock);
				if (t != NULL)
					t->event_sock_sended(sock);
			}

			while(bt->m_nm.event_timeout_on(&sock))
			{
				//std::cout<<"TIMEOUT "<<sock->get_fd()<<std::endl;
				network::sock_event * t = (network::sock_event *)bt->m_nm.Socket_get_assoc(sock);
				if (t != NULL)
					t->event_sock_timeout(sock);
			}

			while(bt->m_nm.event_unresolved_sock(&sock))
			{
				//std::cout<<"TIMEOUT "<<sock->get_fd()<<std::endl;
				network::sock_event * t = (network::sock_event *)bt->m_nm.Socket_get_assoc(sock);
				if (t != NULL)
					t->event_sock_unresolved(sock);
			}
			//bt->m_nm.test_view_socks();
			fs::write_event eo;
			eo.assoc = NULL;
			if (bt->m_fm.get_write_event(&eo))
			{
				fs::file_event * f = (fs::file_event *)eo.assoc;
				if (f != NULL)
				{
					f->event_file_write(&eo);
				}
				else
					std::cout<<"f == NULL\n";
			}
			HashChecker::event_on hc_eo;
			hc_eo.hc_e = NULL;
			if (bt->m_hc.get_event(&hc_eo))
			{
				HashChecker::HashChecker_event * hc_e = hc_eo.hc_e;
				if (hc_e != NULL)
					hc_e->event_piece_hash(hc_eo.piece_index, hc_eo.ok, hc_eo.error);
			}
			for(torrent_map_iter iter = bt->m_torrents.begin(); iter != bt->m_torrents.end(); ++iter)
			{
				(*iter).second->clock();
			}
			usleep(10);
			pthread_mutex_unlock(&bt->m_mutex);
		}
		//printf("MAIN_THREAD stopped\n");
		return (void*)ret;
	}
	void bin2hex(unsigned char * bin, char * hex, int len);
	void add_error_mes(std::string & mes);
	int load_our_torrents();
	int init_torrent(std::string & filename, std::string * hash);
	int init_torrent(const char * filename, std::string * hash);
public:
	Bittorrent();
	int AddTorrent(char * filename, std::string * hash);
	int StartTorrent(std::string & hash, std::string & download_directory);
	int StopTorrent(std::string & hash);
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
	~Bittorrent();
};

} /* namespace Bittorrent */

