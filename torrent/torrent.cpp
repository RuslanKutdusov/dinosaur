/*
 * Torrent.cpp
 *
 *  Created on: 29.01.2012
 *      Author: ruslan
 */

#include "torrent.h"

namespace torrent {

TorrentBase::TorrentBase(network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc)
	//:network::sock_event(), fs::file_event()
{
#ifdef BITTORRENT_DEBUG
	printf("Torrent default constructor\n");
#endif
	if (nm == NULL || g_cfg == NULL || fm == NULL || bc == NULL)
		throw Exception(GENERAL_ERROR_UNDEF_ERROR);

	m_nm = nm;
	m_g_cfg = g_cfg;
	m_fm = fm;
	m_bc = bc;
	m_downloaded  = 0;
	m_uploaded = 0;
	m_rx_speed = 0;
	m_tx_speed = 0;
	m_state = TORRENT_STATE_NONE;
	m_error = TORRENT_ERROR_NO_ERROR;
}

void TorrentBase::init(const Metafile & metafile, const std::string & work_directory, const std::string & download_directory)
{
	if (work_directory.length() == 0)
		throw Exception(GENERAL_ERROR_UNDEF_ERROR);
	BITFIELD bitfield = NULL;
	try{
		m_metafile = metafile;

		m_state_file_name = work_directory;
		m_state_file_name.append(m_metafile.info_hash_hex);

		StateSerializator s(m_state_file_name);
		size_t bitfield_len = ceil(m_metafile.piece_count / 8.0f);
		bitfield = new unsigned char[bitfield_len];
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

		TorrentFile::CreateTorrentFile(boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), m_download_directory, m_torrent_file);

		m_piece_manager.reset(new PieceManager(boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), bitfield));

		if (bitfield != NULL)
			delete[] bitfield;
	}
	catch( std::bad_alloc & e)
	{
		if (bitfield != NULL)
			delete[] bitfield;
		throw Exception(GENERAL_ERROR_NO_MEMORY_AVAILABLE);
	}
	catch ( Exception & e)
	{
		if (bitfield != NULL)
			delete[] bitfield;
		throw Exception(e);
	}
}

TorrentBase::~TorrentBase()
{
#ifdef BITTORRENT_DEBUG
	printf("Torrent destructor\n");
#endif
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
#ifdef BITTORRENT_DEBUG
	printf("Releasing torrent %s %s\n", m_metafile.name.c_str(), m_metafile.info_hash_hex);
#endif
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

void TorrentBase::forced_releasing()
{
	m_state = TORRENT_STATE_RELEASING;
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		(*iter).second->forced_releasing();
	}
	for(peer_map_iter iter = m_seeders.begin(); iter != m_seeders.end(); ++iter)
	{
		(*iter).second->forced_releasing();
	}
	for(peer_map_iter iter = m_leechers.begin(); iter != m_leechers.end(); ++iter)
	{
		(*iter).second->forced_releasing();
	}
	m_seeders.clear();
	m_active_seeders.clear();
	m_waiting_seeders.clear();
	m_leechers.clear();
	m_torrent_file->ReleaseFiles();
	m_torrent_file.reset();
	m_piece_manager.reset();
}

