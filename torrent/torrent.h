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


//события трекера
enum TRACKER_EVENT
{
	TRACKER_EVENT_STARTED,
	TRACKER_EVENT_STOPPED,
	TRACKER_EVENT_COMPLETED,
	TRACKER_EVENT_NONE
};
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

enum PEER_ADD
{
	PEER_ADD_TRACKER,
	PEER_ADD_INCOMING
};

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

class Torrent;

typedef std::map<network::socket_ *, network::sock_event *> socket_map;
typedef std::map<network::socket_ *, network::sock_event *>::iterator socket_map_iter;

class Tracker : public network::sock_event
{
private:
	network::NetworkManager * m_nm;
	cfg::Glob_cfg * m_g_cfg;
	std::string m_announce;
	Torrent * m_torrent;
	std::string m_host;//доменное имя хоста, где находится трекер, нужен для заголовка HTTP запроса (Host: bla-bla-bla)
	std::string m_params;//параметры в анонсе, нужен для формирования URN в HTTP загловке запроса
	network::socket_ * m_sock;
	char m_infohash[SHA1_LENGTH * 3];//infohash в url encode
	char m_buf[BUFFER_SIZE];//буфер, куда кидаем ответ от сервера
	ssize_t m_buflen;//длина ответа в буфере
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
	std::string m_status;
	//берет хэш у Torrent и делает url encode
	void hash2urlencode();
	//обрабатывает ответ сервера, парсит его
	int process_response();
	//формирует и отправляет запрос
	int send_request(TRACKER_EVENT event );
	//удаление сокета
	TRACKER_EVENT m_event_after_connect;
	int restore_socket();
	void delete_socket();
	int parse_announce();
public:
	Tracker();
	Tracker(Torrent * torrent, std::string & announce);
	virtual ~Tracker();
	network::socket_ * get_sock();
	int event_sock_ready2read(network::socket_ * sock);
	int event_sock_closed(network::socket_ * sock);
	int event_sock_sended(network::socket_ * sock);
	int event_sock_connected(network::socket_ * sock);
	int event_sock_accepted(network::socket_ * sock);
	int event_sock_timeout(network::socket_ * sock);
	int event_sock_unresolved(network::socket_* sock);
	int get_peers_count();
	const sockaddr_in * get_peer(int i);
	int update();
	int send_completed();
	int send_stopped();
	int send_started();
	int clock();
	int get_info(tracker_info * info);
	void test_parse_announce(std::string & announce)
	{
		m_announce = announce;
		if (parse_announce() == ERR_NO_ERROR)
		{
			std::cout<<m_host<<std::endl;
			std::cout<<m_params<<std::endl;
		}
		else
			std::cout<<"error"<<std::endl;
	}
};

class Peer : public network::sock_event
{
private:
	network::NetworkManager * m_nm;
	cfg::Glob_cfg * m_g_cfg;
	Torrent * m_torrent;
	sockaddr_in m_addr;
	network::socket_ * m_sock;
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
	//bool m_connection_closed;
	unsigned char * m_bitfield;
	std::set<uint64_t> m_requested_blocks;//блоки, которые мы запросили
	std::set<uint64_t> m_requests_queue;//блоки, которые у нас запросили(очередь запросов)
	int process_messages();
	void init(network::socket_ * sock, Torrent * torrent, PEER_ADD peer_add);
	time_t m_sleep_time;
public:
	Peer();
	Peer(sockaddr_in * addr, Torrent * torrent, PEER_ADD peer_add = PEER_ADD_TRACKER);
	Peer(network::socket_ * sock, Torrent * torrent, PEER_ADD peer_add = PEER_ADD_TRACKER);
	network::socket_ * get_sock();
	int event_sock_ready2read(network::socket_ * sock);
	int event_sock_closed(network::socket_ * sock);
	int event_sock_sended(network::socket_ * sock);
	int event_sock_connected(network::socket_ * sock);
	int event_sock_accepted(network::socket_ * sock);
	int event_sock_timeout(network::socket_ * sock);
	int event_sock_unresolved(network::socket_* sock);
	//int download_piece(uint32_t piece_index);
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
	void goto_sleep();
	int wake_up(network::socket_ * sock, PEER_ADD peer_add);
	bool is_sleep()
	{
		return m_state == PEER_STATE_SLEEP;
	}
	bool may_request();
	bool request_limit();
	bool no_requested_blocks();
	void erase_requested_block(uint64_t block);
	uint64_t get_requested_block();
	double get_rx_speed();
	double get_tx_speed();
	std::string get_ip_str();
	int get_info(peer_info * info);
	~Peer();
};

struct file_info
{
	uint64_t length;
	char *  name;
	bool download;
	int fm_id;
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
	uint32_t remain;//сколько осталось скачать
	uint32_t block_count;//кол-во блоков в куске
	std::map<uint32_t, Peer*> blocks2download;//метки блоков, блок->пир,у которого скачиваем
	uint32_t marked_blocks;//кол-во помеченных файлов
};

