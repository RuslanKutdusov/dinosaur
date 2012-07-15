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

TorrentBase::TorrentBase()
	//:network::sock_event(), fs::file_event()
{
#ifdef BITTORRENT_DEBUG
	printf("Torrent default constructor\n");
#endif
	std::string t = "";
	init_members(NULL, NULL, NULL, NULL, t);
	m_nm = NULL;
	m_g_cfg = NULL;
	m_downloaded = 0;
	m_uploaded = 0;
	m_fm = NULL;
	m_bc = NULL;
	m_rx_speed = 0.0f;
	m_tx_speed = 0.0f;
	m_state = TORRENT_STATE_NONE;
	m_error = TORRENT_ERROR_NO_ERROR;
	m_torrent_file.reset(new TorrentFile());
	m_piece_manager.reset(new PieceManager());
}

TorrentBase::~TorrentBase()
{
#ifdef BITTORRENT_DEBUG
	printf("Torrent destructor\n");
#endif
	release();
#ifdef BITTORRENT_DEBUG
	printf("Torrent destroyed\n");
#endif
}

std::string TorrentBase::get_error()
{
	return m_error;
}

void TorrentBase::prepare2release()
{
	m_state = TORRENT_STATE_RELEASING;
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		if ((*iter).second->prepare2release() != ERR_NO_ERROR)
			m_trackers.erase(iter);
	}
	for(peer_map_iter iter = m_seeders.begin(); iter != m_seeders.end(); ++iter)
	{
		(*iter).second->prepare2release();
	}
	for(peer_map_iter iter = m_leechers.begin(); iter != m_leechers.end(); ++iter)
	{
		(*iter).second->prepare2release();
	}
	m_seeders.clear();
	m_active_seeders.clear();
	m_waiting_seeders.clear();
	m_leechers.clear();
	m_torrent_file->ReleaseFiles();
	m_torrent_file.reset();
	m_piece_manager.reset();
}

void TorrentBase::release()
{
	// TODO Auto-generated destructor stub

	//std::cout<<"Torrent released\n";
}

void TorrentBase::init_members(network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc, std::string & work_directory)
{
	m_nm = nm;
	m_g_cfg = g_cfg;
	m_fm = fm;
	m_bc = bc;
	m_work_directory = work_directory;
	m_downloaded  = 0;
	m_uploaded = 0;
	m_rx_speed = 0;
	m_tx_speed = 0;
	m_state = TORRENT_STATE_NONE;
}

int TorrentBase::Init(const Metafile & metafile, network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc,
		std::string & work_directory)
{
	if (nm == NULL || g_cfg == NULL || fm == NULL || bc == NULL || work_directory.length() == 0)
	{
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_BAD_ARG;
	}
	init_members(nm, g_cfg, fm, bc, work_directory);
	try{

		m_metafile = metafile;
		//формируем путь к файлу вида $HOME/.dinosaur/$INFOHASH_HEX
		m_state_file_name = m_work_directory;
		m_state_file_name.append(m_metafile.info_hash_hex);


		m_waiting_seeders.clear();
		m_active_seeders.clear();
		m_seeders.clear();
		m_leechers.clear();
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

void TorrentBase::add_seeders(uint32_t count, sockaddr_in * addrs)
{
	count = count > m_g_cfg->get_tracker_numwant() ? m_g_cfg->get_tracker_numwant() : count;
	for(uint32_t i = 0;	i < count;	i++)
	{
		try
		{
			std::string key;
			get_peer_key(&addrs[i], &key);
			//если такого пира у нас нет, добавляем его
			if (m_seeders.count(key) == 0)
			{
				PeerPtr peer(new Peer());
				peer->Init(&addrs[i], boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()));
				m_seeders[key] = peer;
				m_waiting_seeders.push_back(peer);
			}

		}
		catch(...)
		{

		}
	}
}

int TorrentBase::add_leecher(network::Socket & sock)
{
	if (sock == NULL)
		return ERR_NULL_REF;
	try
	{
		std::string key;
		get_peer_key(&sock->m_peer, &key);
		//printf("add incoming peer %s\n", key.c_str());
		if ((m_leechers.count(key) == 0 || m_leechers[key] == NULL) && m_leechers.size() < m_g_cfg->get_max_active_leechers())
		{
			PeerPtr peer(new Peer());
			peer->Init(sock, boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), PEER_ADD_INCOMING);
			m_leechers[key] = peer;
		}
		else
		{
			network::Socket s = sock;
			m_nm->Socket_delete(s);
		}
		return ERR_NO_ERROR;
	}
	catch(...)
	{
		network::Socket s = sock;
		m_nm->Socket_delete(s);
		return ERR_INTERNAL;
	}
	return ERR_NO_ERROR;
}

