/*
 * piece_manager.cpp
 *
 *  Created on: 12.07.2012
 *      Author: ruslan
 */

#include "torrent.h"

namespace torrent
{

PieceManager::PieceManager(const TorrentInterfaceInternalPtr & torrent, unsigned char * bitfield)
{
	if (torrent == NULL)
		throw Exception(GENERAL_ERROR_UNDEF_ERROR);
	m_torrent = torrent;
	uint32_t piece_count = m_torrent->get_piece_count();

	m_bitfield_len = ceil(piece_count / 8.0f);
	m_bitfield = new(std::nothrow) unsigned char[m_bitfield_len];
	if (m_bitfield == NULL)
		throw Exception(GENERAL_ERROR_NO_MEMORY_AVAILABLE);

	if (bitfield == NULL)
		memset(m_bitfield, 0, m_bitfield_len);
	else
		memcpy(m_bitfield, bitfield, m_bitfield_len);

	m_piece_for_check_hash = new(std::nothrow) unsigned char[m_torrent->get_piece_length()];
	if (m_piece_for_check_hash == NULL)
	{
		delete[] bitfield;
		throw Exception(GENERAL_ERROR_NO_MEMORY_AVAILABLE);
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
		//m_piece_info[i].prio = PIECE_PRIORITY_NORMAL;

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
	uint32_t piece_length_ = m_torrent->get_piece_length();
	uint64_t length = m_torrent->get_length();
	uint32_t file_count = m_torrent->get_files_count();
	uint32_t piece_count = m_torrent->get_piece_count();
	m_piece_info.resize(piece_count);
	for (uint32_t i = 0; i < piece_count; i++)
	{
		uint64_t abs_offset = i * piece_length_;//смещение до начала куска
		uint32_t piece_length = (i == piece_count - 1) ? length - piece_length_ * i : piece_length_; //размер последнего куска может быть меньше m_piece_length
		int file_index = 0;
		uint64_t last_files_length = 0;
		for(uint32_t j = 0; j < file_count; j++)
		{
			base_file_info * finfo = m_torrent->get_file_info(j);
			last_files_length += finfo->length;//считает размер файлов, когда он станет больше смещения => кусок начинается в j-ом файле
			if (last_files_length >= abs_offset)
			{
				file_index = j;
				break;
			}
		}
		base_file_info * finfo = m_torrent->get_file_info(file_index);
		uint64_t file_offset = last_files_length - finfo->length;//смещение до file_index файла
		m_piece_info[i].file_index = file_index;
		m_piece_info[i].length = piece_length;
		m_piece_info[i].offset = abs_offset - file_offset;//смещение до начала куска внутри файла
		m_piece_info[i].block_count = (uint32_t)ceil((double)piece_length / (double)BLOCK_LENGTH);
		m_torrent->copy_piece_hash(m_piece_info[i].hash, i);
		m_piece_info[i].prio = PIECE_PRIORITY_NORMAL;

		if (!bit_in_bitfield(i, piece_count, m_bitfield))
		{
			m_download_queue.normal_prio_pieces.push_back(i);
			m_piece_info[i].prio_iter = --m_download_queue.normal_prio_pieces.end();
			for(uint32_t block = 0; block < m_piece_info[i].block_count; block++)
				m_piece_info[i].block2download.insert(block);
		}
		else
		{
			m_torrent->inc_downloaded(piece_length);
			m_piece_info[i].prio_iter = m_tag_list.begin();
		}

	}
}

int PieceManager::get_blocks_count_in_piece(uint32_t piece_index, uint32_t & blocks_count)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;

	blocks_count =  m_piece_info[piece_index].block_count;
	return ERR_NO_ERROR;
}

int PieceManager::get_piece_length(uint32_t piece_index, uint32_t & piece_length)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	piece_length = m_piece_info[piece_index].length;
	return ERR_NO_ERROR;
}


int PieceManager::get_block_index_by_offset(uint32_t piece_index, uint32_t block_offset, uint32_t & index)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	uint32_t block_index = block_offset / BLOCK_LENGTH;
	if (block_index >= m_piece_info[piece_index].block_count)
		return ERR_BAD_ARG;
	index = block_index;
	return ERR_NO_ERROR;
}

int PieceManager::get_block_length_by_index(uint32_t piece_index, uint32_t block_index, uint32_t & block_length)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	if (block_index >= m_piece_info[piece_index].block_count)
		return ERR_BAD_ARG;
	block_length = BLOCK_LENGTH;
	if (piece_index == m_piece_info.size() - 1 && block_index == m_piece_info[piece_index].block_count - 1)
	{
		block_length = m_piece_info[piece_index].length - BLOCK_LENGTH * block_index;
	}
	return ERR_NO_ERROR;
}

size_t PieceManager::get_bitfield_length()
{
	return m_bitfield_len;
}

int PieceManager::get_piece_offset(uint32_t piece_index, uint64_t & offset)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	offset = m_piece_info[piece_index].offset;
	return ERR_NO_ERROR;
}

int PieceManager::get_file_index_by_piece(uint32_t piece_index, int & index)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	index = m_piece_info[piece_index].file_index;
	return ERR_NO_ERROR;
}

void PieceManager::copy_bitfield(unsigned char * dst)
{
	memcpy(dst, m_bitfield, m_bitfield_len);
}

