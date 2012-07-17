/*
 * Torrent.h
 *
 *  Created on: 29.01.2012
 *      Author: ruslan
 */

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <list>
#include <fstream>
#include <sys/stat.h>
#include "../network/network.h"
#include "../utils/bencode.h"
#include "../utils/dir_tree.h"
#include "../cfg/glob_cfg.h"
#include "../fs/fs.h"
#include <pcre.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../consts.h"
#include "../utils/sha1.h"
#include "../block_cache/Block_cache.h"
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "state_serializator.h"
#include "metafile.h"
#include "torrent_types.h"

namespace torrent {


class TorrentBase;
typedef boost::shared_ptr<TorrentBase> 					TorrentBasePtr;

class TorrentInterfaceBase;
typedef boost::shared_ptr<TorrentInterfaceBase> 		TorrentInterfaceBasePtr;

class TorrentInterfaceInternal;
typedef boost::shared_ptr<TorrentInterfaceInternal> 	TorrentInterfaceInternalPtr;

class TorrentFile;
typedef boost::shared_ptr<TorrentFile> TorrentFilePtr;

class Tracker : public network::SocketAssociation
{
private:
	enum TRACKER_STATE
	{
		TRACKER_STATE_NONE,
		TRACKER_STATE_CONNECT,
		TRACKER_STATE_WORK,
		TRACKER_STATE_STOPPING
	};

