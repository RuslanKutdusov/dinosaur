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
	:network::sock_event(), fs::file_event()
{
	// TODO Auto-generated constructor stub
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
	m_hc = NULL;
	m_bc = NULL;
	m_rx_speed = 0.0f;
	m_tx_speed = 0.0f;
	m_state = TORRENT_STATE_NONE;
	m_error = TORRENT_ERROR_NO_ERROR;
}

Torrent::~Torrent()
{
	// TODO Auto-generated destructor stub
	release();
	//std::cout<<"Torrent deleted\n";
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
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		delete (*iter).second;
	}
	for(peer_map_iter iter = m_peers.begin(); iter != m_peers.end(); ++iter)
	{
		delete (*iter).second;
	}
	if (m_bitfield != NULL)
		delete[] m_bitfield;
	//std::cout<<"Torrent released\n";
}

char *read_file(const char *file, uint64_t *len)
{
	struct stat st;
	char *ret = NULL;
	FILE *fp;

	if (stat(file, &st))
		return ret;
	*len = st.st_size;

	fp = fopen(file, "r");
	if (!fp)
		return ret;

	ret = (char*)malloc(*len);
	if (!ret)
		return ret;

	fread(ret, 1, *len, fp);

	fclose(fp);

	return ret;
}

int Torrent::Init(std::string metafile, network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, HashChecker::HashChecker * hc, block_cache::Block_cache * bc,
		std::string & work_directory)
{
	//if (!nm)
	//	throw Exception("");//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
	m_nm = nm;
    m_metafile = NULL;
	m_creation_date = 0;
	m_private = 0;
	m_length = 0;
	m_files = NULL;
	m_files_count = 0;
	m_piece_length = 0;
	m_piece_count = 0;
	m_pieces = NULL;
	m_g_cfg = g_cfg;
	m_downloaded = 0;
	m_uploaded = 0;
	m_fm = fm;
	m_hc = hc;
	m_bc = bc;
	m_work_directory = work_directory;
	m_download_directory = "";
	m_state = TORRENT_STATE_NONE;
	m_error = TORRENT_ERROR_NO_ERROR;
	uint64_t len = 0;
	char * buf = read_file(metafile.c_str(), &len);
	if (buf == NULL)
	{
		m_error = TORRENT_ERROR_IO_ERROR;
		return ERR_NULL_REF; //throw FileException(errno);
	}
	m_metafile = bencode::decode(buf, len, true);
	free(buf);
	if (!m_metafile)
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL; //throw Exception("Invalid metafile, parse error");
	}
	//bencode::dump(m_metafile);

	bencode::be_node * temp;

	if (bencode::get_node(m_metafile,"announce-list",&temp) == 0 && bencode::is_list(temp))
	{
		bencode::be_node * l;
		for(int i = 0; bencode::get_node(temp, i, &l) == 0; i++)
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
		if (bencode::get_str(m_metafile,"announce",&str) == ERR_NO_ERROR)
		{
			char *c_str = bencode::str2c_str(str);
			m_announces.push_back(c_str);
			delete[] c_str;
		}
	}

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

	bencode::be_node * info;// = bencode::get_info_dict(m_metafile);
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
	c_str = bencode::str2c_str(str);
	m_name        = c_str;
	delete[] c_str;

	if (bencode::get_node(info,"files",&temp) == -1 || !bencode::is_list(temp))
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
		m_files_count = get_list_size(temp);
		//std::cout<<m_files_count<<std::endl;
		if (m_files_count <= 0)
		{
			m_error = TORRENT_ERROR_INVALID_METAFILE;
			return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get list size");
		}
		//выделяем память
		m_files = new file_info[m_files_count];
		memset(m_files, 0, sizeof(file_info) * m_files_count);
		m_length = 0;
		for(int i = 0; i < m_files_count; i++)
		{
			bencode::be_node * file;// = temp.val.l[i];
			if ( bencode::get_node(temp, i, &file) == -1 || !bencode::is_dict(file))
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
			}
			bencode::be_node * path;
			uint64_t  len = 0;
			if (bencode::get_node(file, "path", &path) == -1 || !bencode::is_list(path))
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
			}
			if (bencode::get_int(file, "length", &len) == -1)
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
			}
			std::string temp_str("");
			for(int j = 0; j < bencode::get_list_size(path); j++)
			{
				bencode::be_str * str;
				if (bencode::get_str(path, j, &str) == -1)
				{
					m_error = TORRENT_ERROR_INVALID_METAFILE;
					return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
				}
				char * c_str = bencode::str2c_str(str);
				temp_str.append(c_str);
				temp_str.append("/");
				delete[] c_str;
			}
			m_files[i].name = new char[temp_str.length()];
			strncpy(m_files[i].name, temp_str.c_str(), temp_str.length() - 1); //в конце стоит слэш, вместо него \0
			m_files[i].name[temp_str.length() - 1] = '\0';
			m_files[i].length = len;
			m_files[i].download = true;
			m_length += m_files[i].length;
		//	std::cout<<m_files[i].name<<" "<<m_files[i].length<<std::endl;
		}
		m_multifile = true;
	}

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

	memset(m_info_hash_bin,0,20);
	bencode::get_hash_bin(info,m_info_hash_bin);
	memset(m_info_hash_hex,0,41);
	bencode::get_hash_hex(info,m_info_hash_hex);


	m_bitfield_len = ceil(m_piece_count / 8.0f);
	m_bitfield = new unsigned char[m_bitfield_len];
	memset(m_bitfield, 0, m_bitfield_len);
	//формируем путь к файлу вида $HOME/.dinosaur/$INFOHASH_HEX
	m_state_file_name = m_work_directory;
	m_state_file_name.append(m_info_hash_hex);
	if (get_state() != ERR_NO_ERROR)
	{
		m_error = TORRENT_ERROR_IO_ERROR;
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
	m_new = read_state_file(&sf) != ERR_NO_ERROR;
	if (m_new)
	{
		//printf("New state\n");
		memset(&sf, 0, sizeof(state_file));
		sf.version = STATE_FILE_VERSION;
		strncpy(sf.download_directory, m_download_directory.c_str(), m_download_directory.length());
		return save_state_file(&sf);
	}
	else
	{
		memcpy(m_bitfield, sf.bitfield, m_bitfield_len);
		m_download_directory = sf.download_directory;
		m_uploaded = sf.uploaded;
	}
	return ERR_NO_ERROR;
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
				Peer * peer = new Peer(&addrs[i],this, PEER_ADD_TRACKER);
				add_socket(peer->get_sock(), peer);
				m_peers[key] = peer;
			}
		}
		catch(NoneCriticalException & e)
		{

		}
	}
}