int PieceManager::front_piece2download(uint32_t & piece_index)
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

	}
	if (!m_download_queue.normal_prio_pieces.empty())
	{
		uint32_t piece_index = m_download_queue.normal_prio_pieces.front();
		m_piece_info[piece_index].prio_iter = m_tag_list.begin();
		m_download_queue.normal_prio_pieces.pop_front();

	}
	if (!m_download_queue.low_prio_pieces.empty())
	{
		uint32_t piece_index = m_download_queue.low_prio_pieces.front();
		m_piece_info[piece_index].prio_iter = m_tag_list.begin();
		m_download_queue.low_prio_pieces.pop_front();

	}
}

int PieceManager::push_piece2download(uint32_t piece_index)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	if (m_piece_info[piece_index].prio_iter != m_tag_list.begin())
		return ERR_ALREADY_EXISTS;

	switch(m_piece_info[piece_index].prio)
	{
	case(PIECE_PRIORITY_LOW):
			m_download_queue.low_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.low_prio_pieces.end();
			break;
	case(PIECE_PRIORITY_NORMAL):
			m_download_queue.normal_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.normal_prio_pieces.end();
			break;
	case(PIECE_PRIORITY_HIGH):
			m_download_queue.high_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.high_prio_pieces.end();
			break;
	default:
			return ERR_INTERNAL;
			break;
	}
	return ERR_NO_ERROR;
}

int PieceManager::set_piece_taken_from(uint32_t piece_index, const std::string & seed)
{
	m_piece_info[piece_index].taken_from.insert(seed);
	return ERR_NO_ERROR;
}

bool PieceManager::get_piece_taken_from(uint32_t piece_index, std::string & seed)
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

int PieceManager::clear_piece_taken_from(uint32_t piece_index)
{
	m_piece_info[piece_index].taken_from.clear();
	return ERR_NO_ERROR;
}

int PieceManager::set_piece_priority(uint32_t piece_index, PIECE_PRIORITY priority)
{
	if (piece_index >= m_piece_info.size() || (priority != PIECE_PRIORITY_LOW && priority != PIECE_PRIORITY_NORMAL && priority != PIECE_PRIORITY_HIGH))
		return ERR_BAD_ARG;
	if (m_piece_info[piece_index].prio_iter == m_tag_list.begin())
		return ERR_EMPTY_QUEUE;
	if (m_piece_info[piece_index].prio == priority)
		return ERR_NO_ERROR;

	switch(m_piece_info[piece_index].prio)
	{
	case(PIECE_PRIORITY_LOW):
			m_download_queue.low_prio_pieces.erase(m_piece_info[piece_index].prio_iter);
			break;
	case(PIECE_PRIORITY_NORMAL):
			m_download_queue.normal_prio_pieces.erase(m_piece_info[piece_index].prio_iter);
			break;
	case(PIECE_PRIORITY_HIGH):
			m_download_queue.high_prio_pieces.erase(m_piece_info[piece_index].prio_iter);
			break;
	default:
			return ERR_INTERNAL;
			break;
	}

	switch(priority)
	{
	case(PIECE_PRIORITY_LOW):
			m_download_queue.low_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.low_prio_pieces.end();
			break;
	case(PIECE_PRIORITY_NORMAL):
			m_download_queue.normal_prio_pieces.push_back(piece_index);
			m_piece_info[piece_index].prio_iter = --m_download_queue.normal_prio_pieces.end();
			break;
	case(PIECE_PRIORITY_HIGH):
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

int PieceManager::get_piece_priority(uint32_t piece_index, PIECE_PRIORITY & priority)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	priority =  m_piece_info[piece_index].prio;
	return ERR_NO_ERROR;
}

int PieceManager::get_block2download(uint32_t piece_index, uint32_t & block_index)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	if (m_piece_info[piece_index].block2download.empty())
		return ERR_EMPTY_QUEUE;
	std::set<uint32_t>::iterator iter = m_piece_info[piece_index].block2download.begin();
	block_index = *iter;
	m_piece_info[piece_index].block2download.erase(iter);
	return ERR_NO_ERROR;
}

int PieceManager::get_block2download_count(uint32_t piece_index)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	return m_piece_info[piece_index].block2download.size();
}

int PieceManager::push_block2download(uint32_t piece_index, uint32_t block_index)
{
	if (piece_index >= m_piece_info.size())
		return ERR_BAD_ARG;
	if (block_index >= m_piece_info[piece_index].block_count)
		return ERR_BAD_ARG;

	//push_piece2download(piece_index);
	m_piece_info[piece_index].block2download.insert(block_index);


	return ERR_NO_ERROR;
}

bool PieceManager::check_piece_hash(uint32_t piece_index)
{
	m_torrent->read_piece(piece_index, m_piece_for_check_hash);
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
	piece_state = PIECE_STATE_NOT_FIN;
	uint32_t piece_index;
	uint32_t block_index;
	uint32_t block_length;
	piece_state = PIECE_STATE_NOT_FIN;
	get_piece_block_from_block_id(we.block_id, piece_index, block_index);
#ifdef BITTORRENT_DEBUG
	//printf("event_file_write piece=%u block=%u writted=%d\n", piece_index, block_index, we->writted);
#endif
	if (we.writted == -1)
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
			printf("PIECE DONE %u ret=%d\n", piece_index, (int)ret);
#endif
		}
	}
	return ERR_NO_ERROR;
}

}
