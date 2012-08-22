/*
 * piece_manager.cpp
 *
 *  Created on: 12.07.2012
 *      Author: ruslan
 */

#include "torrent.h"

namespace dinosaur {
namespace torrent
{

/*
 * Exception::ERR_CODE_NO_MEMORY_AVAILABLE
 */

PieceManager::PieceManager(const TorrentInterfaceInternalPtr & torrent, unsigned char * bitfield)
{
	m_torrent = torrent;
	uint32_t piece_count = m_torrent->get_piece_count();

	m_bitfield_len = ceil(piece_count / 8.0f);
	m_bitfield = new(std::nothrow) unsigned char[m_bitfield_len];
	if (m_bitfield == NULL)
		throw Exception(Exception::ERR_CODE_NO_MEMORY_AVAILABLE);

	if (bitfield == NULL)
		memset(m_bitfield, 0, m_bitfield_len);
	else
		memcpy(m_bitfield, bitfield, m_bitfield_len);

	m_piece_for_check_hash = new(std::nothrow) unsigned char[m_torrent->get_piece_length()];
	if (m_piece_for_check_hash == NULL)
	{
		delete[] bitfield;
		throw Exception(Exception::ERR_CODE_NO_MEMORY_AVAILABLE);
	}


	m_tag_list.push_back(1);
	build_piece_info();
}

PieceManager::~PieceManager()
{
	if (m_bitfield != NULL)
		delete[] m_bitfield;
	if (m_piece_for_check_hash != NULL)
		delete[] m_piece_for_check_hash;
}

void PieceManager::reset()
{
	m_downloadable_blocks.clear();
	m_download_queue.high_prio_pieces.clear();
	m_download_queue.normal_prio_pieces.clear();
	m_download_queue.low_prio_pieces.clear();
	memset(m_bitfield, 0, m_bitfield_len);
	uint32_t piece_count = m_torrent->get_piece_count();
	for (uint32_t i = 0; i < piece_count; i++)
	{
		//m_piece_info[i].prio = DOWNLOAD_PRIORITY_NORMAL;

		//m_download_queue.normal_prio_pieces.push_back(i);
		//m_piece_info[i].prio_iter = --m_download_queue.normal_prio_pieces.end();

		//for(uint32_t block = 0; block < m_piece_info[i].block_count; block++)
		//	m_piece_info[i].block2download.insert(block);

		m_piece_info[i].prio_iter = m_tag_list.begin();

		m_piece_info[i].block2download.clear();
		m_piece_info[i].downloaded_blocks.clear();

		m_piece_info[i].taken_from.clear();
	}
}

void PieceManager::build_piece_info()
{
	uint64_t piece_length_ = m_torrent->get_piece_length();
	uint64_t length = m_torrent->get_length();
	size_t file_count = m_torrent->get_files_count();
	uint64_t piece_count = m_torrent->get_piece_count();
	m_piece_info.resize(piece_count);
	FILE_INDEX file_index = 0;
	FILE_INDEX file_iter = 0;
	uint64_t last_files_length = 0;
	for (PIECE_INDEX piece_index = 0; piece_index < piece_count; piece_index++)
	{
		FILE_OFFSET offset2piece_begin = piece_index * piece_length_;//смещение до начала куска
		uint64_t piece_length = (piece_index == piece_count - 1) ? length - piece_length_ * piece_index : piece_length_; //размер последнего куска может быть меньше m_piece_length

		m_piece_info[piece_index].block_count = (uint32_t)ceil((double)piece_length / (double)BLOCK_LENGTH);
		bool need2download = (!bit_in_bitfield(piece_index, piece_count, m_bitfield));


		for(BLOCK_INDEX block = 0; block < m_piece_info[piece_index].block_count; block++)
		{
			if (last_files_length <= offset2piece_begin)
			{
				for(FILE_INDEX j = file_iter; j < file_count; j++)
				{

					info::file_stat * finfo = m_torrent->get_file_info(j);
					last_files_length += finfo->length;//считает размер файлов, когда он станет больше смещения => кусок начинается в j-ом файле
					if (last_files_length >= offset2piece_begin)
					{
						file_index = j;
						file_iter = j + 1;
						break;
					}
				}
			}


			info::file_stat * finfo = m_torrent->get_file_info(file_index);
			FILE_OFFSET file_offset = last_files_length - finfo->length;//смещение до file_index файла
			if (block == 0)
			{
				m_piece_info[piece_index].file_index = file_index;
				m_piece_info[piece_index].offset = offset2piece_begin - file_offset;//смещение до начала куска внутри файла
			}

			std::pair<FILE_INDEX, FILE_OFFSET> block_info;
			block_info.first  = file_index;
			block_info.second = offset2piece_begin - file_offset;
			m_piece_info[piece_index].block_info.push_back(block_info);

			offset2piece_begin += BLOCK_LENGTH;
			if (need2download)
				m_piece_info[piece_index].block2download.insert(block);
		}


		m_piece_info[piece_index].length = piece_length;
		m_torrent->copy_piece_hash(m_piece_info[piece_index].hash, piece_index);
		m_piece_info[piece_index].prio = DOWNLOAD_PRIORITY_NORMAL;

		if (need2download)
		{
			m_download_queue.normal_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.normal_prio_pieces.end();
		}
		else
		{
			m_torrent->inc_downloaded(piece_length);
			m_piece_info[piece_index].prio_iter = m_tag_list.begin();
		}

		FILE_OFFSET offset = m_piece_info[piece_index].offset;
		FILE_INDEX fi = m_piece_info[piece_index].file_index;
		uint32_t pos = 0;

		while(pos < piece_length)
		{
			uint64_t remain = m_torrent->get_file_info(fi)->length - offset;
			if (fi >= m_file_contains_pieces.size())
				m_file_contains_pieces.resize(fi + 1);
			m_file_contains_pieces[fi].insert(piece_index);
			uint64_t bytes = piece_length - pos > remain ? remain : piece_length - pos;
			if (!need2download)
				m_torrent->update_file_downloaded(fi, bytes);

			fi++;

			pos += bytes;
			offset = 0;
		}
	}
}

void PieceManager::get_blocks_count_in_piece(PIECE_INDEX piece_index, uint32_t & blocks_count)
{
	blocks_count =  m_piece_info[piece_index].block_count;
}

void PieceManager::get_piece_length(PIECE_INDEX piece_index, uint32_t & piece_length)
{
	piece_length = m_piece_info[piece_index].length;
}


void PieceManager::get_block_index_by_offset(PIECE_INDEX piece_index, BLOCK_OFFSET block_offset, BLOCK_INDEX & index)
{
	uint32_t block_index = block_offset / BLOCK_LENGTH;
	index = block_index;
}

void PieceManager::get_block_length_by_index(PIECE_INDEX piece_index, BLOCK_INDEX block_index, uint32_t & block_length)
{
	block_length = BLOCK_LENGTH;
	if (piece_index == m_piece_info.size() - 1 && block_index == m_piece_info[piece_index].block_count - 1)
	{
		block_length = m_piece_info[piece_index].length - BLOCK_LENGTH * block_index;
	}
}

size_t PieceManager::get_bitfield_length()
{
	return m_bitfield_len;
}

void PieceManager::get_piece_offset(PIECE_INDEX piece_index, FILE_OFFSET & offset)
{
	offset = m_piece_info[piece_index].offset;
}

void PieceManager::get_block_info(PIECE_INDEX piece_index, BLOCK_INDEX block_index, FILE_INDEX & file_index, FILE_OFFSET & file_offset)
{
	file_index = m_piece_info[piece_index].block_info[block_index].first;
	file_offset = m_piece_info[piece_index].block_info[block_index].second;
}

void PieceManager::get_file_index_by_piece(PIECE_INDEX piece_index, FILE_INDEX & index)
{
	index = m_piece_info[piece_index].file_index;
}

void PieceManager::copy_bitfield(unsigned char * dst)
{
	memcpy(dst, m_bitfield, m_bitfield_len);
}

int PieceManager::front_piece2download(PIECE_INDEX & piece_index)
{
	if (!m_download_queue.high_prio_pieces.empty())
	{
		piece_index = m_download_queue.high_prio_pieces.front();
		return ERR_NO_ERROR;
	}
	if (!m_download_queue.normal_prio_pieces.empty())
	{
		piece_index = m_download_queue.normal_prio_pieces.front();
		return ERR_NO_ERROR;
	}
	if (!m_download_queue.low_prio_pieces.empty())
	{
		piece_index = m_download_queue.low_prio_pieces.front();
		return ERR_NO_ERROR;
	}
	return ERR_EMPTY_QUEUE;
}

void PieceManager::pop_piece2download()
{
	if (!m_download_queue.high_prio_pieces.empty())
	{
		uint32_t piece_index = m_download_queue.high_prio_pieces.front();
		m_piece_info[piece_index].prio_iter = m_tag_list.begin();
		m_download_queue.high_prio_pieces.pop_front();
		return;

	}
	if (!m_download_queue.normal_prio_pieces.empty())
	{
		uint32_t piece_index = m_download_queue.normal_prio_pieces.front();
		m_piece_info[piece_index].prio_iter = m_tag_list.begin();
		m_download_queue.normal_prio_pieces.pop_front();
		return;

	}
	if (!m_download_queue.low_prio_pieces.empty())
	{
		uint32_t piece_index = m_download_queue.low_prio_pieces.front();
		m_piece_info[piece_index].prio_iter = m_tag_list.begin();
		m_download_queue.low_prio_pieces.pop_front();
		return;

	}
}

bool PieceManager::queue_empty()
{
	return m_download_queue.high_prio_pieces.empty() && m_download_queue.normal_prio_pieces.empty() && m_download_queue.low_prio_pieces.empty();
}

int PieceManager::push_piece2download(PIECE_INDEX piece_index)
{
	if (m_piece_info[piece_index].prio_iter != m_tag_list.begin())
		return ERR_ALREADY_EXISTS;

	switch(m_piece_info[piece_index].prio)
	{
	case(DOWNLOAD_PRIORITY_LOW):
			m_download_queue.low_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.low_prio_pieces.end();
			break;
	case(DOWNLOAD_PRIORITY_NORMAL):
			m_download_queue.normal_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.normal_prio_pieces.end();
			break;
	case(DOWNLOAD_PRIORITY_HIGH):
			m_download_queue.high_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.high_prio_pieces.end();
			break;
	default:
			return ERR_INTERNAL;
			break;
	}
	return ERR_NO_ERROR;
}

int PieceManager::set_piece_taken_from(PIECE_INDEX piece_index, const std::string & seed)
{
	m_piece_info[piece_index].taken_from.insert(seed);
	return ERR_NO_ERROR;
}

bool PieceManager::get_piece_taken_from(PIECE_INDEX piece_index, std::string & seed)
{
	if (!m_piece_info[piece_index].taken_from.empty())
	{
		std::set<std::string>::iterator iter = m_piece_info[piece_index].taken_from.begin();
		seed = *iter;
		m_piece_info[piece_index].taken_from.erase(iter);
		return true;
	}
	return false;
}

int PieceManager::clear_piece_taken_from(PIECE_INDEX piece_index)
{
	m_piece_info[piece_index].taken_from.clear();
	return ERR_NO_ERROR;
}

int PieceManager::set_piece_priority(PIECE_INDEX piece_index, DOWNLOAD_PRIORITY priority)
{
	if (m_piece_info[piece_index].prio_iter == m_tag_list.begin())
		return ERR_EMPTY_QUEUE;
	if (m_piece_info[piece_index].prio == priority)
		return ERR_NO_ERROR;

	switch(m_piece_info[piece_index].prio)
	{
	case(DOWNLOAD_PRIORITY_LOW):
			m_download_queue.low_prio_pieces.erase(m_piece_info[piece_index].prio_iter);
			break;
	case(DOWNLOAD_PRIORITY_NORMAL):
			m_download_queue.normal_prio_pieces.erase(m_piece_info[piece_index].prio_iter);
			break;
	case(DOWNLOAD_PRIORITY_HIGH):
			m_download_queue.high_prio_pieces.erase(m_piece_info[piece_index].prio_iter);
			break;
	default:
			return ERR_INTERNAL;
			break;
	}

	switch(priority)
	{
	case(DOWNLOAD_PRIORITY_LOW):
			m_download_queue.low_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.low_prio_pieces.end();
			break;
	case(DOWNLOAD_PRIORITY_NORMAL):
			m_download_queue.normal_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.normal_prio_pieces.end();
			break;
	case(DOWNLOAD_PRIORITY_HIGH):
			m_download_queue.high_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.high_prio_pieces.end();
			break;
	default:
			return ERR_INTERNAL;
			break;
	}

	m_piece_info[piece_index].prio = priority;
	return ERR_NO_ERROR;
}

int PieceManager::get_piece_priority(PIECE_INDEX piece_index, DOWNLOAD_PRIORITY & priority)
{
	priority =  m_piece_info[piece_index].prio;
	return ERR_NO_ERROR;
}

int PieceManager::set_file_priority(FILE_INDEX file, DOWNLOAD_PRIORITY prio)
{
	std::map<PIECE_INDEX, DOWNLOAD_PRIORITY> old_prios;
	std::set<PIECE_INDEX>::iterator begin = m_file_contains_pieces[file].begin();
	std::set<PIECE_INDEX>::iterator end = m_file_contains_pieces[file].end();
	for(std::set<PIECE_INDEX>::iterator iter = begin; iter != end; ++iter)
	{
		PIECE_INDEX piece_index = *iter;

		if (m_piece_info[piece_index].prio >= prio)
		{
			std::set<PIECE_INDEX>::iterator iter2 = iter;
			++iter2;
			if ((iter == begin || iter2 == end))
				continue;
		}
		old_prios[piece_index] = m_piece_info[piece_index].prio;
		int ret = set_piece_priority(piece_index, prio);
		if (ret == ERR_INTERNAL || ret == ERR_BAD_ARG)
		{
			for(std::map<PIECE_INDEX, DOWNLOAD_PRIORITY>::iterator iter2 = old_prios.begin(); iter2 != old_prios.end(); ++iter2)
			{
				set_piece_priority(iter2->first, iter2->second);
			}
			return ERR_INTERNAL;
		}
	}
	return ERR_NO_ERROR;
}

int PieceManager::get_block2download(PIECE_INDEX piece_index, BLOCK_INDEX & block_index)
{
	if (m_piece_info[piece_index].block2download.empty())
		return ERR_EMPTY_QUEUE;
	std::set<uint32_t>::iterator iter = m_piece_info[piece_index].block2download.begin();
	block_index = *iter;
	m_piece_info[piece_index].block2download.erase(iter);
	return ERR_NO_ERROR;
}

size_t PieceManager::get_block2download_count(PIECE_INDEX piece_index)
{
	return m_piece_info[piece_index].block2download.size();
}

size_t PieceManager::get_donwloaded_blocks_count(PIECE_INDEX piece_index)
{
	return m_piece_info[piece_index].downloaded_blocks.size();
}

int PieceManager::push_block2download(PIECE_INDEX piece_index, BLOCK_INDEX block_index)
{
	m_piece_info[piece_index].block2download.insert(block_index);
	return ERR_NO_ERROR;
}

bool PieceManager::check_piece_hash(PIECE_INDEX piece_index)
{
	if (m_torrent->read_piece(piece_index, m_piece_for_check_hash) != ERR_NO_ERROR)
		return false;
	SHA1_HASH sha1;
	memset(sha1, 0, SHA1_LENGTH);
	m_csha1.Update(m_piece_for_check_hash, m_piece_info[piece_index].length);
	m_csha1.Final();
	m_csha1.GetHash(sha1);
	m_csha1.Reset();
	bool ret =  memcmp(sha1, m_piece_info[piece_index].hash, SHA1_LENGTH) == 0;
	if (ret)
	{
		set_bitfield(piece_index, m_piece_info.size(), m_bitfield);
		m_piece_info[piece_index].block2download.clear();
		m_torrent->inc_downloaded(m_piece_info[piece_index].length);
	}
	else
	{
		reset_bitfield(piece_index, m_piece_info.size(), m_bitfield);
		for(uint32_t block = 0; block < m_piece_info[piece_index].block_count; block++)
				m_piece_info[piece_index].block2download.insert(block);
		m_piece_info[piece_index].downloaded_blocks.clear();
		push_piece2download(piece_index);
	}
	return ret;
}

int PieceManager::event_file_write(const fs::write_event & we, PIECE_STATE & piece_state)
{
	uint32_t piece_index;
	uint32_t block_index;
	uint32_t block_length = 0;
	piece_state = PIECE_STATE_NOT_FIN;
	get_piece_block_from_block_id(we.block_id, piece_index, block_index);
	if (we.writted < 0)
	{
		m_downloadable_blocks.erase(we.block_id);
		return push_block2download(piece_index, block_index);
	}

	m_downloadable_blocks[we.block_id] += we.writted;

	get_block_length_by_index(piece_index, block_index, block_length);

	if (m_downloadable_blocks[we.block_id] > block_length)
	{
		m_downloadable_blocks.erase(we.block_id);
		return push_block2download(piece_index, block_index);
	}

	if (m_downloadable_blocks[we.block_id] == block_length)
	{
		m_downloadable_blocks.erase(we.block_id);
		m_piece_info[piece_index].downloaded_blocks.insert(block_index);
		if (m_piece_info[piece_index].downloaded_blocks.size() == m_piece_info[piece_index].block_count)
		{
			bool ret = check_piece_hash(piece_index);
			if (ret)
			{
				piece_state = PIECE_STATE_FIN_HASH_OK;
			}
			else
			{
				piece_state = PIECE_STATE_FIN_HASH_BAD;
			}
#ifdef BITTORRENT_DEBUG
			LOG(INFO) << "PIECE DONE " << piece_index << " ret=" << ret;
#endif
		}
	}
	return ERR_NO_ERROR;
}

}
}