int TorrentBase::start(std::string & download_directory)
{
	if (m_state == TORRENT_STATE_NONE)
	{
		m_active_seeders.clear();

		StateSerializator s(m_state_file_name);
		size_t bitfield_len = ceil(m_metafile.piece_count / 8.0f);
		unsigned char * bitfield = new unsigned char[bitfield_len];
		if (s.deserialize(m_uploaded, m_download_directory, bitfield, bitfield_len) == -1)
		{
			m_download_directory = download_directory;
			if (m_download_directory[m_download_directory.length() - 1] != '/')
				m_download_directory.append("/");
			m_uploaded = 0;
			memset(bitfield, 0, bitfield_len);
			s.serialize(m_uploaded, m_download_directory, bitfield, bitfield_len);
			delete[] bitfield;
			bitfield = NULL;
		}
		if (m_torrent_file->Init(boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), m_download_directory) != ERR_NO_ERROR)
			return ERR_FILE_ERROR;

		if (m_piece_manager->Init(boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), bitfield) != ERR_NO_ERROR)
			return ERR_INTERNAL;

		if (bitfield != NULL)
			delete[] bitfield;

		for (std::vector<std::string>::iterator iter = m_metafile.announces.begin(); iter != m_metafile.announces.end(); ++iter)
		{
			TrackerPtr temp;
			try
			{
				temp.reset(new Tracker(boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), *iter));
				m_trackers[*iter] = temp;
			}
			catch(NoneCriticalException & e)
			{
#ifdef BITTORRENT_DEBUG
				e.print();
#endif
				continue;
			}
		}
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		inet_aton("127.0.0.1", &addr.sin_addr);
		addr.sin_port = htons(62651);
		add_seeders(1, &addr);
		/*for(std::set<uint32_t>::iterator i = m_piece_manager.m_pieces_to_download.begin();
					i != m_pieces_to_download.end(); ++i)
		{
			TORRENT_TASK task;
			task.task = TORRENT_TASK_DOWNLOAD_PIECE;
			task.task_data = *i;
			m_task_queue.push_back(task);
		}*/
		m_state = TORRENT_STATE_STARTED;
	}
	else
		return ERR_INTERNAL;
	return ERR_NO_ERROR;
}

int TorrentBase::stop()
{
	if (m_state == TORRENT_STATE_NONE && m_state != TORRENT_STATE_CHECKING)
		return ERR_INTERNAL;
	for(peer_list_iter p = m_active_seeders.begin(); p != m_active_seeders.end(); ++p)
	{
		m_waiting_seeders.push_back(*p);
	}
	m_active_seeders.clear();
	for(peer_map_iter p = m_leechers.begin(); p != m_leechers.end(); ++p)
	{
		(*p).second->prepare2release();
	}
	m_leechers.clear();
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		(*iter).second->send_stopped();
	}
	m_task_queue.clear();
	m_state = TORRENT_STATE_STOPPED;
	return 0;
}

int TorrentBase::pause()
{
	if (m_state != TORRENT_STATE_STARTED && m_state != TORRENT_STATE_CHECKING)
		return ERR_INTERNAL;
	for(peer_list_iter p = m_active_seeders.begin(); p != m_active_seeders.end(); ++p)
	{
		m_waiting_seeders.push_back(*p);
	}
	m_active_seeders.clear();
	for(peer_map_iter p = m_leechers.begin(); p != m_leechers.end(); ++p)
	{
		(*p).second->prepare2release();
	}
	m_leechers.clear();
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		(*iter).second->send_stopped();
	}
	m_task_queue.clear();
	m_state = TORRENT_STATE_PAUSED;
	return ERR_NO_ERROR;
}

int TorrentBase::continue_()
{
	if (m_state != TORRENT_STATE_PAUSED && m_state != TORRENT_STATE_CHECKING)
		return ERR_INTERNAL;
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		(*iter).second->send_started();
	}
	m_task_queue.clear();
	/*for(std::set<uint32_t>::iterator i = m_pieces_to_download.begin();
						i != m_pieces_to_download.end(); ++i)
	{
		TORRENT_TASK task;
		task.task = TORRENT_TASK_DOWNLOAD_PIECE;
		task.task_data = *i;
		m_task_queue.push_back(task);
	}*/
	m_state = TORRENT_STATE_STARTED;
	return ERR_NO_ERROR;
}


bool TorrentBase::is_downloaded()
{
	return m_downloaded == m_metafile.length;
}

