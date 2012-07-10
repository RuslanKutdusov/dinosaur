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

namespace torrent {

enum PEER_ADD
{
	PEER_ADD_TRACKER,
	PEER_ADD_INCOMING
};

struct peer_info
{
	char ip[22];
	uint64_t downloaded;
	uint64_t uploaded;
	double downSpeed;
	double upSpeed;
	double available;
};

class tracker_info
{
public:
	std::string announce;
	std::string status;
	uint64_t seeders;
	uint64_t leechers;
	time_t update_in;
};

class TorrentBase;
typedef boost::shared_ptr<TorrentBase> TorrentBasePtr;
class TorrentInterfaceBase;
typedef boost::shared_ptr<TorrentInterfaceBase> TorrentInterfaceBasePtr;
class TorrentInterfaceForPeer;
typedef boost::shared_ptr<TorrentInterfaceForPeer> TorrentInterfaceForPeerPtr;
class TorrentInterfaceForTracker;
typedef boost::shared_ptr<TorrentInterfaceForTracker> TorrentInterfaceForTrackerPtr;
class TorrentInterfaceForTorrentFile;
typedef boost::shared_ptr<TorrentInterfaceForTorrentFile> TorrentInterfaceForTorrentFilePtr;

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
	network::NetworkManager * m_nm;
	cfg::Glob_cfg * m_g_cfg;
	std::string m_announce;
	TorrentInterfaceForTrackerPtr m_torrent;
	std::string m_host;//доменное имя хоста, где находится трекер, нужен для заголовка HTTP запроса (Host: bla-bla-bla)
	std::string m_params;//параметры в анонсе, нужен для формирования URN в HTTP-загловке запроса
	network::Socket m_sock;
	sockaddr_in * m_addr;
	char m_infohash[SHA1_LENGTH * 3 + 1];//infohash в url encode
	char m_buf[BUFFER_SIZE];//буфер, куда кидаем ответ от сервера
	ssize_t m_buflen;//длина ответа в буфере
	TRACKER_STATE m_state;
	std::string m_status;
	bool m_ready2release;
	time_t m_last_update;
	uint64_t m_downloaded;//с момента события started
	uint64_t m_uploaded;//с момента события started
	//ответ трекера
	uint64_t m_interval;
	uint64_t m_min_interval;
	std::string m_tracker_id;
	uint64_t m_seeders;
	uint64_t m_leechers;
	sockaddr_in * m_peers;
	int m_peers_count;
	//берет хэш у Torrent и делает url encode
	void hash2urlencode();
	//обрабатывает ответ сервера, парсит его
	int process_response();
	//формирует и отправляет запрос
	int send_request(TRACKER_EVENT event );
	//удаление сокета
	TRACKER_EVENT m_event_after_connect;
	int restore_socket();
	int parse_announce();
	int send_completed();
	void delete_socket()
	{
		m_nm->Socket_delete(m_sock);
	}
public:
	Tracker();
	Tracker(const TorrentInterfaceForTrackerPtr & torrent, std::string & announce);
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
	network::NetworkManager * m_nm;
	cfg::Glob_cfg * m_g_cfg;
	TorrentInterfaceForPeerPtr m_torrent;
	sockaddr_in m_addr;
	network::Socket m_sock;
	network::buffer m_buf;
	std::string m_ip;
	uint64_t m_downloaded;
	uint64_t m_uploaded;
	std::set<uint32_t> m_available_pieces;
	bool m_peer_choking;//пир нас заблокирован
	bool m_peer_interested;//пир в нас заинтересован
	bool m_am_choking;//я блокирую пира
	bool m_am_interested;//я заинтересован в пире
	int m_state;
	unsigned char * m_bitfield;
	std::set<uint64_t> m_requested_blocks;//блоки, которые мы запросили
	std::set<uint64_t> m_requests_queue;//блоки, которые у нас запросили(очередь запросов)
	int process_messages();
	void goto_sleep();
	time_t m_sleep_time;
public:
	Peer();
	int Init(sockaddr_in * addr, const TorrentInterfaceForPeerPtr & torrent);
	int Init(network::Socket & sock, const TorrentInterfaceForPeerPtr & torrent, PEER_ADD peer_add);
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
	bool no_requested_blocks();
	void erase_requested_block(uint64_t block);
	uint64_t get_requested_block();
	double get_rx_speed();
	double get_tx_speed();
	std::string get_ip_str();
	int get_info(peer_info * info);
	int prepare2release()
	{
		m_nm->Socket_delete(m_sock);
		return ERR_NO_ERROR;
	}
	~Peer();
};

struct file_info
{
	uint64_t length;
	char *  name;
	bool download;
	fs::File file;
};