	//события трекера
	enum TRACKER_EVENT
	{
		TRACKER_EVENT_STARTED,
		TRACKER_EVENT_STOPPED,
		TRACKER_EVENT_COMPLETED,
		TRACKER_EVENT_NONE
	};
private:
	network::NetworkManager * 		m_nm;
	cfg::Glob_cfg * 				m_g_cfg;
	std::string 					m_announce;
	TorrentInterfaceInternalPtr 	m_torrent;
	std::string 					m_host;//доменное имя хоста, где находится трекер, нужен для заголовка HTTP запроса (Host: bla-bla-bla)
	std::string 					m_params;//параметры в анонсе, нужен для формирования URN в HTTP-загловке запроса
	network::Socket 				m_sock;
	sockaddr_in * 					m_addr;
	char							m_infohash[SHA1_LENGTH * 3 + 1];//infohash в url encode
	char 							m_buf[BUFFER_SIZE];//буфер, куда кидаем ответ от сервера
	ssize_t 						m_buflen;//длина ответа в буфере
	TRACKER_STATE 					m_state;
	std::string 					m_status;
	bool 							m_ready2release;
	time_t 							m_last_update;
	uint64_t 						m_downloaded;//с момента события started
	uint64_t 						m_uploaded;//с момента события started
	//ответ трекера
	uint64_t 						m_interval;
	uint64_t 						m_min_interval;
	std::string 					m_tracker_id;
	uint64_t 						m_seeders;
	uint64_t 						m_leechers;
	sockaddr_in * 					m_peers;
	int 							m_peers_count;
	TRACKER_EVENT 					m_event_after_connect;
	//берет хэш у Torrent и делает url encode
	void hash2urlencode();
	//обрабатывает ответ сервера, парсит его
	int process_response();
	//формирует и отправляет запрос
	int send_request(TRACKER_EVENT event );
	int restore_socket();
	int parse_announce();
	int send_completed();
	void delete_socket()
	{
		m_nm->Socket_delete(m_sock);
	}
public:
	Tracker();
	Tracker(const TorrentInterfaceInternalPtr & torrent, std::string & announce);
	virtual ~Tracker();
	int event_sock_ready2read(network::Socket sock);
	int event_sock_closed(network::Socket sock);
	int event_sock_sended(network::Socket sock);
	int event_sock_connected(network::Socket sock);
	int event_sock_accepted(network::Socket sock, network::Socket accepted_sock);
	int event_sock_timeout(network::Socket sock);
	int event_sock_unresolved(network::Socket sock);
	int get_peers_count();
	const sockaddr_in * get_peer(int i);
	int update();
	int prepare2release();
	void forced_releasing();
	int clock(bool & release_me);
	int send_stopped();
	int send_started();
	int get_info(tracker_info * info);
};

class Peer : public network::SocketAssociation
{
private:
	//состояния автомата в классе Peer
	enum PEER_STATE
	{
		PEER_STATE_SEND_HANDSHAKE,
		PEER_STATE_WAIT_HANDSHAKE,
		//PEER_STATE_WAIT_BITFIELD,
		PEER_STATE_GENERAL_READY,
		PEER_STATE_WAIT_UNCHOKE,
		PEER_STATE_REQUEST_READY,
		PEER_STATE_SLEEP
	};
private:
	network::NetworkManager * 		m_nm;
	cfg::Glob_cfg * 				m_g_cfg;
	TorrentInterfaceInternalPtr		m_torrent;
	sockaddr_in 					m_addr;
	network::Socket 				m_sock;
	network::buffer 				m_buf;
	std::string 					m_ip;
	uint64_t 						m_downloaded;
	uint64_t 						m_uploaded;
	std::set<uint32_t> 				m_available_pieces;
	bool 							m_peer_choking;//пир нас заблокирован
	bool 							m_peer_interested;//пир в нас заинтересован
	bool 							m_am_choking;//я блокирую пира
	bool 							m_am_interested;//я заинтересован в пире
	int 							m_state;
	BITFIELD 						m_bitfield;
	std::set<BLOCK_ID> 				m_requested_blocks;//блоки, которые мы запросили
	std::set<BLOCK_ID> 				m_requests_queue;//блоки, которые у нас запросили(очередь запросов)
	time_t 							m_sleep_time;
	int process_messages();
	void goto_sleep();
public:
	Peer();
	int Init(sockaddr_in * addr, const TorrentInterfaceInternalPtr & torrent);
	int Init(network::Socket & sock, const TorrentInterfaceInternalPtr & torrent, PEER_ADD peer_add);
	int event_sock_ready2read(network::Socket sock);
	int event_sock_closed(network::Socket sock);
	int event_sock_sended(network::Socket sock);
	int event_sock_connected(network::Socket sock);
	int event_sock_accepted(network::Socket sock, network::Socket accepted_sock);
	int event_sock_timeout(network::Socket sock);
	int event_sock_unresolved(network::Socket sock);
	int send_have(uint32_t piece_index);
	int send_handshake();
	int send_bitfield();
	int send_choke();
	int send_unchoke();
	int send_interested();
	int send_not_interested();
	int send_request(uint32_t piece, uint32_t block, uint32_t block_length);
	int send_piece(uint32_t piece, uint32_t offset, uint32_t length,  char * block);
	bool have_piece(uint32_t piece_index);
	int clock();
	//int wake_up(network::Socket & sock, PEER_ADD peer_add);
	bool is_sleep();
	bool may_request();
	bool request_limit();
	void erase_requested_block(const BLOCK_ID & block_id);
	bool get_requested_block(BLOCK_ID & block_id);
	double get_rx_speed();
	double get_tx_speed();
	std::string get_ip_str();
	int get_info(peer_info * info);
	int prepare2release();
	void forced_releasing();
	~Peer();
};

typedef boost::shared_ptr<Peer> PeerPtr;
typedef boost::shared_ptr<Tracker> TrackerPtr;

typedef std::map<std::string, TrackerPtr> tracker_map;
typedef tracker_map::iterator tracker_map_iter;

typedef std::map<std::string, PeerPtr> peer_map;
typedef peer_map::iterator peer_map_iter;

typedef std::list<PeerPtr> peer_list;
typedef peer_list::iterator peer_list_iter;

class PieceManager
{
private:
	typedef std::list<uint32_t> uint32_list;
	typedef std::list<uint32_t>::iterator uint32_list_iter;
	struct piece_info
	{
		int 							file_index;//индекс файла, в котором начинается кусок
		uint32_t 						length;//его длина
		uint32_t 						remain;
		uint64_t 						offset;//смещение внутри файла до начала куска
		uint32_t 						block_count;//кол-во блоков в куске
		std::set<PeerPtr> 				taken_from;
		uint32_list			 			block2download;
		SHA1_HASH						hash;
		PIECE_PRIORITY					prio;
		uint32_list_iter				prio_iter;
	};
	struct download_queue
	{
		uint32_list 					low_prio_pieces;
		uint32_list 					normal_prio_pieces;
		uint32_list 					high_prio_pieces;
	};
private:
	TorrentInterfaceInternalPtr 		m_torrent;
	BITFIELD 							m_bitfield;
	size_t 								m_bitfield_len;
	std::set<uint32_t>					m_pieces_to_download;//куски, которые надо загрузить
	std::vector<piece_info> 			m_piece_info;
	unsigned char *						m_piece_for_check_hash;
	CSHA1 								m_csha1;
	download_queue						m_download_queue;
	uint32_list							m_tag_list;//хранит один элемент, на который будут ссылатся prio_iter у загруженных кусков
	void build_piece_info();
public:
	PieceManager(const TorrentInterfaceInternalPtr & torrent, BITFIELD bitfield);
	void reset();
	~PieceManager();
	int get_blocks_count_in_piece(uint32_t piece_index, uint32_t & blocks_count);
	int get_piece_length(uint32_t piece_index, uint32_t & piece_length);
	int get_block_index_by_offset(uint32_t piece_index, uint32_t block_offset, uint32_t & index);
	int get_block_length_by_index(uint32_t piece_index, uint32_t block_index, uint32_t & block_length);
	int get_piece_offset(uint32_t piece_index, uint64_t & offset);
	int get_file_index_by_piece(uint32_t piece_index, int & index);
	size_t get_bitfield_length();
	void copy_bitfield(BITFIELD dst);
	bool check_piece_hash(unsigned char * piece, uint32_t piece_index);
	int front_piece2download(uint32_t & piece_index)
	void pop_piece2download();
	int push_piece2download(uint32_t piece_index);
	int set_piece_priority(uint32_t piece_index, PIECE_PRIORITY priority);
	int get_piece_priority(uint32_t piece_index, PIECE_PRIORITY & priority);

};
typedef boost::shared_ptr<PieceManager> PieceManagerPtr;


class TorrentFile : public fs::FileAssociation
{
private:
	fs::FileManager * 				m_fm;
	TorrentInterfaceInternalPtr 	m_torrent;
	std::vector<file>				m_files;
	TorrentFile(const TorrentInterfaceInternalPtr & t);
	void init(const std::string & path);
public:
	int save_block(uint32_t piece, uint32_t block_offset, uint32_t block_length, char * block);
	int read_block(uint32_t piece, uint32_t block_index, char * block, uint32_t & block_length);
	int read_piece(uint32_t piece_index, unsigned char * dst);
	int event_file_write(fs::write_event * eo);
	void ReleaseFiles();
	~TorrentFile();
	static void CreateTorrentFile(const TorrentInterfaceInternalPtr & t, const std::string & path, TorrentFilePtr & ptr)
	{
		try
		{
			ptr.reset(new TorrentFile(t));
			ptr->init(path);
		}
		catch(Exception & e)
		{
			ptr.reset();
			throw Exception(e);
		}
	}
};


class TorrentBase : public boost::enable_shared_from_this<TorrentBase>
{
private:
	enum TORRENT_STATE
	{
		TORRENT_STATE_NONE,
		TORRENT_STATE_STARTED,
		TORRENT_STATE_STOPPED,
		TORRENT_STATE_PAUSED,
		TORRENT_STATE_CHECKING,
		TORRENT_STATE_RELEASING
	};