int TorrentBase::check()
{
	if (m_state == TORRENT_STATE_NONE)
		return ERR_INTERNAL;
	for(peer_list_iter p = m_active_seeders.begin(); p != m_active_seeders.end(); ++p)
	{
		m_waiting_seeders.push_back(*p);
	}
	m_active_seeders.clear();
	for(peer_map_iter p = m_leechers.begin(); p != m_leechers.end(); ++p)
	{
		(*p).second->prepare2release();
	}
	m_leechers.clear();
	m_task_queue.clear();
	m_downloaded = 0;
	m_piece_manager->reset();
	/*for(uint32_t i = 0; i < m_piece_count; i++)
	{
		TORRENT_TASK task;
		task.task = TORRENT_TASK_CHECK_HASH;
		task.task_data = i;
		m_task_queue.push_back(task);
	}*/
	m_state = TORRENT_STATE_CHECKING;
	return ERR_NO_ERROR;
}


int TorrentBase::handle_download_task()
{
	return ERR_NO_ERROR;
}

int TorrentBase::clock()
{
	//printf("CLOCK\n");
	m_rx_speed = 0.0f;
	m_tx_speed = 0.0f;

	if (m_state == TORRENT_STATE_RELEASING)
		for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
		{
			bool release_tracker_instance;
			(*iter).second->clock(release_tracker_instance);
			if (release_tracker_instance)
				m_trackers.erase(iter);
		}

	if (m_state == TORRENT_STATE_STARTED)
	{
		if (m_active_seeders.size() < m_g_cfg->get_max_active_seeders() &&
				m_waiting_seeders.size() > 0)
		{
			PeerPtr seed = m_waiting_seeders.front();
			m_waiting_seeders.pop_front();
			if (seed->is_sleep())
				m_waiting_seeders.push_back(seed);
			else
				m_active_seeders.push_back(seed);
		}
		std::map<uint32_t, uint32_t> requested_blocks_num;
		for(peer_list_iter seed_iter = m_active_seeders.begin(),
							seed_iter_add = m_active_seeders.begin(); seed_iter != m_active_seeders.end(); seed_iter = seed_iter_add)
		{
			++seed_iter_add;
			PeerPtr seed = *seed_iter;
			seed->clock();
			if (seed->is_sleep())
			{
				while(!seed->no_requested_blocks())
				{
					uint64_t block_id = seed->get_requested_block();
					uint32_t piece, block;
					get_piece_block_from_id(block_id, piece, block);
					//m_piece_states[piece].block2download.push_back(block);
					//m_piece_states[piece].taken_from.erase(seed);
				}
				m_active_seeders.erase(seed_iter);
				m_waiting_seeders.push_back(seed);
			}
			else
			{
				m_rx_speed += seed->get_rx_speed();
				m_tx_speed += seed->get_tx_speed();
				for(std::list<uint32_t>::iterator have_iter = m_have_list.begin();
						have_iter != m_have_list.end();
						++have_iter)
				{
					seed->send_have(*have_iter);
				}
				if (m_downloadable_pieces.size() < m_active_seeders.size() &&
						m_task_queue.size() > 0 &&
						m_task_queue.front().task == TORRENT_TASK_DOWNLOAD_PIECE)
				{
					uint32_t piece = m_task_queue.front().task_data;
					m_task_queue.pop_front();
					m_downloadable_pieces.push_back(piece);
				}
				if (m_downloadable_pieces.size() > 0)
				{
					uint32_t piece  = m_downloadable_pieces.front();
					m_downloadable_pieces.pop_front();
					m_downloadable_pieces.push_back(piece);
					if (seed->have_piece(piece) && seed->may_request())
					{
						requested_blocks_num[piece] = 0;
						/*while(!seed->request_limit() && !m_piece_states[piece].block2download.empty())
						{
							uint32_t block = m_piece_states[piece].block2download.front();
							uint32_t block_length;
							m_torrent_file->get_block_length_by_index(piece, block, &block_length);
							if (seed->send_request(piece, block, block_length) != ERR_NO_ERROR)
								break;
							m_piece_states[piece].block2download.pop_front();
							m_piece_states[piece].taken_from.insert(seed);
							requested_blocks_num[piece]++;
						}*/
					}
				}

			}
		}

		for(std::list<uint32_t>::iterator piece_iter = m_downloadable_pieces.begin(),
				piece_iter_add = m_downloadable_pieces.begin(); piece_iter != m_downloadable_pieces.end(); piece_iter = piece_iter_add)
		{
			++piece_iter_add;
			/*uint32_t piece = *piece_iter;

			if (m_piece_states[piece].block2download.size() == m_torrent_file->get_blocks_count_in_piece(piece))
			{
				m_downloadable_pieces.erase(piece_iter);
				TORRENT_TASK task;
				task.task = TORRENT_TASK_DOWNLOAD_PIECE;
				task.task_data = piece;
				m_task_queue.push_back(task);
				m_piece_states[piece].taken_from.clear();//на пожарный случай
			}*/
		}

		for(peer_map_iter leecher_iter = m_leechers.begin(); leecher_iter != m_leechers.end(); ++leecher_iter)
		{
			PeerPtr leecher = leecher_iter->second;
			leecher->clock();
			if (leecher->is_sleep())
			{
				leecher->prepare2release();
				m_leechers.erase(leecher_iter);
			}
			else
			{
				m_rx_speed += leecher->get_rx_speed();
				m_tx_speed += leecher->get_tx_speed();
				for(std::list<uint32_t>::iterator have_iter = m_have_list.begin();
						have_iter != m_have_list.end();
						++have_iter)
				{
					leecher->send_have(*have_iter);
				}
			}
		}
		m_have_list.clear();

		for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
		{
			bool release_tracker_instance;
			(*iter).second->clock(release_tracker_instance);
			if (release_tracker_instance)
				m_trackers.erase(iter);
		}
	}
	if (m_state == TORRENT_STATE_CHECKING)
	{

	}

	/*if (m_task_queue.front().task == TORRENT_TASK_DOWNLOAD_PIECE)
		handle_download_task();

	if (m_task_queue.front().task == TORRENT_TASK_CHECK_HASH)
	{
		//printf("Checking %llu", m_task_queue.front().task_data);
		m_torrent_file->check_piece_hash(m_task_queue.front().task_data);
		m_task_queue.pop_front();
		if (m_task_queue.empty())
			continue_();
	}*/

	return 0;
}