struct piece_offset
{
	int file_index;
	uint32_t length;
	uint64_t offset;
};

struct piece_info
{
	int file_index;//индекс файла, в котором начинается кусок
	uint32_t length;//его длина
	uint64_t offset;//смещение внутри файла до начала куска
	uint32_t block_count;//кол-во блоков в куске
};

typedef boost::shared_ptr<Peer> PeerPtr;
typedef boost::shared_ptr<Tracker> TrackerPtr;
typedef std::map<std::string, TrackerPtr> tracker_map;
typedef tracker_map::iterator tracker_map_iter;
typedef std::map<std::string, PeerPtr> peer_map;
typedef peer_map::iterator peer_map_iter;

typedef std::list<PeerPtr> peer_list;
typedef peer_list::iterator peer_list_iter;

class TorrentFile : public fs::FileAssociation
{
private:
	fs::FileManager * m_fm;
	TorrentInterfaceForTorrentFilePtr m_torrent;
	file_info * m_files;
	uint64_t m_length;
	uint32_t m_files_count;
	uint32_t m_pieces_count;
	uint64_t m_piece_length;
	unsigned char ** m_pieces;
	char * m_piece_for_check_hash;
	CSHA1 m_csha1;
	void build_piece_offset_table();
public:
	std::vector<piece_info> m_piece_info;
	TorrentFile();
	int Init(const TorrentInterfaceForTorrentFilePtr & t, std::string & path, bool _new);
	int save_block(uint32_t piece, uint32_t block_offset, uint32_t block_length, char * block);
	int read_block(uint32_t piece, uint32_t block_index, char * block, uint32_t * block_length);
	int read_piece(uint32_t piece_index);
	bool check_piece_hash(uint32_t piece_index);
	int event_file_write(fs::write_event * eo);
	uint32_t get_blocks_count_in_piece(uint32_t piece);
	uint32_t get_piece_length(uint32_t piece);
	int get_block_index_by_offset(uint32_t piece_index, uint32_t block_offset, uint32_t * index);
	int get_block_length_by_index(uint32_t piece_index, uint32_t block_index, uint32_t * block_length);
	void ReleaseFiles();
	~TorrentFile();
};
typedef boost::shared_ptr<TorrentFile> TorrentFilePtr;

class torrent_info
{
public:
	typedef std::list<peer_info> peer_info_list;
	typedef peer_info_list::iterator peer_info_iter;
	typedef std::list<tracker_info> tracker_info_list;
	typedef tracker_info_list::iterator tracker_info_iter;
	typedef std::pair<std::string, uint64_t> file;
	typedef std::list<file> file_list;
	typedef file_list::iterator file_list_iter;
public:
	std::string name;
	std::string comment;
	std::string created_by;
	std::string download_directory;
	uint64_t creation_date;
	uint64_t private_;
	uint64_t length;
	uint64_t piece_length;
	uint32_t piece_count;
	uint64_t downloaded;
	uint64_t uploaded;
	double rx_speed;
	double tx_speed;
	peer_info_list peers;
	tracker_info_list trackers;
	file_list file_list_;
	char info_hash_hex[SHA1_LENGTH * 2 + 1];
	int progress;
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
		TORRENT_STATE_CHECKING
	};

	enum TORRENT_TASKS
	{
			TORRENT_TASK_DOWNLOAD_PIECE,
			TORRENT_TASK_CHECK_HASH
	};

	struct TORRENT_TASK
	{
			TORRENT_TASKS task;
			uint64_t task_data;//здесь будем хранить например номер куска для проверки
	};

	//так выглядит файл состояний
	struct state_file
	{
		int version;
		char download_directory[MAX_FILENAME_LENGTH];
		//uint64_t downloaded;
		uint64_t uploaded;
		char future_use[256];
		char bitfield[131072];
	};