	enum TORRENT_TASKS
	{
			TORRENT_TASK_DOWNLOAD_PIECE,
			TORRENT_TASK_CHECK_HASH
	};

	struct TORRENT_TASK
	{
			TORRENT_TASKS 		task;
			uint64_t 			task_data;//здесь будем хранить например номер куска для проверки
	};
protected:
	network::NetworkManager * 		m_nm;
	cfg::Glob_cfg * 				m_g_cfg;
	fs::FileManager * 				m_fm;
	block_cache::Block_cache *		m_bc;
	TorrentFilePtr 					m_torrent_file;
	PieceManagerPtr 				m_piece_manager;
	Metafile 						m_metafile;

	std::string 					m_download_directory;
	std::string 					m_state_file_name;

	tracker_map 					m_trackers;
	peer_map 						m_seeders;
	peer_list 						m_waiting_seeders;
	peer_list 						m_active_seeders;
	peer_map 						m_leechers;
	std::list<uint32_t> 			m_have_list;//список кусков, которые мы только загрузили, и о которых надо известить всех участников раздачи(сообщение have)
	std::list<TORRENT_TASK> 		m_task_queue;
	std::list<uint32_t> 			m_downloadable_pieces;

	uint64_t 						m_downloaded;
	uint64_t 						m_uploaded;
	double 							m_rx_speed;
	double 							m_tx_speed;
	TORRENT_STATE 					m_state;
	std::string 					m_error;