int TorrentBase::event_piece_hash(uint32_t piece_index, bool ok, bool error)
{
	if (ok && !error)
	{
		m_have_list.push_back(piece_index);
		m_downloaded += m_piece_manager->get_piece_length(piece_index);
		//save_state();
		m_piece_manager->piece_good(piece_index);
		printf("Piece %d OK\n", piece_index);
		//printf("rx=%f tx=%f done=%d downloaded=%llu\n", m_rx_speed, m_tx_speed, m_pieces_to_download.size(), m_downloaded);
	}
	if (!ok || error)
	{
		if (m_state == TORRENT_STATE_STARTED)
		{
			TORRENT_TASK task;
			task.task = TORRENT_TASK_DOWNLOAD_PIECE;
			task.task_data = piece_index;
			m_task_queue.push_back(task);
		}
		//save_state();
		m_piece_manager->piece_bad(piece_index);
		printf("Piece %d BAD\n", piece_index);
		//printf("rx=%f tx=%f done=%d downloaded=%llu\n", m_rx_speed, m_tx_speed, m_pieces_to_download.size(), m_downloaded);
	}
	return ERR_NO_ERROR;
}

int TorrentBase::get_info(torrent_info * info)
{
	if (info == NULL)
		return ERR_NULL_REF;
	info->comment = m_metafile.comment;
	info->created_by = m_metafile.created_by;
	info->creation_date = m_metafile.creation_date;
	info->download_directory = m_download_directory;
	info->downloaded = m_downloaded;
	info->length = m_metafile.length;
	info->name = m_metafile.name;
	info->piece_count = m_metafile.piece_count;
	info->piece_length = m_metafile.piece_length;
	info->private_ = m_metafile.private_;
	info->rx_speed = m_rx_speed;
	info->tx_speed = m_tx_speed;
	info->uploaded = m_uploaded;
	memcpy(info->info_hash_hex, m_metafile.info_hash_hex, SHA1_HEX_LENGTH);
	for(peer_list_iter iter = m_active_seeders.begin(); iter != m_active_seeders.end(); ++iter)
	{
		PeerPtr seeder = *iter;
		peer_info pi;
		seeder->get_info(&pi);
		info->peers.push_back(pi);
	}
	for(peer_map_iter iter = m_leechers.begin(); iter != m_leechers.end(); ++iter)
	{
		PeerPtr leecher = (*iter).second;
		if (!leecher->is_sleep())
		{
			peer_info pi;
			leecher->get_info(&pi);
			info->peers.push_back(pi);
		}
	}
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		tracker_info ti;
		(*iter).second->get_info(&ti);
		info->trackers.push_back(ti);
	}
	for(size_t i = 0; i < m_metafile.files.size(); i++)
	{
		torrent_info::file f;
		f.first = m_metafile.files[i].name;
		f.second = m_metafile.files[i].length;
		info->file_list_.push_back(f);
	}
	if (m_state == TORRENT_STATE_CHECKING)
		info->progress = ((m_metafile.piece_count - m_task_queue.size()) * 100) / m_metafile.piece_count;
	else
		info->progress = (m_downloaded * 100) / m_metafile.length;
	return ERR_NO_ERROR;
}

int TorrentBase::erase_state()
{
	remove(m_state_file_name.c_str());
	std::string infohash_str = m_metafile.info_hash_hex;
	std::string fname =  m_work_directory + infohash_str + ".torrent";
	remove(fname.c_str());
	return ERR_NO_ERROR;
}

} /* namespace TorrentNamespace */
