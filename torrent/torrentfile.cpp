/*
 * torrentfile.cpp
 *
 *  Created on: 28.02.2012
 *      Author: ruslan
 */
#include "torrent.h"

namespace torrent
{

TorrentFile::TorrentFile(const TorrentInterfaceInternalPtr & t)
{
#ifdef BITTORRENT_DEBUG
	printf("TorrentFile default constructor\n");
#endif
	if (t == NULL)
		throw Exception(GENERAL_ERROR_UNDEF_ERROR);

	m_torrent = t;
	m_fm = t->get_fm();
}

void TorrentFile::init(const std::string & path)
{
	if (path == "" || path[0] != '/')
		throw Exception(GENERAL_ERROR_UNDEF_ERROR);

	std::string i_path(path);
	if (i_path[i_path.length() - 1] != '/')
		i_path.append("/");

	uint32_t files_count = m_torrent->get_files_count();

	if (files_count > 1)
	{
		if (m_torrent->get_dirtree() == NULL)
			throw Exception(GENERAL_ERROR_UNDEF_ERROR);
		m_torrent->get_dirtree()->make_dir_tree(i_path);
		i_path.append(m_torrent->get_name() + "/");
	}
	for(uint32_t i = 0; i < files_count; i++)
	{
		base_file_info * fi = m_torrent->get_file_info(i);
		file f;
		f.length = fi->length;
		f.name = i_path + fi->name;
		f.download = true;
		m_fm->File_add(f.name, f.length, false, shared_from_this(), f.file_);
		m_files.push_back(f);
	}
}

TorrentFile::~TorrentFile()
{
#ifdef BITTORRENT_DEBUG
	printf("TorrentFile destructor\n");
#endif
#ifdef BITTORRENT_DEBUG
	printf("TorrentFile destroyed\n");
#endif
}

int TorrentFile::save_block(uint32_t piece, uint32_t block_offset, uint32_t block_length, char * block)
{
	if (block == NULL)
		return ERR_BAD_ARG;
	uint32_t block_index;
	if (m_torrent->get_block_index_by_offset(piece, block_offset, block_index) != ERR_NO_ERROR)
		return ERR_INTERNAL;
	//printf("saving block %u %u\n", piece, block_index);
	uint32_t real_block_length;
	if (m_torrent->get_block_length_by_index(piece, block_index, real_block_length) != ERR_NO_ERROR || real_block_length != block_length)
		return ERR_INTERNAL;

	int file_index;
	if (m_torrent->get_file_index_by_piece(piece, file_index) != ERR_NO_ERROR)
		return ERR_INTERNAL;

	uint64_t offset;
	if (m_torrent->get_piece_offset(piece, offset) != ERR_NO_ERROR)
		return ERR_INTERNAL;
	offset += block_offset;

	while(offset >= m_files[file_index].length)
	{
		offset -= m_files[file_index++].length;
	}
	uint32_t pos = 0;
	BLOCK_ID block_id;
	generate_block_id(piece, block_index, block_id);
	//uint64_t id = ((uint64_t)piece << 32) + block_index;
	while(pos < block_length)
	{
		//определяем сколько возможно записать в файл
		uint32_t remain = m_files[file_index].length - offset;
		//если данных для записи больше,чем это возможно, пишем в файл сколько можем(remain), иначе пишем все что есть
		uint32_t to_write = block_length - pos > remain ? remain : block_length - pos;
		if (m_fm->File_write(m_files[file_index++].file_, &block[pos], to_write, offset, block_id) != ERR_NO_ERROR)
		{
			return ERR_INTERNAL;
		}
		pos += to_write;
		offset = 0;
	}
	return ERR_NO_ERROR;
}

int TorrentFile::read_block(uint32_t piece, uint32_t block_index, char * block, uint32_t & block_length)
{
	if (m_torrent->get_block_length_by_index(piece, block_index, block_length) != ERR_NO_ERROR)
		return ERR_INTERNAL;

	BLOCK_ID block_id;
	generate_block_id(piece, block_index, block_id);
	//printf("look to cache...\n");
	block_cache::cache_key key(m_torrent.get(), block_id);
	//if (m_torrent->m_bc->get(m_torrent, id, block) == ERR_NO_ERROR)
	if (m_torrent->get_bc()->get(key, (block_cache::cache_element *)block) == ERR_NO_ERROR)
		return ERR_NO_ERROR;

	int file_index;
	if (m_torrent->get_file_index_by_piece(piece, file_index) != ERR_NO_ERROR)
		return ERR_INTERNAL;

	uint64_t offset;
	if (m_torrent->get_piece_offset(piece, offset) != ERR_NO_ERROR)
		return ERR_INTERNAL;
	offset += block_index * BLOCK_LENGTH;

	while(offset >= m_files[file_index].length)
	{
		offset -= m_files[file_index++].length;
	}
	uint32_t pos = 0;
	while(pos < block_length)
	{
		//определяем сколько возможно прочитать из файла
		uint32_t remain = m_files[file_index].length - offset;
		//если прочитать надо больше, чем это возможно, читаем сколько можем(remain), иначе читаем все
		uint32_t to_read = block_length - pos > remain ? remain : block_length - pos;
		int ret = m_fm->File_read_immediately(m_files[file_index++].file_, &block[pos], offset, to_read);
		if (ret < 0)
			return ERR_INTERNAL;
		pos += to_read;
		offset = 0;
	}
	//printf("Put in cache\n");
	m_torrent->get_bc()->put(key, (block_cache::cache_element*)block);
	return ERR_NO_ERROR;

}

int TorrentFile::read_piece(uint32_t piece_index, unsigned char * dst)
{
	//printf("read piece %u\n", piece_index);
	uint64_t offset;
	int file_index;
	uint32_t piece_length;
	uint32_t to_read;
	uint32_t pos = 0;

	if (m_torrent->get_piece_offset(piece_index, offset) != ERR_NO_ERROR)
		return ERR_INTERNAL;
	if (m_torrent->get_file_index_by_piece(piece_index, file_index) != ERR_NO_ERROR)
		return ERR_INTERNAL;
	if (m_torrent->get_piece_length(piece_index, piece_length) != ERR_NO_ERROR)
		return ERR_INTERNAL;
	to_read = piece_length;


	while(pos < piece_length)
	{
		int r = m_fm->File_read_immediately(m_files[file_index++].file_, (char*)&dst[pos], offset, to_read);
		if (r < 0)
		{
			return ERR_INTERNAL;
		}
		pos += r;
		to_read -= r;
		offset = 0;
	}
	return ERR_NO_ERROR;
}

int TorrentFile::event_file_write(fs::write_event * eo)
{
	return m_torrent->event_file_write(eo);
	/*if (eo->writted >= 0)
	{
		uint32_t block_index = get_block_from_id(eo->block_id);//(eo->id & (uint32_t)4294967295);
		uint32_t piece = get_piece_from_id(eo->block_id);//(eo->id - block_index)>>32;

		//if (block_done(piece, block_index) == 0)
		//	m_piece_info[piece].remain -= eo->writted;
		if (m_piece_info[piece].remain == 0)
		{
			//printf("piece done %u, checking hash\n", piece);
			check_piece_hash(piece);
		}
		return ERR_NO_ERROR;
	}
	else
	{
		//printf("write error\n");
		uint32_t block_index = get_block_from_id(eo->block_id);//(eo->id & (uint32_t)4294967295);
		uint32_t piece = get_piece_from_id(eo->block_id);//(eo->id - block_index)>>32;

	}*/
}

void TorrentFile::ReleaseFiles()
{
	for(uint32_t i = 0; i < m_files.size(); i++)
	{
		m_fm->File_delete(m_files[i].file_);
	}
}


}
