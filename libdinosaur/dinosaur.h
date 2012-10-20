/*
 * Dinosaur.h
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
#include "network/network.h"
#include "utils/bencode.h"
#include "torrent/torrent.h"
#include "torrent/metafile.h"
#include "cfg/glob_cfg.h"
#include "fs/fs.h"
#include "block_cache/Block_cache.h"
#include <set>
#include <map>
#include <vector>
#include <string>
#include <list>
#include <boost/shared_ptr.hpp>
#include "log/log.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/list.hpp>
#include <execinfo.h>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace dinosaur {

class Dinosaur;
typedef boost::shared_ptr<Dinosaur> DinosaurPtr;

class Dinosaur : public network::SocketAssociation {
private:
	struct torrent_info
	{
		std::string 				metafile_path;
		bool						in_queue;
		bool						paused_by_user;
		friend class boost::serialization::access;
	private:
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & metafile_path;
			ar & in_queue;
			ar & paused_by_user;
		}
	};
private:
	typedef std::list<std::string> string_list;
	struct torrent_queue
	{
		string_list 				active;
		string_list 				in_queue;
	};
	struct torrent_map_element
	{
		bool paused_by_user;
		torrent::TorrentInterfaceBasePtr ptr;
	};
	typedef std::map<std::string, torrent_map_element> torrent_map;
	typedef torrent_map::iterator torrent_map_iter;
	network::NetworkManager 		m_nm;
	fs::FileManager 				m_fm;
	block_cache::Block_cache 		m_bc;
	network::Socket 				m_sock;
	socket_status					m_sock_status;
	torrent_map 					m_torrents;
	torrent_queue					m_torrent_queue;
	std::string 					m_directory;
	pthread_t 						m_thread;
	pthread_mutex_t					m_mutex;
	bool 							m_thread_stop;
	torrent_failures				m_fails_torrents;//while creating Dinosaur
	static void * thread(void * arg);
	void bin2hex(unsigned char * bin, char * hex, int len);
	void load_our_torrents();
	void init_torrent(const torrent::Metafile & metafile, const std::string & download_directory, std::string & hash);
	Dinosaur();
	void init_listen_socket();
	std::list<torrent_info>			m_serializable;
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
	void event_sock_ready2read(network::Socket sock);
	void event_sock_error(network::Socket sock, int errno_);
	void event_sock_sended(network::Socket sock);
	void event_sock_connected(network::Socket sock);
	void event_sock_accepted(network::Socket sock, network::Socket accepted_sock);
	void event_sock_timeout(network::Socket sock);
	void event_sock_unresolved(network::Socket sock);
public:
	cfg::Glob_cfg Config;
	void AddTorrent(torrent::Metafile & metafile, const std::string & download_directory, std::string & hash);
	void PauseTorrent(const std::string & hash);
	void ContinueTorrent(const std::string & hash);
	void CheckTorrent(const std::string & hash);
	void DeleteTorrent(const std::string & hash, bool with_data);
	void get_torrent_info_stat(const std::string & hash, info::torrent_stat & ref);
	void get_torrent_info_dyn(const std::string & hash, info::torrent_dyn & ref);
	void get_torrent_info_trackers(const std::string & hash, info::trackers & ref);
	void get_torrent_info_file_stat(const std::string & hash, FILE_INDEX index, info::file_stat & ref);
	void get_torrent_info_file_dyn(const std::string & hash, FILE_INDEX index, info::file_dyn & ref);
	void get_torrent_info_seeders(const std::string & hash, info::peers & ref);
	void get_torrent_info_leechers(const std::string & hash, info::peers & ref);
	void get_torrent_info_downloadable_pieces(const std::string & hash, info::downloadable_pieces & ref);
	void get_torrent_failure_desc(const std::string & hash, torrent_failure & ref);
	void set_file_priority(const std::string & hash, FILE_INDEX file, DOWNLOAD_PRIORITY prio);
	void get_TorrentList(std::list<std::string>  & ref);
	void get_active_torrents(std::list<std::string>  & ref);
	void get_torrents_in_queue(std::list<std::string>  & ref);
	void UpdateConfigs();
	const socket_status & get_socket_status()
	{
		return m_sock_status;
	}
	void DeleteSocket()
	{
		try
		{
			m_nm.Socket_delete(m_sock);
		}
		catch(Exception & e)
		{

		}
		m_sock_status.listen = false;
	}
	~Dinosaur();
	/*
	 * Exception::ERR_CODE_CAN_NOT_CREATE_THREAD
	 * SyscallException
	 */
	static void CreateDinosaur(DinosaurPtr & ptr, torrent_failures & fail_torrents )
	{
		try
		{
			ptr.reset(new Dinosaur());
			fail_torrents = ptr->m_fails_torrents;
			pthread_mutex_lock(&ptr->m_mutex);
			ptr->init_listen_socket();
			pthread_mutex_unlock(&ptr->m_mutex);
		}
		catch (SyscallException & e)
		{
			ptr.reset();
			throw e;
		}
		catch(Exception & e)
		{
			ptr.reset();
			throw e;
		}
	}
	static void DeleteDinosaur(DinosaurPtr & ptr)
	{
		try
		{
			ptr->m_nm.Socket_delete(ptr->m_sock);
		}
		catch (Exception & e) {

		}
		ptr.reset();
	}
};

} /* namespace Dinosaur */

