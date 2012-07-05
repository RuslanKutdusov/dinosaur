/*
 * Torrent.cpp
 *
 *  Created on: 29.01.2012
 *      Author: ruslan
 */

#include "torrent.h"

namespace torrent {

void set_bitfield(uint32_t piece, uint32_t piece_count, unsigned char * bitfield)
{
	if (piece >= piece_count)
		return;
	int byte_index = (int)floor(piece / 8);
	int bit_index = piece - byte_index * 8;
	int bit = (int)pow(2.0f, 7 - bit_index);
	bitfield[byte_index] |= bit;
}

void reset_bitfield(uint32_t piece, uint32_t piece_count, unsigned char * bitfield)
{
	if (piece >= piece_count)
		return;
	int byte_index = (int)floor(piece / 8);
	int bit_index = piece - byte_index * 8;
	int bit = (int)pow(2.0f, 7 - bit_index);
	unsigned char mask;
	memset(&mask, 255, 1);
	mask ^= bit;
	bitfield[byte_index] &= mask;
}

bool bit_in_bitfield(uint32_t piece, uint32_t piece_count, unsigned char * bitfield)
{
	if (piece >= piece_count)
			return false;
	int byte_index = (int)floor(piece / 8);
	int bit_index = piece - byte_index * 8;
	int bit = (int)pow(2.0f, 7 - bit_index);
	return (bitfield[byte_index] & bit) > 0;
}

void get_peer_key(sockaddr_in * addr, std::string * key)
{
	*key = inet_ntoa(addr->sin_addr);
	uint16_t port = ntohs(addr->sin_port);
	char port_c[256];
	sprintf(port_c, "%u", port);
	key->append(":");
	key->append(port_c);
}

Torrent::Torrent()
	//:network::sock_event(), fs::file_event()
{
#ifdef BITTORRENT_DEBUG
	printf("Torrent default constructor\n");
#endif
	std::string t = "";
	init_members(NULL, NULL, NULL, NULL, t);
	m_nm = NULL;
	m_metafile = NULL;
	m_creation_date = 0;
	m_private = 0;
	m_length = 0;
	m_files = NULL;
	m_files_count = 0;
	m_piece_length = 0;
	m_piece_count = 0;
	m_pieces = NULL;
	m_g_cfg = NULL;
	m_downloaded = 0;
	m_uploaded = 0;
	m_bitfield = NULL;
	m_fm = NULL;
	m_bc = NULL;
	m_rx_speed = 0.0f;
	m_tx_speed = 0.0f;
	m_state = TORRENT_STATE_NONE;
	m_error = TORRENT_ERROR_NO_ERROR;
	m_torrent_file.reset(new TorrentFile());
}

Torrent::~Torrent()
{
#ifdef BITTORRENT_DEBUG
	printf("Torrent destructor\n");
#endif
	release();
#ifdef BITTORRENT_DEBUG
	printf("Torrent destroyed\n");
#endif
}

void Torrent::Prepare2Release()
{
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		if ((*iter).second->prepare2release() != ERR_NO_ERROR)
			m_trackers.erase(iter);
	}
	for(peer_map_iter iter = m_peers.begin(); iter != m_peers.end(); ++iter)
	{
		(*iter).second->prepare2release();
	}
	//m_trackers.clear();
	m_peers.clear();
	m_torrent_file->ReleaseFiles();
	m_torrent_file.reset();
}

void Torrent::release()
{
	// TODO Auto-generated destructor stub
	if (m_metafile != NULL)
		bencode::_free(m_metafile);
	for (int i = 0; i < m_files_count; i++)
	{
		if (m_files[i].name != NULL)
			delete[] m_files[i].name;
	}
	if (m_files != NULL)
		delete[] m_files;
	if (m_bitfield != NULL)
		delete[] m_bitfield;

	//std::cout<<"Torrent released\n";
}

void Torrent::init_members(network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc, std::string & work_directory)
{
	m_nm = nm;
	m_metafile = NULL;
	m_g_cfg = g_cfg;
	m_fm = fm;
	m_bc = bc;
	m_work_directory = work_directory;
	m_creation_date = 0;
	m_private = 0;
	m_length = 0;
	m_files = NULL;
	m_files_count = 0;
	m_dir_tree = NULL;
	m_piece_length = 0;
	m_piece_count = 0;
	m_pieces = NULL;
	m_downloaded  = 0;
	m_uploaded = 0;
	m_bitfield = NULL;
	m_bitfield_len = 0;
	m_rx_speed = 0;
	m_tx_speed = 0;
	m_state = TORRENT_STATE_NONE;
}

