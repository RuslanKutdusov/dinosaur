/*
 * Torrent.cpp
 *
 *  Created on: 29.01.2012
 *      Author: ruslan
 */

#include "torrent.h"

namespace dinosaur {
namespace torrent {

TorrentBase::TorrentBase(network::NetworkManager * nm, cfg::Glob_cfg * g_cfg, fs::FileManager * fm, block_cache::Block_cache * bc)
	//:network::sock_event(), fs::file_event()
{
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Torrent default constructor";
#endif
	if (nm == NULL || g_cfg == NULL || fm == NULL || bc == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);

	m_nm = nm;
	m_g_cfg = g_cfg;
	m_fm = fm;
	m_bc = bc;
	m_downloaded  = 0;
	m_uploaded = 0;
	m_rx_speed = 0;
	m_tx_speed = 0;
	m_state = TORRENT_STATE_NONE;
	m_failure_desc.errno_ = 0;
	m_failure_desc.exception_errcode = Exception::NO_ERROR;
	m_remain_time = 0;
}

void TorrentBase::init(const Metafile & metafile, const std::string & work_directory, const std::string & download_directory)
{
	if (work_directory.length() == 0)
		throw Exception(Exception::ERR_CODE_UNDEF);
	BITFIELD bitfield = NULL;
	try{
		m_metafile = metafile;

		m_state_file_name = work_directory;
		m_state_file_name.append(m_metafile.info_hash_hex);

		StateSerializator s(m_state_file_name);
		size_t bitfield_len = ceil(m_metafile.piece_count / 8.0f);
		bool file_should_exists = true;
		bitfield = new unsigned char[bitfield_len];
		if (s.deserialize(m_uploaded, m_download_directory, bitfield, bitfield_len, m_start_time, (int&)m_state) == -1)
		{
			m_download_directory = download_directory;
			if (m_download_directory[m_download_directory.length() - 1] != '/')
				m_download_directory.append("/");
			m_uploaded = 0;
			memset(bitfield, 0, bitfield_len);
			m_start_time = time(NULL);
			m_state = TORRENT_STATE_STARTED;
			s.serialize(m_uploaded, m_download_directory, bitfield, bitfield_len, m_start_time, m_state);
			delete[] bitfield;
			bitfield = NULL;
			file_should_exists = false;
		}


		uint32_t files_exists;
		TorrentFile::CreateTorrentFile(boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), m_download_directory, file_should_exists, files_exists, m_torrent_file);

		m_piece_manager.reset(new PieceManager(boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), bitfield));

		if (bitfield != NULL)
			delete[] bitfield;


		for (std::vector<std::string>::iterator iter = m_metafile.announces.begin(); iter != m_metafile.announces.end(); ++iter)
		{
			TrackerPtr temp;
			temp.reset(new Tracker(boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), *iter));
			m_trackers[*iter] = temp;
		}

		if (m_state == TORRENT_STATE_STARTED)
			m_work = m_downloaded == m_metafile.length ? TORRENT_WORK_UPLOADING : TORRENT_WORK_DOWNLOADING;
		if (m_state == TORRENT_STATE_PAUSED)
			m_work = TORRENT_WORK_PAUSED;
		if (m_state == TORRENT_STATE_DONE)
			m_work = TORRENT_WORK_DONE;
	}
	catch( std::bad_alloc & e)
	{
		if (bitfield != NULL)
			delete[] bitfield;
		torrent_failure tf;
		tf.exception_errcode = Exception::ERR_CODE_NO_MEMORY_AVAILABLE;
		tf.errno_ = 0;
		tf.description = metafile.name;
		tf.where = TORRENT_FAILURE_INIT_TORRENT;
		set_failure(tf);
	}
	catch ( Exception & e)
	{
		if (bitfield != NULL)
			delete[] bitfield;
		torrent_failure tf;
		tf.exception_errcode = e.get_errcode();
		tf.errno_ = 0;
		tf.description = metafile.name;
		tf.where = TORRENT_FAILURE_INIT_TORRENT;
		set_failure(tf);
	}
	catch( SyscallException & e )
	{
		if (bitfield != NULL)
			delete[] bitfield;
		torrent_failure tf;
		tf.exception_errcode = Exception::NO_ERROR;
		tf.errno_ = e.get_errno();
		tf.description = metafile.name;
		tf.where = TORRENT_FAILURE_INIT_TORRENT;
		set_failure(tf);
	}
}