protected:
	network::NetworkManager * m_nm;
	bencode::be_node * m_metafile;
	uint64_t m_metafile_len;
	cfg::Glob_cfg * m_g_cfg;
	fs::FileManager * m_fm;
	block_cache::Block_cache * m_bc;
	std::string m_work_directory;
	std::string m_download_directory;
	std::string m_state_file_name;
	std::vector<std::string> m_announces;
	std::string m_comment;
	std::string m_created_by;
	uint64_t m_creation_date;
	uint64_t m_private;
	uint64_t m_length;
	file_info * m_files;
	int m_files_count;
	dir_tree::DirTree * m_dir_tree;//почему ссылка, а не просто член класса?????????????????????????????????????????????????///
	std::string m_name;
	uint64_t m_piece_length;
	uint32_t m_piece_count;
	char * m_pieces;
	unsigned char m_info_hash_bin[SHA1_LENGTH];
	char m_info_hash_hex[SHA1_LENGTH * 2 + 1];
	tracker_map m_trackers;
	peer_map m_seeders;
	peer_list m_waiting_seeders;
	peer_list m_active_seeders;
	peer_map m_leechers;
	TorrentFilePtr m_torrent_file;
	bool m_new;
	uint64_t m_downloaded;
	uint64_t m_uploaded;
	unsigned char * m_bitfield;
	size_t m_bitfield_len;
	std::set<uint32_t> m_pieces_to_download;//куски, которые надо загрузить
	std::list<uint32_t> m_have_list;//список кусков, которые мы только загрузили, и о которых надо известить всех участников раздачи(сообщение have)
	std::list<TORRENT_TASK> m_task_queue;
	double m_rx_speed;
	double m_tx_speed;
	TORRENT_STATE m_state;
	std::string m_error;
	virtual void init_members(network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc, std::string & work_directory);
	virtual int get_announces_from_metafile();
	virtual void get_additional_info_from_metafile();
	virtual int get_files_info_from_metafile(bencode::be_node * info);
	virtual int get_main_info_from_metafile( uint64_t metafile_len);
	virtual int calculate_info_hash(bencode::be_node * info, uint64_t metafile_len);
	virtual void init_bitfield();
	virtual void release();
	virtual int read_state_file(state_file * sf);
	virtual int save_state_file(state_file * sf);
	virtual int get_state();
	virtual int save_state();
	virtual int handle_download_task();
	virtual void add_seeders(uint32_t count, sockaddr_in * addrs);
	virtual int add_leecher(network::Socket & sock);
	virtual int Init(std::string metafile, network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc,
			std::string & work_directory, bool is_new);
	virtual std::string get_error();
	virtual int start(std::string & download_directory);
	virtual int stop();
	virtual int pause();
	virtual int continue_();
	virtual int check();
	virtual bool is_downloaded();
	virtual int event_piece_hash(uint32_t piece_index, bool ok, bool error);
	virtual int clock();
	virtual int get_info(torrent_info * info);
	virtual int save_meta2file(const char * filepath);
	virtual int erase_state();
	virtual void prepare2release();
public:
	TorrentBase();
	virtual ~TorrentBase();
};

class TorrentInterfaceBase : public TorrentBase
{
public:
	TorrentInterfaceBase()
	{}
	virtual ~TorrentInterfaceBase() {}
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
	void copy_infohash_bin(char * dst);
	void copy_infohash_bin(unsigned char * dst);
	virtual int add_leecher(network::Socket & sock) = 0;
	virtual int Init(std::string metafile, network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc,
			std::string & work_directory, bool is_new) = 0;
	virtual std::string get_error() = 0;
	virtual int start(std::string & download_directory) = 0;
	virtual int stop() = 0;
	virtual int pause() = 0;
	virtual int continue_() = 0;
	virtual int check() = 0;
	virtual bool is_downloaded() = 0;
	virtual int clock() = 0;
	virtual int get_info(torrent_info * info) = 0;
	virtual int save_meta2file(const char * filepath) = 0;
	virtual int erase_state() = 0;
	virtual void prepare2release() = 0;
};

class TorrentInterfaceForPeer : public TorrentInterfaceBase
{
public:
	virtual ~TorrentInterfaceForPeer() {}
	size_t get_bitfield_length();
	void get_torrentfile(TorrentFilePtr & ptr);
	int memcmp_infohash_bin(char * mem);
	void copy_bitfield(char * dst);
	void inc_uploaded(uint32_t bytes_num);
};

class TorrentInterfaceForTracker : public TorrentInterfaceBase
{
public:
	virtual ~TorrentInterfaceForTracker() {}
	virtual void add_seeders(uint32_t count, sockaddr_in * addrs) = 0;
};

class TorrentInterfaceForTorrentFile : public TorrentInterfaceBase
{
public:
	virtual ~TorrentInterfaceForTorrentFile() {}
	void set_error(std::string err);
	dir_tree::DirTree * get_dirtree();
	void copy_piece_hash(unsigned char * dst, uint32_t piece_index);
	file_info * get_file_info(int file_index);
	virtual int event_piece_hash(uint32_t piece_index, bool ok, bool error) = 0;
};

void set_bitfield(uint32_t piece, uint32_t piece_count, unsigned char * bitfield);
void reset_bitfield(uint32_t piece, uint32_t piece_count, unsigned char * bitfield);
bool bit_in_bitfield(uint32_t piece, uint32_t piece_count, unsigned char * bitfield);
void get_peer_key(sockaddr_in * addr, std::string * key);
} /* namespace TorrentNamespace */