int Torrent::get_announces_from_metafile()
{
	if (m_metafile == NULL)
		return ERR_NULL_REF;
	bencode::be_node * node;

	if (bencode::get_node(m_metafile,"announce-list",&node) == 0 && bencode::is_list(node))
	{
		bencode::be_node * l;
		for(int i = 0; bencode::get_node(node, i, &l) == 0; i++)
		{
			if (!bencode::is_list(l))
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, bad announce-list");
			}
			bencode::be_str * str;
			for(int j = 0; bencode::get_str(l, j, &str) == 0; j++)
			{
				char *c_str = bencode::str2c_str(str);
				m_announces.push_back(c_str);
				delete[] c_str;
			}
		}
	}
	else
	{
		bencode::be_str * str;
		//отсутствие трекеров не ошибка
		if (bencode::get_str(m_metafile,"announce",&str) == ERR_NO_ERROR)
		{
			char *c_str = bencode::str2c_str(str);
			m_announces.push_back(c_str);
			delete[] c_str;
		}
	}
	return ERR_NO_ERROR;
}

void Torrent::get_additional_info_from_metafile()
{
	if (m_metafile == NULL)
		return;
	bencode::be_str * str;
	char * c_str;
	if (bencode::get_str(m_metafile,"comment",&str) == 0)
	{
		c_str = bencode::str2c_str(str);
		m_comment        = c_str;
		delete[] c_str;
	}

	if (bencode::get_str(m_metafile,"created by",&str) == 0)
	{
		c_str = bencode::str2c_str(str);
		m_created_by        = c_str;
		delete[] c_str;
	}

	bencode::get_int(m_metafile,"creation date",&m_creation_date);
}

int Torrent::get_files_info_from_metafile(bencode::be_node * info)
{
	if (m_metafile == NULL || info == NULL)
		return ERR_NULL_REF;
	m_dir_tree = new dir_tree::DirTree(m_name);
	bencode::be_node * files_node;
	if (bencode::get_node(info,"files",&files_node) == -1 || !bencode::is_list(files_node))
	{
		if (bencode::get_int(info,"length",&m_length) == -1 || m_length <= 0)
		{
			m_error = TORRENT_ERROR_INVALID_METAFILE;
			return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get key length");
		}
		m_files = new file_info[1];
		m_files[0].length = m_length;
		ssize_t sl = m_name.length();
		//std::cout<<sl<<std::endl;
		m_files[0].name = new char[sl + 1];
		memset(m_files[0].name, 0, sl + 1);
		strncpy(m_files[0].name, m_name.c_str(), sl);
		//m_files->name[sl] = '\0';
		m_files[0].download = true;
		m_files_count = 1;
		m_multifile = false;
	}
	else
	{//определяем кол-во файлов
		m_files_count = get_list_size(files_node);
		//std::cout<<m_files_count<<std::endl;
		if (m_files_count <= 0)
		{
			m_error = TORRENT_ERROR_INVALID_METAFILE;
			return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get list size");
		}
		//выделяем память
		m_files = new file_info[m_files_count];
		//memset(m_files, 0, sizeof(file_info) * m_files_count);
		m_length = 0;
		for(int i = 0; i < m_files_count; i++)
		{
			bencode::be_node * file_node;// = temp.val.l[i];
			if ( bencode::get_node(files_node, i, &file_node) == -1 || !bencode::is_dict(file_node))
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
			}

			bencode::be_node * path_node;
			if (bencode::get_node(file_node, "path", &path_node) == -1 || !bencode::is_list(path_node))
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
			}
			if (bencode::get_int(file_node, "length", &m_files[i].length) == -1)
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
			}

			m_dir_tree->reset();
			int path_length = 0;
			int path_list_size = bencode::get_list_size(path_node);
			for(int j = 0; j < path_list_size; j++)
			{
				bencode::be_str * str;
				if (bencode::get_str(path_node, j, &str) == -1)
				{
					m_error = TORRENT_ERROR_INVALID_METAFILE;
					return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
				}
				path_length += str->len + 1;// 1 for slash and \0 at the end;
			}
			m_files[i].name = new char[path_length];
			int substring_offset = 0;
			for(int j = 0; j < path_list_size; j++)
			{
				bencode::be_str * str;
				if (bencode::get_str(path_node, j, &str) == -1)
				{
					m_error = TORRENT_ERROR_INVALID_METAFILE;
					return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
				}
				strncpy(&m_files[i].name[substring_offset], str->ptr, str->len);
				if (j < path_list_size - 1)
				{
					m_files[i].name[substring_offset + str->len] = '\0';
					if (m_dir_tree->put(&m_files[i].name[substring_offset]) != ERR_NO_ERROR)
					{
						m_error = GENERAL_ERROR_UNDEF_ERROR;
						return ERR_INTERNAL;
					}
				}

				substring_offset += str->len;
				m_files[i].name[substring_offset++] = '/';
			}
			m_files[i].name[path_length - 1] = '\0';
			m_files[i].download = true;
			m_length += m_files[i].length;
		}
		m_multifile = true;
	}
	return ERR_NO_ERROR;
}