TorrentBase::~TorrentBase()
{
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Torrent destructor " <<  m_metafile.name.c_str() << " " << m_metafile.info_hash_hex;
#endif
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Torrent destroyed " <<  m_metafile.name.c_str() << " " << m_metafile.info_hash_hex;
#endif
}

void TorrentBase::save_state()
{
	StateSerializator s(m_state_file_name);
	BITFIELD bitfield = new unsigned char[m_piece_manager->get_bitfield_length()];
	m_piece_manager->copy_bitfield(bitfield);
	int state;
	switch (m_state) {
		case TORRENT_STATE_CHECKING:
			state = TORRENT_STATE_PAUSED;
			break;
		case TORRENT_STATE_FAILURE:
			state = TORRENT_STATE_PAUSED;
			break;
		case TORRENT_STATE_INIT_FORCED_RELEASING:
			state = TORRENT_STATE_PAUSED;
			break;
		case TORRENT_STATE_INIT_RELEASING:
			state = TORRENT_STATE_PAUSED;
			break;
		case TORRENT_STATE_NONE:
			state = TORRENT_STATE_PAUSED;
			break;
		case TORRENT_STATE_PAUSED:
			state = TORRENT_STATE_PAUSED;
			break;
		case TORRENT_STATE_RELEASING:
			state = TORRENT_STATE_PAUSED;
			break;
		case TORRENT_STATE_STARTED:
			state = TORRENT_STATE_STARTED;
			break;
		case TORRENT_STATE_DONE:
			state = TORRENT_STATE_DONE;
			break;

		default:
			break;
	}
	s.serialize(m_uploaded, m_download_directory, bitfield,m_piece_manager->get_bitfield_length(), m_start_time, state);
	delete[] bitfield;
}

void TorrentBase::prepare2release()
{
	if (m_state == TORRENT_STATE_INIT_RELEASING || m_state == TORRENT_STATE_INIT_FORCED_RELEASING || m_state == TORRENT_STATE_RELEASING)
		return;
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Releasing torrent " <<  m_metafile.name.c_str() << " " << m_metafile.info_hash_hex;
#endif
	save_state();
	m_state = TORRENT_STATE_INIT_RELEASING;
}

void TorrentBase::forced_releasing()
{
	if (m_state == TORRENT_STATE_INIT_RELEASING || m_state == TORRENT_STATE_INIT_FORCED_RELEASING || m_state == TORRENT_STATE_RELEASING)
		return;
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Forced releasing torrent " <<  m_metafile.name.c_str() << " " << m_metafile.info_hash_hex;
#endif
	save_state();
	m_state = TORRENT_STATE_INIT_FORCED_RELEASING;
}