	virtual int handle_download_task();
	virtual void add_seeders(uint32_t count, sockaddr_in * addrs);
	virtual int add_leecher(network::Socket & sock);
	virtual std::string get_error();
	virtual int start();
	virtual int stop();
	virtual int pause();
	virtual int continue_();
	virtual int check();
	virtual bool is_downloaded();
	virtual int event_piece_hash(uint32_t piece_index, bool ok, bool error);
	virtual int clock();
	virtual int get_info(torrent_info * info);
	virtual int erase_state();
	virtual void prepare2release();
	virtual void forced_releasing();
	TorrentBase(network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc);
	void init(const Metafile & metafile, const std::string & work_directory, const std::string & download_directory);
	static void CreateTorrent(network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc,
				const Metafile & metafile, const std::string & work_directory, const std::string & download_directory, TorrentBasePtr & ptr)
	{
		try
		{
			ptr.reset(new TorrentBase(nm, g_cfg, fm, bc));
			ptr->init(metafile, work_directory, download_directory);
		}
		catch (Exception & e)
		{
			ptr.reset();
			throw Exception(e);
		}
	}
public:
	virtual ~TorrentBase();
};

class TorrentInterfaceBase : public TorrentBase
{
public:
	static void CreateTorrent(network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc,
			const Metafile & metafile, const std::string & work_directory, const std::string & download_directory, TorrentInterfaceBasePtr & ptr)
	{
		ptr.reset();
		TorrentBasePtr base_ptr;
		TorrentBase::CreateTorrent(nm, g_cfg, fm, bc, metafile, work_directory, download_directory, base_ptr);
		ptr = boost::static_pointer_cast<TorrentInterfaceBase>(base_ptr);
	}
	virtual ~TorrentInterfaceBase() {}
	virtual void add_seeders(uint32_t count, sockaddr_in * addrs) = 0;
	virtual int add_leecher(network::Socket & sock) = 0;
	virtual std::string get_error() = 0;
	virtual int start() = 0;
	virtual int stop() = 0;
	virtual int pause() = 0;
	virtual int continue_() = 0;
	virtual int check() = 0;
	virtual bool is_downloaded() = 0;
	virtual int clock() = 0;
	virtual int get_info(torrent_info * info) = 0;
	virtual int erase_state() = 0;
	virtual void prepare2release() = 0;
	virtual void forced_releasing() = 0;
};

class TorrentInterfaceInternal : public TorrentBase
{
public:
	network::NetworkManager * get_nm();
	cfg::Glob_cfg * get_cfg();
	fs::FileManager * get_fm();
	block_cache::Block_cache * get_bc();
	uint32_t get_piece_count();
	uint32_t get_piece_length();
	int get_files_count();
	uint64_t get_length();
	std::string get_name();
	uint64_t get_downloaded();
	uint64_t get_uploaded();
	size_t get_bitfield_length();
	dir_tree::DirTree * get_dirtree();
	base_file_info * get_file_info(uint32_t file_index);
	int get_blocks_count_in_piece(uint32_t piece, uint32_t & blocks_count);
	int get_piece_length(uint32_t piece, uint32_t & piece_length);
	int get_block_index_by_offset(uint32_t piece_index, uint32_t block_offset, uint32_t & index);
	int get_block_length_by_index(uint32_t piece_index, uint32_t block_index, uint32_t & block_length);
	int get_piece_offset(uint32_t piece_index, uint64_t & offset);
	int get_file_index_by_piece(uint32_t piece_index, int & index);
	void copy_infohash_bin(SHA1_HASH dst);
	int memcmp_infohash_bin(SHA1_HASH mem);
	void copy_bitfield(BITFIELD dst);
	void inc_uploaded(uint32_t bytes_num);
	void inc_downloaded(uint32_t bytes_num);
	void set_error(std::string err);
	void copy_piece_hash(SHA1_HASH dst, uint32_t piece_index);
	int save_block(uint32_t piece, uint32_t block_offset, uint32_t block_length, char * block);
	int read_block(uint32_t piece, uint32_t block_index, char * block, uint32_t & block_length);
	virtual int event_piece_hash(uint32_t piece_index, bool ok, bool error) = 0;
	virtual void add_seeders(uint32_t count, sockaddr_in * addrs) = 0;
};

void set_bitfield(uint32_t piece, uint32_t piece_count, BITFIELD bitfield);
void reset_bitfield(uint32_t piece, uint32_t piece_count, BITFIELD bitfield);
bool bit_in_bitfield(uint32_t piece, uint32_t piece_count, BITFIELD bitfield);
void get_peer_key(sockaddr_in * addr, std::string & key);

} /* namespace TorrentNamespace */