int Torrent::get_main_info_from_metafile( uint64_t metafile_len)
{
	if (m_metafile == NULL)
		return ERR_NULL_REF;

	bencode::be_node * info;// = bencode::get_info_dict(m_metafile);
	bencode::be_str * str;
	if (bencode::get_node(m_metafile, "info", &info) == -1)
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get info dictionary");
	}

	if (bencode::get_str(info,"name",&str) == -1)
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get key name");
	}
	char * c_str = bencode::str2c_str(str);
	m_name        = c_str;
	delete[] c_str;

	get_files_info_from_metafile(info);

	if (bencode::get_int(info,"piece length",&m_piece_length) == -1 || m_piece_length == 0)
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL; //throw Exception("Invalid metafile");
	}

	if (bencode::get_str(info,"pieces",&str) == -1 || str->len % 20 != 0)
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL; //throw Exception("Invalid metafile");
	}
	m_pieces	 = str->ptr;
	m_piece_count = str->len / 20;

	if (bencode::get_int(m_metafile,"private",&m_private) == -1)
		bencode::get_int(info,"private",&m_private);

	return calculate_info_hash(info, metafile_len);
}

int Torrent::calculate_info_hash(bencode::be_node * info, uint64_t metafile_len)
{
	if (info == NULL || metafile_len == 0)
		return ERR_BAD_ARG;
	memset(m_info_hash_bin,0,20);
	memset(m_info_hash_hex,0,41);
	char * bencoded_info = new char[metafile_len];
	uint32_t bencoded_info_len = 0;
	if (bencode::encode(info, &bencoded_info, metafile_len, &bencoded_info_len) != ERR_NO_ERROR)
	{
		delete[] bencoded_info;
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL;
	}
	CSHA1 csha1;
	csha1.Update((unsigned char*)bencoded_info,bencoded_info_len);
	csha1.Final();
	csha1.ReportHash(m_info_hash_hex,CSHA1::REPORT_HEX);
	csha1.GetHash(m_info_hash_bin);
	delete[] bencoded_info;
	return ERR_NO_ERROR;
}

void Torrent::init_bitfield()
{
	m_bitfield_len = ceil(m_piece_count / 8.0f);
	m_bitfield = new unsigned char[m_bitfield_len];
	memset(m_bitfield, 0, m_bitfield_len);
}

int Torrent::Init(std::string metafile, network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc,
		std::string & work_directory, bool is_new)
{
	if (nm == NULL || g_cfg == NULL || fm == NULL || bc == NULL || work_directory.length() == 0)
	{
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_BAD_ARG;
	}
	init_members(nm, g_cfg, fm, bc, work_directory);
	m_new = is_new;
	try{
		char * buf = read_file(metafile.c_str(), &m_metafile_len, MAX_TORRENT_FILE_LENGTH);
		if (buf == NULL)
		{
			m_error = TORRENT_ERROR_IO_ERROR;
			return ERR_NULL_REF; //throw FileException(errno);
		}
		m_metafile = bencode::decode(buf, m_metafile_len, true);
		free(buf);
		if (!m_metafile)
		{
			m_error = TORRENT_ERROR_INVALID_METAFILE;
			return ERR_INTERNAL; //throw Exception("Invalid metafile, parse error");
		}

		if (get_announces_from_metafile() != ERR_NO_ERROR)
			return ERR_INTERNAL;

		get_additional_info_from_metafile();

		if (get_main_info_from_metafile(m_metafile_len) != ERR_NO_ERROR)
			return ERR_INTERNAL;

		init_bitfield();

		//формируем путь к файлу вида $HOME/.dinosaur/$INFOHASH_HEX
		m_state_file_name = m_work_directory;
		m_state_file_name.append(m_info_hash_hex);
		if (!is_new)
			if (get_state() != ERR_NO_ERROR)
			{
				m_error = TORRENT_ERROR_NO_STATE_FILE;
				return ERR_INTERNAL;
			}
		for(uint32_t i = 0; i < m_piece_count; i++)
		{
			if (!bit_in_bitfield(i, m_piece_count, m_bitfield))
				m_pieces_to_download.insert(i);
			else
			{
				uint32_t piece_length = (i == m_piece_count - 1) ? m_length - m_piece_length * i : m_piece_length;
				m_downloaded += piece_length;
			}
		}
	}
	catch( std::bad_alloc & e)
	{
		m_error = GENERAL_ERROR_NO_MEMORY_AVAILABLE;
		return ERR_INTERNAL;
	}
	catch( Exception & e)
	{
		return ERR_INTERNAL;
	}
	return ERR_NO_ERROR;
}