void TorrentBase::releasing()
{
	for(tracker_map_iter iter = m_trackers.begin(), iter2 = iter;
					iter != m_trackers.end(); iter = iter2)
	{
		++iter2;
		if (m_state == TORRENT_STATE_INIT_FORCED_RELEASING)
		{
			(*iter).second->forced_releasing();
			m_trackers.erase(iter);
		}
		else
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

	if (m_torrent_file != NULL)
		m_torrent_file->ReleaseFiles();
	m_torrent_file.reset();
	m_piece_manager.reset();
}

void TorrentBase::add_seeders(uint32_t count, sockaddr_in * addrs)
{
	if (addrs == NULL || count == 0)
		return;
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

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_SEED_REJECTED
 */

void TorrentBase::add_seeder(sockaddr_in * addr)
{
	if (addr == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	if (m_state != TORRENT_STATE_STARTED)
		throw Exception(Exception::ERR_CODE_LEECHER_REJECTED);
	std::string key;
	get_peer_key(addr, key);
	try
	{
		if (m_seeders.count(key) == 0)
		{
			PeerPtr peer(new Peer());
			peer->Init(addr, boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()));
			m_seeders[key] = peer;
			m_waiting_seeders.push_back(peer);
		}
		else
		{
			throw Exception(Exception::ERR_CODE_SEED_REJECTED);
		}
	}
	catch(Exception & e)
	{
		throw Exception(e);
	}
	catch(std::bad_alloc & e)
	{
		throw Exception(Exception::ERR_CODE_SEED_REJECTED);
	}
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_LEECHER_REJECTED
 */

void TorrentBase::add_leecher(network::Socket & sock)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	if (m_state != TORRENT_STATE_STARTED)
		throw Exception(Exception::ERR_CODE_LEECHER_REJECTED);
	try
	{
		std::string key;
		get_peer_key(&sock->m_peer, key);
		//LOG(INFO) << "add incoming peer %s\n", key.c_str());
		if ((m_leechers.count(key) == 0 || m_leechers[key] == NULL) && m_leechers.size() < m_g_cfg->get_max_active_leechers())
		{
			PeerPtr peer(new Peer());
			peer->Init(sock, boost::static_pointer_cast<TorrentInterfaceInternal>(shared_from_this()), PEER_ADD_INCOMING);
			m_leechers[key] = peer;
		}
		else
		{
			throw Exception(Exception::ERR_CODE_LEECHER_REJECTED);
		}
	}
	catch(Exception & e)
	{
		throw Exception(e);
	}
	catch(std::bad_alloc & e)
	{
		throw Exception(Exception::ERR_CODE_LEECHER_REJECTED);
	}
}

/*
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void TorrentBase::pause()
{
	if (m_state != TORRENT_STATE_STARTED && m_state != TORRENT_STATE_CHECKING && m_state != TORRENT_STATE_SET_FAILURE)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);
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
		seed->goto_sleep();
		m_waiting_seeders.push_back(seed);
	}
	m_active_seeders.clear();
	for(peer_map_iter p = m_leechers.begin(); p != m_leechers.end(); ++p)
	{
		(*p).second->prepare2release();
	}
	m_leechers.clear();
	if (m_state == TORRENT_STATE_STARTED)
		for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
		{
			(*iter).second->send_stopped();
		}
	m_state = TORRENT_STATE_PAUSED;
	//m_pieces2check.clear(); не вычищаем, check->pause->check норм продолжит проверку, check->pause->continue->check начнет сначала проверку
	m_work = TORRENT_WORK_PAUSED;
}

/*
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void TorrentBase::continue_()
{
	if (m_state != TORRENT_STATE_PAUSED && m_state != TORRENT_STATE_CHECKING)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);
	for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
	{
		(*iter).second->send_started();
	}
	m_state = TORRENT_STATE_STARTED;
	m_pieces2check.clear();
	m_work = m_downloaded == m_metafile.length ? TORRENT_WORK_UPLOADING : TORRENT_WORK_DOWNLOADING;
}


bool TorrentBase::is_downloaded()
{
	return m_downloaded == m_metafile.length;
}

/*
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void TorrentBase::check()
{
	if (m_state != TORRENT_STATE_PAUSED && m_state != TORRENT_STATE_STARTED && m_state != TORRENT_STATE_DONE)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);

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
		seed->goto_sleep();
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
	m_torrent_file->clear_file_downloaded();
	for(uint32_t i = 0; i < m_metafile.piece_count; i++)
	{
		m_pieces2check.push_back(i);
	}
	m_state = TORRENT_STATE_CHECKING;
	m_work = TORRENT_WORK_CHECKING;
}

void TorrentBase::set_failure(const torrent_failure & tf)
{
	m_failure_desc = tf;
	m_state = TORRENT_STATE_SET_FAILURE;
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Torrent " << m_metafile.name.c_str() << " FAILURE";
	LOG(INFO) << "Where: " << m_failure_desc.where;
	LOG(INFO) << "errno: " << sys_errlist[m_failure_desc.errno_];
	LOG(INFO) << "exc_err: " <<  exception_errcode2str(m_failure_desc.exception_errcode).c_str();
	LOG(INFO) << "desc: " <<  m_failure_desc.description.c_str();
#endif
}

int TorrentBase::clock()
{
	m_rx_speed = 0.0f;
	m_tx_speed = 0.0f;
	m_remain_time = 0;
	m_ratio = m_downloaded == 0 ? 0.0f : (float)m_uploaded / (float)m_downloaded;
	//TODO проверять насколько загружен кэш FileManager, напр. если свободно в кэше менее 50%-70% - работаем на всю катушку, иначе, как то ограничиваем себя
	if (m_state == TORRENT_STATE_NONE)
	{
		torrent_failure  tf;
		tf.errno_ = 0;
		tf.exception_errcode = Exception::ERR_CODE_UNDEF;
		tf.where = TORRENT_FAILURE_DOWNLOADING;
		set_failure(tf);
		return 0;
	}
	if (m_state == TORRENT_STATE_INIT_RELEASING || m_state == TORRENT_STATE_INIT_FORCED_RELEASING)
	{
		releasing();
		m_state = TORRENT_STATE_RELEASING;
	}
	if (m_state == TORRENT_STATE_RELEASING)
		for(tracker_map_iter iter = m_trackers.begin(), iter2 = iter;
				iter != m_trackers.end(); iter = iter2)
		{
			++iter2;
			bool release_tracker_instance;
			(*iter).second->clock(release_tracker_instance);
			if (release_tracker_instance)
				m_trackers.erase(iter);
		}

	if (m_state == TORRENT_STATE_SET_FAILURE)
	{
		#ifdef BITTORRENT_DEBUG
		LOG(INFO) << "Stopping due failure";
		#endif
		pause();
		m_state = TORRENT_STATE_FAILURE;
		m_work = TORRENT_WORK_FAILURE;
	}
	if (m_state == TORRENT_STATE_STARTED)
	{
		if (m_ratio > m_g_cfg->get_fin_ratio() && is_downloaded())
		{
			pause();
			m_state = TORRENT_STATE_DONE;
			m_work = TORRENT_WORK_DONE;
			return 0;
		}
		if (m_active_seeders.size() < m_g_cfg->get_max_active_seeders() &&
				m_waiting_seeders.size() > 0 && !is_downloaded())
		{
			PeerPtr seed = m_waiting_seeders.front();
			m_waiting_seeders.pop_front();
			if (seed->is_sleep())
				m_waiting_seeders.push_back(seed);
			else
			{
				m_active_seeders.push_back(seed);
				#ifdef BITTORRENT_DEBUG
				LOG(INFO) << "Pushing seed " << seed->get_ip_str().c_str() << " to active seeders\n";
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
				LOG(INFO) << "Pushing seed " << seed->get_ip_str().c_str() << " to waitig seeders\n";
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
					LOG(INFO) << "Piece " << piece_index << " now is downloadable\n";
					#endif
				}
				if (!m_downloadable_pieces.empty())
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
						LOG(INFO) << "Piece " << piece_index << " is requested from seed " <<  seed->get_ip_str().c_str();
						#endif
					}
				}

			}
		}

		if (m_piece_manager->queue_empty() && m_downloadable_pieces.empty() && !is_downloaded())
		{
			torrent_failure tf;
			tf.where = TORRENT_FAILURE_DOWNLOADING;
			tf.errno_ = 0;
			tf.exception_errcode = Exception::ERR_CODE_UNDEF;
			set_failure(tf);
		}

		for(peer_list_iter seed_iter = m_active_seeders.begin(); seed_iter != m_active_seeders.end(); ++seed_iter)
		{
			PeerPtr seed = *seed_iter;
			m_rx_speed += seed->get_rx_speed();
			m_tx_speed += seed->get_tx_speed();
		}

		for(peer_map_iter leecher_iter = m_leechers.begin(), leecher_iter2 = leecher_iter; leecher_iter != m_leechers.end(); leecher_iter = leecher_iter2)
		{
			++leecher_iter2;
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
				if (leecher->peer_interested())
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
		}

		if (m_rx_speed > 0)
			m_remain_time = (m_metafile.length - m_downloaded) / m_rx_speed;
		else
			m_remain_time = 0;
	}

	if (m_state == TORRENT_STATE_CHECKING)
	{
		if (m_pieces2check.empty())
		{
			if (!is_downloaded() || m_ratio < m_g_cfg->get_fin_ratio())
				continue_();
			else
			{
				m_state = TORRENT_STATE_DONE;
				m_work = TORRENT_WORK_DONE;
				save_state();
			}
			return ERR_NO_ERROR;
		}
		std::list<uint32_t>::iterator iter = m_pieces2check.begin();
		if (m_piece_manager->check_piece_hash(*iter))
			m_torrent_file->update_file_downloaded(*iter);
		m_pieces2check.erase(iter);
		save_state();
	}
	return 0;
}

int TorrentBase::event_file_write(const fs::write_event & we)
{
	PIECE_STATE piece_state;
	uint32_t piece_index = we.block_id.first;
	m_piece_manager->event_file_write(we, piece_state);
	if (we.writted == -1)
	{
		torrent_failure tf;
		tf.description = we.file->get_name();
		tf.errno_ = we.errno_;
		tf.exception_errcode = we.exception_errcode;
		tf.where = TORRENT_FAILURE_WRITE_FILE;
		set_failure(tf);
		return ERR_INTERNAL;
	}
	if (piece_state == PIECE_STATE_FIN_HASH_OK)
	{
		m_downloadable_pieces.remove(piece_index);

		save_state();

		m_piece_manager->clear_piece_taken_from(piece_index);

		m_work = m_downloaded == m_metafile.length ? TORRENT_WORK_UPLOADING : TORRENT_WORK_DOWNLOADING;
		if (m_state == TORRENT_STATE_STARTED && m_downloaded == m_metafile.length)
			for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
				(*iter).second->send_completed();

		if (m_g_cfg->get_send_have())
			m_have_list.push_back(piece_index);

		m_torrent_file->update_file_downloaded(piece_index);

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

void TorrentBase::get_info_stat(info::torrent_stat & ref)
{
	ref.comment = m_metafile.comment;
	ref.created_by = m_metafile.created_by;
	ref.creation_date = m_metafile.creation_date;
	ref.download_directory = m_download_directory;
	ref.length = m_metafile.length;
	ref.name = m_metafile.name;
	ref.piece_count = m_metafile.piece_count;
	ref.piece_length = m_metafile.piece_length;
	ref.private_ = m_metafile.private_;
	ref.start_time = m_start_time;
	ref.files_count = m_metafile.files.size();
	memcpy(ref.info_hash_hex, m_metafile.info_hash_hex, SHA1_HEX_LENGTH);
}

void TorrentBase::get_info_dyn(info::torrent_dyn & ref)
{
	ref.downloaded = m_downloaded;
	ref.uploaded = m_uploaded;
	ref.rx_speed = m_rx_speed;
	ref.tx_speed = m_tx_speed;
	ref.seeders  = m_active_seeders.size();
	ref.total_seeders = m_seeders.size();
	ref.leechers = m_leechers.size();
	if (m_state == TORRENT_STATE_CHECKING)
		ref.progress = ((m_metafile.piece_count - m_pieces2check.size()) * 100) / m_metafile.piece_count;
	else
		ref.progress = (m_downloaded * 100) / m_metafile.length;
	ref.work = m_work;
	ref.remain_time = m_remain_time;
	ref.ratio = m_ratio;
}

void TorrentBase::get_info_trackers(info::trackers & ref)
{
	ref.clear();
	if (m_state == TORRENT_STATE_STARTED)
		for(tracker_map_iter iter = m_trackers.begin(); iter != m_trackers.end(); ++iter)
		{
			info::tracker t;
			(*iter).second->get_info(t);
			ref.push_back(t);
		}
}

void TorrentBase::get_info_file_stat(FILE_INDEX index, info::file_stat & ref)
{
	if (index >= m_metafile.files.size())
		throw Exception(Exception::ERR_CODE_INVALID_FILE_INDEX);
    ref = m_metafile.files[index];
}

/*
 * Exception::ERR_CODE_INVALID_FILE_INDEX
 */

void TorrentBase::get_info_file_dyn(FILE_INDEX index, info::file_dyn & ref)
{
	if (index >= m_metafile.files.size())
		throw Exception(Exception::ERR_CODE_INVALID_FILE_INDEX);
	m_torrent_file->get_file_priority(index, ref.priority);
	m_torrent_file->get_file_downloaded(index, ref.downloaded);
}

void TorrentBase::get_info_seeders(info::peers & ref)
{
	ref.clear();
	for(peer_list_iter iter = m_active_seeders.begin(); iter != m_active_seeders.end(); ++iter)
	{
		info::peer p;
		(*iter)->get_info(p);
		ref.push_back(p);
	}
}

void TorrentBase::get_info_leechers(info::peers & ref)
{
	ref.clear();
	for(peer_map_iter iter = m_leechers.begin(); iter != m_leechers.end(); ++iter)
	{
		info::peer l;
		(*iter).second->get_info(l);
		ref.push_back(l);
	}
}

void TorrentBase::get_info_downloadable_pieces(info::downloadable_pieces & ref)
{
	ref.clear();
	for(std::list<uint32_t>::iterator iter = m_downloadable_pieces.begin(); iter != m_downloadable_pieces.end(); ++iter)
	{
		info::downloadable_piece dp;
		dp.block2download = m_piece_manager->get_block2download_count(*iter);
		dp.downloaded_blocks = m_piece_manager->get_donwloaded_blocks_count(*iter);
		dp.index = *iter;
		m_piece_manager->get_piece_priority(*iter, dp.priority);
		ref.push_back(dp);
	}
}


int TorrentBase::erase_state()
{
	remove(m_state_file_name.c_str());
	return ERR_NO_ERROR;
}

/*
 * Exception::ERR_CODE_INVALID_FILE_INDEX
 * Exception::ERR_CODE_FAIL_SET_FILE_PRIORITY
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void TorrentBase::set_file_priority(FILE_INDEX file, DOWNLOAD_PRIORITY prio)
{
	if (m_state == TORRENT_STATE_CHECKING)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);
	if (file >= m_metafile.files.size())
		throw Exception(Exception::ERR_CODE_INVALID_FILE_INDEX);
	DOWNLOAD_PRIORITY old_prio;
	m_torrent_file->get_file_priority(file, old_prio);
	if (old_prio == prio)
		return;
	m_torrent_file->set_file_priority(file, prio);
	if (m_piece_manager->set_file_priority(file, prio) != ERR_NO_ERROR)
	{
		m_torrent_file->set_file_priority(file, old_prio);
		throw Exception(Exception::ERR_CODE_FAIL_SET_FILE_PRIORITY);
	}
}

const torrent_failure & TorrentBase::get_failure_desc()
{
	return m_failure_desc;
}

} /* namespace TorrentNamespace */
}