class TorrentFile
{
private:
	fs::FileManager * m_fm;
	Torrent * m_torrent;
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
	int Init(Torrent * t, std::string & path, bool _new);
	int save_block(uint32_t piece, uint32_t block_offset, uint32_t block_length, char * block);
	int read_block(uint32_t piece, uint32_t block_index, char * block, uint32_t * block_length);
	int read_piece(uint32_t piece_index);
	bool check_piece_hash(uint32_t piece_index);
	bool piece_is_done(uint32_t piece_index);
	int blocks_in_progress(uint32_t piece_index);
	int event_file_write(fs::write_event * eo);
	uint32_t get_blocks_count_in_piece(uint32_t piece);
	uint32_t get_piece_length(uint32_t piece);
	int blocks2download(uint32_t piece_index, uint32_t * block, uint32_t array_len);
	int mark_block(uint32_t piece_index, uint32_t block_index, Peer * peer);
	int unmark_block(uint32_t piece_index, uint32_t block_index);
	int unmark_if_match(uint32_t piece_index, uint32_t block_index, Peer * peer);
	int restore_piece2download(uint32_t piece_index);
	Peer * get_block_mark(uint32_t piece_index, uint32_t block_index);
	int block_done(uint32_t piece_index, uint32_t block_index);
	int get_block_index_by_offset(uint32_t piece_index, uint32_t block_offset, uint32_t * index);
	int get_block_length_by_index(uint32_t piece_index, uint32_t block_index, uint32_t * block_length);
	~TorrentFile();
};

typedef std::map<std::string, Tracker*> tracker_map;
typedef std::map<std::string, Tracker*>::iterator tracker_map_iter;
typedef std::map<std::string, Peer*> peer_map;
typedef std::map<std::string, Peer*>::iterator peer_map_iter;

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
};

class Torrent : public network::sock_event, public fs::file_event
{
private:
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
	dir_tree::DirTree * m_dir_tree;
	std::string m_name;
	uint64_t m_piece_length;
	uint32_t m_piece_count;
	char * m_pieces;
	unsigned char m_info_hash_bin[SHA1_LENGTH];
	char m_info_hash_hex[SHA1_LENGTH * 2 + 1];
	tracker_map m_trackers;
	socket_map m_sockets;//по сокету ассоциируемся с объектом sock_event(Peer, Tracker)
	//TorrentFile m_torrent_file;
	peer_map m_peers;
	bool m_new;
	bool m_multifile;
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
	void init_members(network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc, std::string & work_directory);
	int get_announces_from_metafile();
	void get_additional_info_from_metafile();
	int get_files_info_from_metafile(bencode::be_node * info);
	int get_main_info_from_metafile( uint64_t metafile_len);
	int calculate_info_hash(bencode::be_node * info, uint64_t metafile_len);
	void init_bitfield();
	void release();
	int read_state_file(state_file * sf);
	int save_state_file(state_file * sf);
	int get_state();
	int save_state();
	void delete_socket(network::socket_ * sock, network::sock_event * se);
	void add_socket(network::socket_ * sock, network::sock_event * se);
	network::sock_event * get_sock_event(network::socket_ * sock);
	int handle_download_task();
public:
	TorrentFile m_torrent_file;
	void take_peers(int count, sockaddr_in * addrs);
	void delete_peer(Peer * peer);
public:
	Torrent();
	int Init(std::string metafile, network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc,
			std::string & work_directory, bool is_new);
	std::string get_error()
	{
		return m_error;
	}
	int start(std::string & download_directory);
	int stop();
	int pause();
	int continue_();
	int check();
	bool is_downloaded();
	int event_sock_ready2read(network::socket_ * sock);
	int event_sock_closed(network::socket_ * sock);
	int event_sock_sended(network::socket_ * sock);
	int event_sock_connected(network::socket_ * sock);
	int event_sock_accepted(network::socket_ * sock);
	int event_sock_timeout(network::socket_ * sock);
	int event_sock_unresolved(network::socket_* sock);
	int event_file_write(fs::write_event * eo);
	int event_piece_hash(uint32_t piece_index, bool ok, bool error);
	int clock();
	//добавляет пира либо от пользователя, либо от входящего соединения
	//если от пользователя то требуемое состояние PEER_STATE_WAIT_HANDSHAKE
	//если вх. соединение
	int add_incoming_peer(network::socket_ * sock);
	int get_info(torrent_info * info);
	int save_meta2file(const char * filepath);
	virtual ~Torrent();
	friend class Tracker;
	friend class Peer;
	friend class TorrentFile;
};

void set_bitfield(uint32_t piece, uint32_t piece_count, unsigned char * bitfield);
void reset_bitfield(uint32_t piece, uint32_t piece_count, unsigned char * bitfield);
bool bit_in_bitfield(uint32_t piece, uint32_t piece_count, unsigned char * bitfield);
void get_peer_key(sockaddr_in * addr, std::string * key);
} /* namespace TorrentNamespace */