void TorrentBase::add_seeders(uint32_t count, sockaddr_in * addrs)
{
	count = count > m_g_cfg->get_tracker_numwant() ? m_g_cfg->get_tracker_numwant() : count;
	for(uint32_t i = 0;	i < count;	i++)
	{
		try
		{
			std::string key;
			get_peer_key(&addrs[i], key);
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
		get_peer_key(&sock->m_peer, key);
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

int TorrentBase::start()
{
	if (m_state == TORRENT_STATE_NONE)
	{
		m_active_seeders.clear();
		m_pieces2check.clear();
		m_work = m_downloaded == m_metafile.length ? TORRENT_UPLOADING : TORRENT_DOWNLOADING;
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
		m_state = TORRENT_STATE_STARTED;
	}
	else
		return ERR_INTERNAL;
	return ERR_NO_ERROR;
}

int TorrentBase::stop()
{
	if (m_state != TORRENT_STATE_STARTED && m_state != TORRENT_STATE_CHECKING)
		return ERR_INTERNAL;
	for(peer_list_iter p = m_active_seeders.begin(); p != m_active_seeders.end(); ++p)
	{
		BLOCK_ID block_id;
		PeerPtr seed = *p;
		while(seed->get_requested_block(block_id))
		{
			uint32_t piece, block;
			get_piece_block_from_block_id(block_id, piece, block);
			m_piece_manager->push_block2download(piece, block);
		}
		m_waiting_seeders.push_back(seed);
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
	m_state = TORRENT_STATE_STOPPED;
	m_pieces2check.clear();
	m_work = TORRENT_PAUSED;
	return 0;
}

int TorrentBase::pause()
{
	if (m_state != TORRENT_STATE_STARTED && m_state != TORRENT_STATE_CHECKING)
		return ERR_INTERNAL;
	for(peer_list_iter p = m_active_seeders.begin(); p != m_active_seeders.end(); ++p)
	{
		BLOCK_ID block_id;
		PeerPtr seed = *p;
		while(seed->get_requested_block(block_id))
		{
			uint32_t piece, block;
			get_piece_block_from_block_id(block_id, piece, block);
			m_piece_manager->push_block2download(piece, block);
		}
		m_waiting_seeders.push_back(seed);
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
	m_state = TORRENT_STATE_PAUSED;
	//m_pieces2check.clear(); не вычищаем, check->pause->check норм продолжит проверку, check->pause->continue->check начнет сначала проверку
	m_work = TORRENT_PAUSED;
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
	m_state = TORRENT_STATE_STARTED;
	m_pieces2check.clear();
	m_work = m_downloaded == m_metafile.length ? TORRENT_UPLOADING : TORRENT_DOWNLOADING;
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
		BLOCK_ID block_id;
		PeerPtr seed = *p;
		while(seed->get_requested_block(block_id))
		{
			uint32_t piece, block;
			get_piece_block_from_block_id(block_id, piece, block);
			m_piece_manager->push_block2download(piece, block);
		}
		m_waiting_seeders.push_back(seed);
	}
	m_active_seeders.clear();

	for(peer_map_iter p = m_leechers.begin(); p != m_leechers.end(); ++p)
	{
		(*p).second->prepare2release();
	}
	m_leechers.clear();

	m_downloaded = 0;
	m_piece_manager->reset();
	m_pieces2check.clear();
	m_downloadable_pieces.clear();
	for(uint32_t i = 0; i < m_metafile.piece_count; i++)
	{
		m_pieces2check.push_back(i);
	}
	m_state = TORRENT_STATE_CHECKING;
	m_work = TORRENT_CHECKING;
	return ERR_NO_ERROR;
}

int TorrentBase::clock()
{
	m_rx_speed = 0.0f;
	m_tx_speed = 0.0f;
	//TODO проверять насколько загружен кэш FileManager, напр. если свободно в кэше менее 50%-70% - работаем на всю катушку, иначе, как то ограничиваем себя
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
			{
				m_active_seeders.push_back(seed);
#ifdef BITTORRENT_DEBUG
				printf("Pushing seed %s to active seeders\n", seed->get_ip_str().c_str());
#endif
			}
		}
		if (!m_active_seeders.empty())
		{
			//++seed_iter_add;
			peer_list_iter seed_iter = m_active_seeders.begin();
			PeerPtr seed = *seed_iter;
			seed->clock();
			if (seed->is_sleep())
			{
				BLOCK_ID block_id;
				while(seed->get_requested_block(block_id))
				{
					uint32_t piece, block;
					get_piece_block_from_block_id(block_id, piece, block);
					m_piece_manager->push_block2download(piece, block);
				}
				m_active_seeders.erase(seed_iter);
				m_waiting_seeders.push_back(seed);
				#ifdef BITTORRENT_DEBUG
								printf("Pushing seed %s to waiting seeders\n", seed->get_ip_str().c_str());
				#endif
			}
			else
			{
				m_active_seeders.erase(seed_iter);
				m_active_seeders.push_back(seed);
				for(std::list<uint32_t>::iterator have_iter = m_have_list.begin();
						have_iter != m_have_list.end();
						++have_iter)
				{
					seed->send_have(*have_iter);
				}
				uint32_t piece_index;
				if (m_downloadable_pieces.size() < m_active_seeders.size() && m_piece_manager->front_piece2download(piece_index) == ERR_NO_ERROR)
				{
					m_downloadable_pieces.push_back(piece_index);
					m_piece_manager->pop_piece2download();
					#ifdef BITTORRENT_DEBUG
					printf("Piece %u now is downloadable\n", piece_index);
					#endif
				}
				if (m_downloadable_pieces.size() > 0)
				{
					uint32_t piece_index  = m_downloadable_pieces.front();
					m_downloadable_pieces.pop_front();
					m_downloadable_pieces.push_back(piece_index);
					if (m_piece_manager->get_block2download_count(piece_index) != 0 && seed->may_request() && seed->have_piece(piece_index))
					{
						uint32_t block_index;
						while(m_piece_manager->get_block2download(piece_index, block_index) == ERR_NO_ERROR)
						{
							seed->request(piece_index, block_index);
						}
						m_piece_manager->set_piece_taken_from(piece_index, seed->get_ip_str());
						#ifdef BITTORRENT_DEBUG
						printf("Piece %u is requested from seed %s\n", piece_index, seed->get_ip_str().c_str());
						#endif
					}
				}

			}
		}

		for(peer_list_iter seed_iter = m_active_seeders.begin(); seed_iter != m_active_seeders.end(); ++seed_iter)
		{
			PeerPtr seed = *seed_iter;
			m_rx_speed += seed->get_rx_speed();
			m_tx_speed += seed->get_tx_speed();
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
		if (m_pieces2check.empty())
			return continue_();
		std::list<uint32_t>::iterator iter = m_pieces2check.begin();
		m_piece_manager->check_piece_hash(*iter);
		m_pieces2check.erase(iter);
	}

	return 0;
}

int TorrentBase::event_file_write(const fs::write_event & we)
{
	PIECE_STATE piece_state;
	uint32_t piece_index = we.block_id.first;
	m_piece_manager->event_file_write(we, piece_state);
	if (piece_state == PIECE_STATE_FIN_HASH_OK)
	{
		m_downloadable_pieces.remove(piece_index);
		StateSerializator s(m_state_file_name);
		BITFIELD bitfield = new unsigned char[m_piece_manager->get_bitfield_length()];
		m_piece_manager->copy_bitfield(bitfield);
		s.serialize(m_uploaded, m_download_directory, bitfield,m_piece_manager->get_bitfield_length());
		delete[] bitfield;
		m_piece_manager->clear_piece_taken_from(piece_index);
		m_work = m_downloaded == m_metafile.length ? TORRENT_UPLOADING : TORRENT_DOWNLOADING;
		return ERR_NO_ERROR;
	}
	if (piece_state == PIECE_STATE_FIN_HASH_BAD)
	{
		m_downloadable_pieces.remove(piece_index);
		std::string seed;
		while(m_piece_manager->get_piece_taken_from(piece_index, seed))
		{
			m_seeders[seed]->goto_sleep();
		}
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
	info->total_seeders = m_seeders.size();
	memcpy(info->info_hash_hex, m_metafile.info_hash_hex, SHA1_HEX_LENGTH);
	for(peer_list_iter iter = m_active_seeders.begin(); iter != m_active_seeders.end(); ++iter)
	{
		PeerPtr seeder = *iter;
		peer_info pi;
		seeder->get_info(&pi);
		info->seeders.push_back(pi);
	}
	for(peer_map_iter iter = m_leechers.begin(); iter != m_leechers.end(); ++iter)
	{
		PeerPtr leecher = (*iter).second;
		if (!leecher->is_sleep())
		{
			peer_info pi;
			leecher->get_info(&pi);
			info->leechers.push_back(pi);
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
		info->progress = ((m_metafile.piece_count - m_pieces2check.size()) * 100) / m_metafile.piece_count;
	else
		info->progress = (m_downloaded * 100) / m_metafile.length;
	return ERR_NO_ERROR;
}

int TorrentBase::erase_state()
{
	remove(m_state_file_name.c_str());
	return ERR_NO_ERROR;
}

} /* namespace TorrentNamespace */
