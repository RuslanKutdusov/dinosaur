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
		f.priority = FILE_PRIORITY_NORMAL;
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

int TorrentFile::save_block(PIECE_INDEX piece, BLOCK_OFFSET block_offset, uint32_t block_length, char * block)
{
	if (block == NULL)
		return ERR_BAD_ARG;
	uint32_t block_index;
	m_torrent->get_block_index_by_offset(piece, block_offset, block_index) ;
	uint32_t real_block_length;
	m_torrent->get_block_length_by_index(piece, block_index, real_block_length);
	if (real_block_length != block_length)
		return ERR_INTERNAL;

	FILE_INDEX file_index;
	FILE_OFFSET offset;
	m_torrent->get_block_info(piece, block_index, file_index, offset);

	uint32_t pos = 0;
	BLOCK_ID block_id;
	generate_block_id(piece, block_index, block_id);

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

int TorrentFile::read_block(PIECE_INDEX piece, BLOCK_INDEX block_index, char * block, uint32_t & block_length)
{
	m_torrent->get_block_length_by_index(piece, block_index, block_length);

	BLOCK_ID block_id;
	generate_block_id(piece, block_index, block_id);
	block_cache::cache_key key(m_torrent.get(), block_id);
	if (m_torrent->get_bc()->get(key, (block_cache::cache_element *)block) == ERR_NO_ERROR)
		return ERR_NO_ERROR;

	FILE_INDEX file_index;
	FILE_OFFSET offset;
	m_torrent->get_block_info(piece, block_index, file_index, offset);

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

int TorrentFile::read_piece(PIECE_INDEX piece_index, unsigned char * dst)
{
	//printf("read piece %u\n", piece_index);
	FILE_OFFSET offset;
	FILE_INDEX file_index;
	uint32_t piece_length;
	uint32_t to_read;
	uint32_t pos = 0;

	m_torrent->get_piece_offset(piece_index, offset);
	m_torrent->get_file_index_by_piece(piece_index, file_index);
	m_torrent->get_piece_length(piece_index, piece_length);
	to_read = piece_length;


	while(pos < piece_length)
	{
		//определяем сколько возможно прочитать из файла
		uint32_t remain = m_files[file_index].length - offset;
		//если прочитать надо больше, чем это возможно, читаем сколько можем(remain), иначе читаем все
		uint32_t to_read = piece_length - pos > remain ? remain : piece_length - pos;
		int ret = m_fm->File_read_immediately(m_files[file_index++].file_, (char*)&dst[pos], offset, to_read);
		if (ret < 0)
			return ERR_INTERNAL;
		pos += to_read;
		offset = 0;
	}

	/*while(pos < piece_length)
	{
		int r = m_fm->File_read_immediately(m_files[file_index++].file_, (char*)&dst[pos], offset, to_read);
		if (r < 0)
		{
			return ERR_INTERNAL;
		}
		pos += r;
		to_read -= r;
		offset = 0;
	}*/
	return ERR_NO_ERROR;
}

int TorrentFile::event_file_write(const fs::write_event & eo)
{
	return m_torrent->event_file_write(eo);
}

void TorrentFile::ReleaseFiles()
{
	for(size_t i = 0; i < m_files.size(); i++)
	{
		m_fm->File_delete(m_files[i].file_);
	}
}

void TorrentFile::set_file_priority(FILE_INDEX file, DOWNLOAD_PRIORITY prio)
{
	m_files[file].priority = prio;
}

void TorrentFile::get_file_priority(FILE_INDEX file, DOWNLOAD_PRIORITY & prio)
{
	prio = m_files[file].priority;
}

}