void Torrent::delete_peer(Peer * peer)
{
	if (peer != NULL)
	{
		//m_sockets.erase(peer->get_sock_id());
		delete_socket(peer->get_sock(), peer);
		m_peers.erase(peer->get_ip_str());
		while(!peer->no_requested_blocks())
		{
			uint64_t id = peer->get_requested_block();
			uint32_t block_index = get_block_from_id(id);//(id & (uint32_t)4294967295);
			uint32_t piece = get_piece_from_id(id);//(id - block_index)>>32;
			m_torrent_file.unmark_if_match(piece,block_index, peer);
		}
		delete peer;
	}
}

int Torrent::add_incoming_peer(network::socket_ * sock)
{
	if (sock == NULL)
		return ERR_NULL_REF;
	try
	{
		std::string key;
		get_peer_key(&sock->m_peer, &key);
		//printf("add incoming peer %s\n", key.c_str());
		if (m_peers.count(key) == 0)
		{
			//printf("peer not exists %s\n", key.c_str());
			Peer * peer = new Peer(sock, this, PEER_ADD_INCOMING);
			add_socket(peer->get_sock(), peer);
			m_peers[key] = peer;
		}
		else
		{
			//printf("peer exists, waking up %s\n", key.c_str());
			Peer * peer = m_peers[key];
			add_socket(sock, peer);
			peer->wake_up(sock, PEER_ADD_INCOMING);
		}
		return ERR_NO_ERROR;
	}
	catch(NoneCriticalException & e)
	{
		//e.print();
		return ERR_INTERNAL;
		//continue;
	}
}

int Torrent::start(std::string & download_directory)
{
	if (m_state == TORRENT_STATE_NONE)
	{
		if (m_download_directory == "")//если торрент новый, эта строка будет пустой, иначе прочтется из файла состояния
		{
			m_download_directory = download_directory;
			if (m_download_directory[m_download_directory.length() - 1] != '/')
				m_download_directory.append("/");
			save_state();
		}
		std::string dir = m_download_directory;
		if (m_multifile)
			dir.append(m_name);
		std::cout<<dir<<std::endl;
		if (m_torrent_file.Init(this, dir, m_new) != ERR_NO_ERROR)
			return ERR_FILE_ERROR;
		for (std::vector<std::string>::iterator iter = m_announces.begin(); iter != m_announces.end(); ++iter)
		{
			//std::cout<<*iter<<std::endl;
			Tracker * temp;
			try
			{
				temp = new Tracker(this, *iter);
			}
			catch(NoneCriticalException & e)
			{
				e.print();
				continue;
			}
			m_trackers[*iter] = temp;
			network::socket_ * sock = temp->get_sock();
			if (sock != NULL)
				m_sockets[sock] = temp;
		}
		m_state = TORRENT_STATE_STARTED;
	}
	if (m_state == TORRENT_STATE_STOPPED)
	{
		for(peer_map_iter p = m_peers.begin(); p != m_peers.end(); ++p)
		{
			Peer * peer = (*p).second;
			peer->wake_up(NULL, PEER_ADD_TRACKER);
		}
		for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
		{
			(*iter).second->send_started();
		}
		m_state = TORRENT_STATE_STARTED;
	}
	return ERR_NO_ERROR;
}