int Torrent::read_state_file(state_file * sf)
{
	struct stat st;
	if (sf == NULL)
		return ERR_NULL_REF;
	if (stat(m_state_file_name.c_str(), &st) == -1 || (uint64_t)st.st_size != sizeof(state_file))
		return ERR_INTERNAL;
	int fd = open(m_state_file_name.c_str(), O_RDWR | O_CREAT, S_IRWXU);
	if (fd == -1)
		return ERR_SYSCALL_ERROR;
	ssize_t ret = read(fd, (void*)sf, sizeof(state_file));
	if (ret == -1)
	{
		close(fd);
		return ERR_SYSCALL_ERROR;
	}
	close(fd);
	return ERR_NO_ERROR;
}

int Torrent::save_state_file(state_file * sf)
{
	if (sf == NULL)
		return ERR_NULL_REF;
	int fd = open(m_state_file_name.c_str(), O_RDWR | O_CREAT, S_IRWXU);
	if (fd == -1)
		return ERR_SYSCALL_ERROR;
	ssize_t ret = write(fd, sf, sizeof(state_file));
	if (ret == -1)
	{
		close(fd);
		return ERR_SYSCALL_ERROR;
	}
	close(fd);
	return ERR_NO_ERROR;
}

int Torrent::get_state()
{
	state_file sf;
	if (read_state_file(&sf) == ERR_NO_ERROR)
	{
		memcpy(m_bitfield, sf.bitfield, m_bitfield_len);
		m_download_directory = sf.download_directory;
		m_uploaded = sf.uploaded;
		return ERR_NO_ERROR;
	}
	else
		return ERR_FILE_ERROR;
}


int Torrent::save_state()
{
	state_file sf;
	memset(&sf, 0, sizeof(state_file));
	sf.version = STATE_FILE_VERSION;
	if (m_download_directory.length() > MAX_FILENAME_LENGTH)
		return ERR_INTERNAL;
	strncpy(sf.download_directory, m_download_directory.c_str(), m_download_directory.length());
	memcpy(sf.bitfield, m_bitfield, m_bitfield_len);
	//sf.downloaded = m_downloaded;
	sf.uploaded = m_uploaded;
	return save_state_file(&sf);
}

void Torrent::take_peers(int count, sockaddr_in * addrs)
{
	for(int i = 0; i < count; i++)
	{
		try
		{
			std::string key;
			get_peer_key(&addrs[i], &key);
			//если такого пира у нас нет, добавляем его
			if (m_peers.count(key) == 0)
			{
				PeerPtr peer(new Peer());
				peer->Init(&addrs[i], shared_from_this(), PEER_ADD_TRACKER);
				m_peers[key] = peer;
			}
		}
		catch(NoneCriticalException & e)
		{

		}
	}
}

void Torrent::delete_peer(PeerPtr & peer)
{
	if (peer != NULL)
	{
		//m_sockets.erase(peer->get_sock_id());
		//peer->DeleteSocket();
		m_peers.erase(peer->get_ip_str());
		peer.reset();
	}
}

