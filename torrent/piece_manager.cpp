/*
 * piece_manager.cpp
 *
 *  Created on: 12.07.2012
 *      Author: ruslan
 */



namespace torrent
{
PieceManager::PieceManager()
{
	m_pieces = NULL;
	m_bitfield = NULL;
}

int PieceManager::Init(const TorrentInterfaceForPieceManagerPtr & torrent, unsigned char * bitfield)
{
	m_torrent = torrent;
	m_piece_count = m_torrent->get_piece_count();

	m_bitfield_len = ceil(m_piece_count / 8.0f);
	m_bitfield = new(std::nothrow) unsigned char[m_bitfield_len];
	if (m_bitfield == NULL)
	{
		m_torrent->set_error(GENERAL_ERROR_NO_MEMORY_AVAILABLE);
		return ERR_SYSCALL_ERROR;
	}
	if (bitfield == NULL)
		memset(m_bitfield, 0, m_bitfield_len);
	else
		memcpy(m_bitfield, bitfield, m_bitfield_len);

	m_pieces = new(std::nothrow) unsigned char*[piece_count];
	if (m_pieces == NULL)
	{
		m_torrent->set_error(GENERAL_ERROR_NO_MEMORY_AVAILABLE);
		return ERR_SYSCALL_ERROR;
	}
	for(uint32_t i = 0; i < m_piece_count; i++)
	{
		m_pieces[i]=new(std::nothrow) unsigned char[SHA1_LENGTH];
		if (m_pieces[i] == NULL)
		{
			m_torrent->set_error(GENERAL_ERROR_NO_MEMORY_AVAILABLE);
			return ERR_SYSCALL_ERROR;
		}
		m_torrent->copy_piece_hash(m_pieces[i * SHA1_LENGTH], i);
	}
	build_piece_info();
	return ERR_NO_ERROR;
}

PieceManager::~PieceManager()
{
	if (m_pieces != NULL)
	{
		for(uint32_t i = 0;i < m_torrent->get_piece_count(); i++)
		{
			delete[] m_pieces[i];
		}
		delete[] m_pieces;
	}
	if (m_bitfield != NULL)
		delete[] m_bitfield;
}

void PieceManager::reset()
{
	m_pieces_to_download.clear();
	memset(m_bitfield, 0, m_bitfield_len);
	for (uint32_t i = 0; i < piece_count; i++)
	{
		m_piece_info[i].remain = m_piece_info[i].length;
		for(uint32_t block = 0; block < m_piece_info[i].block_count; block++)
			m_piece_info[i].block2download.push_back(block);
	}

}

void PieceManager::build_piece_info()
{
	uint32_t piece_length = m_torrent->get_piece_length();
	uint64_t length = m_torrent->get_length();
	uint32_t file_count = m_torrent->get_files_count();
	m_piece_info.resize(m_piece_count);
	for (uint32_t i = 0; i < m_piece_count; i++)
	{
		uint64_t abs_offset = i * piece_length;//смещение до начала куска
		uint64_t piece_length = (i == m_piece_count - 1) ? length - piece_length * i : piece_length; //размер последнего куска может быть меньше m_piece_length
		int file_index = 0;
		uint64_t last_files_length = 0;
		for(uint32_t j = 0; j < file_count; j++)
		{
			file_info * finfo = m_torrent->get_file_info(j);
			last_files_length += finfo.length;//считает размер файлов, когда он станет больше смещения => кусок начинается в j-ом файле
			if (last_files_length >= abs_offset)
			{
				file_index = j;
				break;
			}
		}
		file_info * finfo = m_torrent->get_file_info(file_index);
		uint64_t file_offset = last_files_length - finfo.length;//смещение до file_index файла
		m_piece_info[i].file_index = file_index;
		m_piece_info[i].length = piece_length;
		m_piece_info[i].remain = piece_length;
		m_piece_info[i].offset = abs_offset - file_offset;//смещение до начала куска внутри файла
		m_piece_info[i].block_count = (uint32_t)ceil((double)piece_length / (double)BLOCK_LENGTH);

		if (!bit_in_bitfield(i, m_piece_count, m_bitfield))
			m_pieces_to_download.insert(i);
		else
			m_torrent->inc_downloaded(piece_length);

		for(uint32_t block = 0; block < m_piece_info[i].block_count; block++)
			m_piece_info[i].block2download.push_back(block);
	}
}

uint32_t PieceManager::get_blocks_count_in_piece(uint32_t piece)
{
	if (m_torrent == NULL)
		return 0;
	if (piece >= m_pieces_count)
		return 0;
	return m_piece_info[piece].block_count;
}

uint32_t PieceManager::get_piece_length(uint32_t piece)
{
	if (m_torrent == NULL)
		return 0;
	if (piece >= m_pieces_count)
		return 0;
	return m_piece_info[piece].length;
}


int PieceManager::get_block_index_by_offset(uint32_t piece_index, uint32_t block_offset, uint32_t & index)
{
	if (m_torrent == NULL)
		return 0;
	if (piece_index >= m_pieces_count)
		return ERR_BAD_ARG;
	uint32_t block_index = block_offset / BLOCK_LENGTH;
	if (block_index >= m_piece_info[piece_index].block_count)
		return ERR_BAD_ARG;
	index = block_index;
	return ERR_NO_ERROR;
}

int PieceManager::get_block_length_by_index(uint32_t piece_index, uint32_t block_index, uint32_t & block_length)
{
	if (m_torrent == NULL)
		return 0;
	if (piece_index >= m_pieces_count)
		return ERR_BAD_ARG;
	if (block_index >= m_piece_info[piece_index].block_count)
		return ERR_BAD_ARG;
	block_length = BLOCK_LENGTH;
	if (piece_index == m_pieces_count - 1 && block_index == m_piece_info[piece_index].block_count - 1)
	{
		block_length = m_piece_info[piece_index].length - BLOCK_LENGTH * block_index;
	}
	return ERR_NO_ERROR;
}

size_t PieceManager::get_bitfield_length()
{
	if (m_torrent == NULL)
		return 0;
	return m_bitfield_len;
}

void PieceManager::copy_bitfield(unsigned char * dst)
{
	memcpy(dst, m_bitfield, m_bitfield_len);
}

void PieceManager::piece_good(uint32_t piece_index)
{
	if (m_torrent == NULL)
		return;
	if (piece >= m_pieces_count)
		return;
	m_pieces_to_download.erase(piece_index);
	set_bitfield(piece_index, m_pieces_count, m_bitfield);
	m_piece_info[piece_index].remain = 0;
}

void PieceManager::piece_bad(uint32_t piece_index)
{
	if (m_torrent == NULL)
		return;
	if (piece >= m_pieces_count)
		return;
	m_pieces_to_download.insert(piece_index);
	reset_bitfield(piece_index, m_pieces_count, m_bitfield);
	m_piece_info[piece_index].remain = m_piece_info[piece_index].length;
}

}