int Torrent::stop()
{
	if (m_state != TORRENT_STATE_STARTED)
		return ERR_INTERNAL;
	for(peer_map_iter p = m_peers.begin(); p != m_peers.end(); ++p)
	{
		Peer * peer = (*p).second;
		peer->goto_sleep();
	}
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		(*iter).second->send_stopped();
	}
	m_state = TORRENT_STATE_STOPPED;
	return 0;
}


bool Torrent::is_downloaded()
{
	return m_downloaded == m_length;
}

int Torrent::clock()
{
	//printf("CLOCK\n");
	m_rx_speed = 0.0f;
	m_tx_speed = 0.0f;
	for(peer_map_iter p = m_peers.begin(); p != m_peers.end(); ++p)
	{
		Peer * peer = (*p).second;
		m_rx_speed += peer->get_rx_speed();
		m_tx_speed += peer->get_tx_speed();
		for(std::list<uint32_t>::iterator iter = m_have_list.begin();
				iter != m_have_list.end();
				++iter)
		{
			//printf("Send have %u\n", *iter);
			peer->send_have(*iter);
		}
		peer->clock();
		if(peer->may_request() && !peer->request_limit() && m_state == TORRENT_STATE_STARTED)
		{
			for(std::set<uint32_t>::iterator i = m_pieces_to_download.begin();
						i != m_pieces_to_download.end(); ++i)
			{
				uint32_t piece = *i;
				if (!peer->have_piece(piece))
					continue;
				if (peer->request_limit())
					break;

				uint32_t blocks_count = m_torrent_file.get_blocks_count_in_piece(piece);
				if (blocks_count == 0)
					continue;

				for(std::map<uint32_t, Peer*>::iterator iter = m_torrent_file.m_piece_info[piece].blocks2download.begin();
					iter != m_torrent_file.m_piece_info[piece].blocks2download.end(); ++iter)
				{
					if ((*iter).second == NULL)//если блок не помечен
					{
						printf("Unmarked piece %u %d block2download.size=%d\n", piece, (*iter).first, m_torrent_file.m_piece_info[piece].blocks2download.size());
						if (peer->request_limit())
							break;
						uint32_t block_length;
						m_torrent_file.get_block_length_by_index(piece, (*iter).first, &block_length);
						if (peer->send_request(piece, (*iter).first, block_length) == ERR_NO_ERROR)
							m_torrent_file.mark_block(piece, (*iter).first, peer);
					}
				}
				if (m_torrent_file.blocks_in_progress(piece) == 0)
				{
					m_pieces_to_download.erase(i);
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
				m_torrent_file.unmark_if_match(piece,block_index, peer);
			}
		}
	}
	m_have_list.clear();
	if (m_state == TORRENT_STATE_STARTED)
		for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
		{
			(*iter).second->clock();
		}
	return 0;
}

network::sock_event * Torrent::get_sock_event(network::socket_ * sock)
{
	socket_map_iter iter = m_sockets.find(sock);
	if (iter == m_sockets.end() && m_sockets.count(sock) == 0)
		return NULL;
	return (*iter).second;
}

int Torrent::event_sock_ready2read(network::socket_ * sock)
{
	//std::cout<<"TORRENT event_sock_ready2read "<<sock_id<<" "<<m_name<<std::endl;
	network::sock_event * se = get_sock_event(sock);//m_sockets[sock_id];
	if (se == NULL)
		return ERR_BAD_ARG;
	se->event_sock_ready2read(sock);
	return 0;
}

int Torrent::event_sock_closed(network::socket_ * sock)
{
	//std::cout<<"TORRENT event_sock_closed "<<sock_id<<" "<<m_name<<std::endl;
	network::sock_event * se = get_sock_event(sock);//m_sockets[sock_id];
	if (se == NULL)
		return ERR_BAD_ARG;
	se->event_sock_closed(sock);
	return 0;
}

int Torrent::event_sock_sended(network::socket_ * sock)
{
	//std::cout<<"TORRENT event_sock_sended "<<sock_id<<" "<<m_name<<std::endl;
	network::sock_event * se = get_sock_event(sock);//m_sockets[sock_id];
	if (se == NULL)
		return ERR_BAD_ARG;
	se->event_sock_sended(sock);
	return 0;
}

int Torrent::event_sock_connected(network::socket_ * sock)
{
	//std::cout<<"TORRENT event_sock_connected "<<sock_id<<" "<<m_name<<std::endl;
	network::sock_event * se = get_sock_event(sock);//m_sockets[sock_id];
	if (se == NULL)
		return ERR_BAD_ARG;
	se->event_sock_connected(sock);
	return 0;
}

int Torrent::event_sock_accepted(network::socket_ * sock)
{
	//std::cout<<"TORRENT event_sock_accepted "<<sock_id<<" "<<m_name<<std::endl;
	network::sock_event * se = get_sock_event(sock);//m_sockets[sock_id];
	if (se == NULL)
		return ERR_BAD_ARG;
	se->event_sock_accepted(sock);
	return 0;
}

int Torrent::event_sock_timeout(network::socket_ * sock)
{
	network::sock_event * se = get_sock_event(sock);//m_sockets[sock_id];
	if (se == NULL)
		return ERR_BAD_ARG;
	se->event_sock_timeout(sock);
	return 0;
}

int Torrent::event_sock_unresolved(network::socket_* sock)
{
	network::sock_event * se = get_sock_event(sock);//m_sockets[sock_id];
	if (se == NULL)
		return ERR_BAD_ARG;
	se->event_sock_unresolved(sock);
	return 0;
}

int Torrent::event_file_write(fs::write_event * eo)
{
	return m_torrent_file.event_file_write(eo);
}


int Torrent::event_piece_hash(uint32_t piece_index, bool ok, bool error)
{
	if (ok && !error)
	{
		m_have_list.push_back(piece_index);
		m_downloaded +=m_torrent_file.m_piece_info[piece_index].length;
		set_bitfield(piece_index, m_piece_count, m_bitfield);
		save_state();
		printf("Piece %d OK\n", piece_index);
		//printf("rx=%f tx=%f done=%d downloaded=%llu\n", m_rx_speed, m_tx_speed, m_pieces_to_download.size(), m_downloaded);
	}
	if (!ok || error)
	{
		m_torrent_file.restore_piece2download(piece_index);
		m_pieces_to_download.insert(piece_index);
		reset_bitfield(piece_index, m_piece_count, m_bitfield);
		save_state();
		printf("Piece %d BAD\n", piece_index);
		//printf("rx=%f tx=%f done=%d downloaded=%llu\n", m_rx_speed, m_tx_speed, m_pieces_to_download.size(), m_downloaded);
	}
	return ERR_NO_ERROR;
}

void Torrent::get_info_hash(unsigned char * info_hash)
{
	if (info_hash == NULL)
		return;
	memcpy(info_hash, m_info_hash_bin, SHA1_LENGTH);
	return;
}

int Torrent::set_download_file(int index, bool download)
{
	if (index >= m_files_count || index < 0)
		return ERR_BAD_ARG;
	m_files[index].download = download;
	return ERR_NO_ERROR;
}

void Torrent::delete_socket(network::socket_ * sock, network::sock_event * se)
{
	socket_map_iter iter = m_sockets.find(sock);
	if (iter == m_sockets.end() && m_sockets.count(sock) == 0)
		return;
	if ((*iter).second == se)
		m_sockets.erase(iter);
}

void Torrent::add_socket(network::socket_ * sock, network::sock_event * se)
{
	if (sock != NULL)
		m_sockets[sock] = se;
}

bool Torrent::check()
{
	return m_torrent_file.check_all_pieces();
}

int Torrent::get_info(torrent_info * info)
{
	if (info == NULL)
		return ERR_NULL_REF;
	info->comment = m_comment;
	info->created_by = m_created_by;
	info->creation_date = m_creation_date;
	info->downloaded = m_downloaded;
	info->length = m_length;
	info->name = m_name;
	info->piece_count = m_piece_count;
	info->piece_length = m_piece_length;
	info->private_ = m_private;
	info->rx_speed = m_rx_speed;
	info->tx_speed = m_tx_speed;
	info->uploaded = m_uploaded;
	for(peer_map_iter iter = m_peers.begin(); iter != m_peers.end(); ++iter)
	{
		Peer * peer = (*iter).second;
		if (peer->get_sock() != NULL)
		{
			peer_info pi;
			peer->get_info(&pi);
			info->peers.push_back(pi);
		}
	}
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		Tracker *  tracker = (*iter).second;
		tracker_info ti;
		tracker->get_info(&ti);
		info->trackers.push_back(ti);
	}
	for(int i = 0; i < m_files_count; i++)
	{
		torrent_info::file f;
		f.first = m_files[i].name;
		f.second = m_files[i].length;
		info->file_list_.push_back(f);
	}
	return ERR_NO_ERROR;
}

} /* namespace TorrentNamespace */