int Torrent::add_incoming_peer(network::Socket & sock)
{
	if (sock == NULL)
		return ERR_NULL_REF;
	try
	{
		std::string key;
		get_peer_key(&sock->m_peer, &key);
		//printf("add incoming peer %s\n", key.c_str());
		if (m_peers.count(key) == 0 || m_peers[key] == NULL)
		{
			PeerPtr peer(new Peer());
			peer->Init(sock, shared_from_this(), PEER_ADD_INCOMING);
			m_peers[key] = peer;
		}
		else
		{
			m_peers[key]->wake_up(sock, PEER_ADD_INCOMING);
		}
		return ERR_NO_ERROR;
	}
	catch(NoneCriticalException & e)
	{
		//e.print();
		return ERR_INTERNAL;
		//continue;
	}
	return ERR_NO_ERROR;
}

int Torrent::start(std::string & download_directory)
{
	if (m_state == TORRENT_STATE_NONE)
	{
		if (m_new)
		{
			m_download_directory = download_directory;
			if (m_download_directory[m_download_directory.length() - 1] != '/')
				m_download_directory.append("/");
			save_state();
		}
		if (m_torrent_file->Init(shared_from_this(), m_download_directory, m_new) != ERR_NO_ERROR)
			return ERR_FILE_ERROR;
		for (std::vector<std::string>::iterator iter = m_announces.begin(); iter != m_announces.end(); ++iter)
		{
			//std::cout<<*iter<<std::endl;
			TrackerPtr temp;
			try
			{
				temp.reset(new Tracker(shared_from_this(), *iter));
				m_trackers[*iter] = temp;
			}
			catch(NoneCriticalException & e)
			{
				e.print();
				continue;
			}
		}
		for(std::set<uint32_t>::iterator i = m_pieces_to_download.begin();
					i != m_pieces_to_download.end(); ++i)
		{
			TORRENT_TASK task;
			task.task = TORRENT_TASK_DOWNLOAD_PIECE;
			task.task_data = *i;
			m_task_queue.push_back(task);
		}
		m_state = TORRENT_STATE_STARTED;
	}
	else
		return ERR_INTERNAL;
	return ERR_NO_ERROR;
}

int Torrent::stop()
{
	if (m_state == TORRENT_STATE_NONE && m_state != TORRENT_STATE_CHECKING)
		return ERR_INTERNAL;
	for(peer_map_iter p = m_peers.begin(); p != m_peers.end(); ++p)
	{
		(*p).second->goto_sleep();
	}
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		(*iter).second->send_stopped();
	}
	m_task_queue.clear();
	m_state = TORRENT_STATE_STOPPED;
	return 0;
}

int Torrent::pause()
{
	if (m_state != TORRENT_STATE_STARTED && m_state != TORRENT_STATE_CHECKING)
		return ERR_INTERNAL;
	for(peer_map_iter p = m_peers.begin(); p != m_peers.end(); ++p)
	{
		(*p).second->goto_sleep();
	}
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		(*iter).second->send_stopped();
	}
	m_task_queue.clear();
	m_state = TORRENT_STATE_PAUSED;
	return ERR_NO_ERROR;
}

int Torrent::continue_()
{
	if (m_state != TORRENT_STATE_PAUSED && m_state != TORRENT_STATE_CHECKING)
		return ERR_INTERNAL;
	for(peer_map_iter p = m_peers.begin(); p != m_peers.end(); ++p)
	{
		network::Socket sock;
		(*p).second->wake_up(sock, PEER_ADD_TRACKER);
	}
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		(*iter).second->send_started();
	}
	m_task_queue.clear();
	for(std::set<uint32_t>::iterator i = m_pieces_to_download.begin();
						i != m_pieces_to_download.end(); ++i)
	{
		TORRENT_TASK task;
		task.task = TORRENT_TASK_DOWNLOAD_PIECE;
		task.task_data = *i;
		m_task_queue.push_back(task);
	}
	m_state = TORRENT_STATE_STARTED;
	return ERR_NO_ERROR;
}


bool Torrent::is_downloaded()
{
	return m_downloaded == m_length;
}

int Torrent::check()
{
	if (m_state == TORRENT_STATE_NONE)
		return ERR_INTERNAL;
	for(peer_map_iter p = m_peers.begin(); p != m_peers.end(); ++p)
	{
		(*p).second->goto_sleep();
	}
	m_task_queue.clear();
	m_downloaded = 0;
	m_pieces_to_download.clear();
	memset(m_bitfield, 0, m_bitfield_len);
	for(uint32_t i = 0; i < m_piece_count; i++)
	{
		TORRENT_TASK task;
		task.task = TORRENT_TASK_CHECK_HASH;
		task.task_data = i;
		m_task_queue.push_back(task);
		m_pieces_to_download.insert(i);
	}
	m_state = TORRENT_STATE_CHECKING;
	return ERR_NO_ERROR;
}


int Torrent::handle_download_task()
{
	/*for(peer_map_iter p = m_peers.begin(); p != m_peers.end(); ++p)
	{
		Peer * peer = (*p).second;
		if(peer->may_request() && !peer->request_limit() && m_state == TORRENT_STATE_STARTED)
		{
			for(std::list<TORRENT_TASK>::iterator task_iter = m_task_queue.begin(), task_iter_;
													task_iter != m_task_queue.end();
													task_iter = task_iter_)
			{
				uint32_t piece = (*task_iter).task_data;
				if (!peer->have_piece(piece))
				{
					task_iter_ = task_iter;
					++task_iter_;
				}
				if (peer->request_limit())
					break;

				uint32_t blocks_count = m_torrent_file->get_blocks_count_in_piece(piece);
				if (blocks_count == 0)
				{
					task_iter_ = task_iter;
					++task_iter_;
				}

				for(std::map<uint32_t, Peer*>::iterator iter = m_torrent_file->m_piece_info[piece].blocks2download.begin();
					iter != m_torrent_file->m_piece_info[piece].blocks2download.end(); ++iter)
				{
					if ((*iter).second == NULL)//если блок не помечен
					{
						//printf("Unmarked piece %u %d block2download.size=%d\n", piece, (*iter).first, m_torrent_file->m_piece_info[piece].blocks2download.size());
						if (peer->request_limit())
							break;
						uint32_t block_length;
						m_torrent_file->get_block_length_by_index(piece, (*iter).first, &block_length);
						if (peer->send_request(piece, (*iter).first, block_length) == ERR_NO_ERROR)
							m_torrent_file->mark_block(piece, (*iter).first, peer);
					}
				}

				task_iter_ = task_iter;
				++task_iter_;

				if (m_torrent_file->blocks_in_progress(piece) == 0)
				{
					m_pieces_to_download.erase(piece);
					m_task_queue.erase(task_iter);
				}
			}
		}
		//если заснул, снимаем метки с запрошенных блоков
		if (peer->is_sleep())
		{
			while(!peer->no_requested_blocks())
			{
				uint64_t id = peer->get_requested_block();
				uint32_t block_index = get_block_from_id(id);//(id & (uint32_t)4294967295);
				uint32_t piece = get_piece_from_id(id);//(id - block_index)>>32;
				m_torrent_file->unmark_if_match(piece,block_index, peer);
			}
		}
	}*/
	return ERR_NO_ERROR;
}

int Torrent::clock()
{
	//printf("CLOCK\n");
	m_rx_speed = 0.0f;
	m_tx_speed = 0.0f;
	if (m_state != TORRENT_STATE_STARTED && m_state != TORRENT_STATE_CHECKING)
		return ERR_NO_ERROR;
	for(peer_map_iter p = m_peers.begin(); p != m_peers.end(); ++p)
	{
		PeerPtr peer = (*p).second;
		m_rx_speed += peer->get_rx_speed();
		m_tx_speed += peer->get_tx_speed();
		for(std::list<uint32_t>::iterator iter = m_have_list.begin();
				iter != m_have_list.end();
				++iter)
		{
			peer->send_have(*iter);
		}
		if (m_state == TORRENT_STATE_STARTED)
			peer->clock();
	}
	m_have_list.clear();

	if (m_task_queue.front().task == TORRENT_TASK_DOWNLOAD_PIECE)
		handle_download_task();

	if (m_task_queue.front().task == TORRENT_TASK_CHECK_HASH)
	{
		//printf("Checking %llu", m_task_queue.front().task_data);
		m_torrent_file->check_piece_hash(m_task_queue.front().task_data);
		m_task_queue.pop_front();
		if (m_task_queue.empty())
			continue_();
	}

	if (m_state == TORRENT_STATE_STARTED)
		for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
		{
			bool release_tracker_instance;
			(*iter).second->clock(release_tracker_instance);
			if (release_tracker_instance)
				m_trackers.erase(iter);
		}
	return 0;
}


int Torrent::event_piece_hash(uint32_t piece_index, bool ok, bool error)
{
	if (ok && !error)
	{
		m_have_list.push_back(piece_index);
		m_downloaded +=m_torrent_file->m_piece_info[piece_index].length;
		set_bitfield(piece_index, m_piece_count, m_bitfield);
		save_state();
		m_pieces_to_download.erase(piece_index);
		printf("Piece %d OK\n", piece_index);
		//printf("rx=%f tx=%f done=%d downloaded=%llu\n", m_rx_speed, m_tx_speed, m_pieces_to_download.size(), m_downloaded);
	}
	if (!ok || error)
	{
		m_pieces_to_download.insert(piece_index);
		if (m_state == TORRENT_STATE_STARTED)
		{
			TORRENT_TASK task;
			task.task = TORRENT_TASK_DOWNLOAD_PIECE;
			task.task_data = piece_index;
			m_task_queue.push_back(task);
		}
		reset_bitfield(piece_index, m_piece_count, m_bitfield);
		save_state();
		printf("Piece %d BAD\n", piece_index);
		//printf("rx=%f tx=%f done=%d downloaded=%llu\n", m_rx_speed, m_tx_speed, m_pieces_to_download.size(), m_downloaded);
	}
	return ERR_NO_ERROR;
}

int Torrent::get_info(torrent_info * info)
{
	if (info == NULL)
		return ERR_NULL_REF;
	info->comment = m_comment;
	info->created_by = m_created_by;
	info->creation_date = m_creation_date;
	info->download_directory = m_download_directory;
	info->downloaded = m_downloaded;
	info->length = m_length;
	info->name = m_name;
	info->piece_count = m_piece_count;
	info->piece_length = m_piece_length;
	info->private_ = m_private;
	info->rx_speed = m_rx_speed;
	info->tx_speed = m_tx_speed;
	info->uploaded = m_uploaded;
	memcpy(info->info_hash_hex, m_info_hash_hex, SHA1_LENGTH * 2 + 1);
	for(peer_map_iter iter = m_peers.begin(); iter != m_peers.end(); ++iter)
	{
		PeerPtr peer = (*iter).second;
		if (!peer->is_sleep())
		{
			peer_info pi;
			peer->get_info(&pi);
			info->peers.push_back(pi);
		}
	}
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		tracker_info ti;
		(*iter).second->get_info(&ti);
		info->trackers.push_back(ti);
	}
	for(int i = 0; i < m_files_count; i++)
	{
		torrent_info::file f;
		f.first = m_files[i].name;
		f.second = m_files[i].length;
		info->file_list_.push_back(f);
	}
	if (m_state == TORRENT_STATE_CHECKING)
		info->progress = ((m_piece_count - m_task_queue.size()) * 100) / m_piece_count;
	else
		info->progress = (m_downloaded * 100) / m_length;
	return ERR_NO_ERROR;
}

int Torrent::save_meta2file(const char * filepath)
{
	if (filepath == NULL)
	{
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_BAD_ARG;
	}

	if (m_metafile == NULL)
	{
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_NULL_REF;
	}

	char * bencoded_metafile = new(std::nothrow) char[m_metafile_len];
	if (bencoded_metafile == NULL)
	{
		m_error = GENERAL_ERROR_NO_MEMORY_AVAILABLE;
		return ERR_INTERNAL;
	}
	uint32_t bencoded_metafile_len = 0;
	if (bencode::encode(m_metafile, &bencoded_metafile, m_metafile_len, &bencoded_metafile_len) != ERR_NO_ERROR)
	{
		delete[] bencoded_metafile;
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL;
	}
	int fd = open(filepath, O_RDWR | O_CREAT, S_IRWXU);
	if (fd == -1)
	{
		delete[] bencoded_metafile;
		m_error = TORRENT_ERROR_IO_ERROR;
		return ERR_SYSCALL_ERROR;
	}
	ssize_t ret = write(fd, bencoded_metafile, bencoded_metafile_len);
	if (ret == -1)
	{
		delete[] bencoded_metafile;
		m_error = TORRENT_ERROR_IO_ERROR;
		return ERR_SYSCALL_ERROR;
		close(fd);
		return ERR_SYSCALL_ERROR;
	}
	close(fd);
	delete[] bencoded_metafile;
	return ERR_NO_ERROR;
}

int Torrent::erase_state()
{
	remove(m_state_file_name.c_str());
	std::string infohash_str = m_info_hash_hex;
	std::string fname =  m_work_directory + infohash_str + ".torrent";
	remove(fname.c_str());
	return ERR_NO_ERROR;
}

} /* namespace TorrentNamespace */
