/*
 * torrentfile.cpp
 *
 *  Created on: 28.02.2012
 *      Author: ruslan
 */
#include "torrent.h"

namespace torrent
{

TorrentFile::TorrentFile()
{
#ifdef BITTORRENT_DEBUG
	printf("TorrentFile default constructor\n");
#endif
	m_files = NULL;
	m_length = 0;
	m_files_count = 0;
	m_pieces_count = 0;
	m_piece_length = 0;
	m_pieces = NULL;
	m_piece_for_check_hash = NULL;
	//std::cout<<"TorrentFile created(default)\n";
}

int TorrentFile::Init(const TorrentInterfaceForTorrentFilePtr & t, std::string & path,bool _new)
{
	//std::cout<<"TorrentFile created\n";
	if (t == NULL || path == "" || path[0] != '/')
		return ERR_BAD_ARG;
	m_piece_for_check_hash = new(std::nothrow) char[t->get_piece_length()];
	if (m_piece_for_check_hash == NULL)
	{
		t->set_error(GENERAL_ERROR_NO_MEMORY_AVAILABLE);
		return ERR_SYSCALL_ERROR;
	}

	m_torrent = t;
	m_fm = t->get_fm();
	m_files_count  = t->get_files_count();
	m_length = t->get_length();

	m_files = new file_info[m_files_count];
	if (m_files == NULL)
	{
		t->set_error(GENERAL_ERROR_NO_MEMORY_AVAILABLE);
		return ERR_SYSCALL_ERROR;
	}
	std::string i_path(path);
	if (i_path[i_path.length() - 1] != '/')
		i_path.append("/");
	//std::cout<<i_path<<std::endl;

	if (m_files_count > 1)
	{
		if (m_torrent->get_dirtree() == NULL)
		{
			t->set_error(GENERAL_ERROR_UNDEF_ERROR);
			return ERR_SYSCALL_ERROR;
		}
		m_torrent->get_dirtree()->make_dir_tree(i_path);
		i_path.append(m_torrent->get_name() + "/");
	}
	for(uint32_t i = 0; i < m_files_count; i++)
	{
		file_info * fi = t->get_file_info(i);
		m_files[i].length = fi->length;
		int l = strlen(fi->name) + i_path.length();
		m_files[i].name = new(std::nothrow) char[l + 1];// + \0
		if (m_files[i].name  == NULL)
		{
			t->set_error(GENERAL_ERROR_NO_MEMORY_AVAILABLE);
			return ERR_SYSCALL_ERROR;
		}
		memset(m_files[i].name, 0, l + 1);
		strncat(m_files[i].name, i_path.c_str(), i_path.length());
		strncat(m_files[i].name, fi->name, strlen(fi->name));
		m_files[i].download = fi->download;
		m_files[i].file = fs::File();
		m_fm->File_add(m_files[i].name, m_files[i].length, false, shared_from_this(), m_files[i].file);
	}
	build_piece_offset_table();
	return ERR_NO_ERROR;
}

TorrentFile::~TorrentFile()
{
#ifdef BITTORRENT_DEBUG
	printf("TorrentFile destructor\n");
#endif
	if (m_files != NULL)
	{
		for (uint32_t i = 0; i < m_files_count; i++)
		{
			if (m_files[i].name != NULL)
				delete[] m_files[i].name;
		}
		delete[] m_files;
	}
	if (m_piece_for_check_hash != NULL)
		delete[] m_piece_for_check_hash;
#ifdef BITTORRENT_DEBUG
	printf("TorrentFile destroyed\n");
#endif
}

int TorrentFile::save_block(uint32_t piece, uint32_t block_offset, uint32_t block_length, char * block)
{
	if (piece >= m_pieces_count || block == NULL)
		return ERR_BAD_ARG;
	uint32_t block_index = block_offset / BLOCK_LENGTH;
	if (block_index >= m_piece_info[piece].block_count)
		return ERR_INTERNAL;
	//printf("saving block %u %u\n", piece, block_index);
	uint32_t real_block_length;
	if (get_block_length_by_index(piece, block_index, &real_block_length) != ERR_NO_ERROR || real_block_length != block_length)
	{
		return ERR_INTERNAL;
	}

	int file_index = m_piece_info[piece].file_index;
	uint64_t offset = block_offset + m_piece_info[piece].offset;
	while(offset >= m_files[file_index].length)
	{
		offset -= m_files[file_index++].length;
	}
	uint32_t pos = 0;
	uint64_t id = generate_block_id(piece, block_index);
	//uint64_t id = ((uint64_t)piece << 32) + block_index;
	while(pos < block_length)
	{
		//определяем сколько возможно записать в файл
		uint32_t remain = m_files[file_index].length - offset;
		//если данных для записи больше,чем это возможно, пишем в файл сколько можем(remain), иначе пишем все что есть
		uint32_t to_write = block_length - pos > remain ? remain : block_length - pos;
		if (m_fm->File_write(m_files[file_index++].file, &block[pos], to_write, offset, id) != ERR_NO_ERROR)
		{
			return ERR_INTERNAL;
		}
		pos += to_write;
		offset = 0;
	}
	return ERR_NO_ERROR;
}

int TorrentFile::read_block(uint32_t piece, uint32_t block_index, char * block, uint32_t * block_length)
{
	if (piece >= m_pieces_count)
		return ERR_BAD_ARG;
	if (block_index >= m_piece_info[piece].block_count)
		return ERR_INTERNAL;
	get_block_length_by_index(piece, block_index, block_length);

	uint64_t id = generate_block_id(piece, block_index);
	//printf("look to cache...\n");
	block_cache::cache_key key(m_torrent.get(), id);
	//if (m_torrent->m_bc->get(m_torrent, id, block) == ERR_NO_ERROR)
	if (m_torrent->get_bc()->get(key, (block_cache::cache_element *)block) == ERR_NO_ERROR)
		return ERR_NO_ERROR;

	//printf("not in cache, read from file\n");
	int file_index = m_piece_info[piece].file_index;
	uint32_t offset = block_index * BLOCK_LENGTH + m_piece_info[piece].offset;
	while(offset >= m_files[file_index].length)
	{
		offset -= m_files[file_index++].length;
	}
	uint32_t pos = 0;
	while(pos < *block_length)
	{
		//определяем сколько возможно прочитать из файла
		uint32_t remain = m_files[file_index].length - offset;
		//если прочитать надо больше, чем это возможно, читаем сколько можем(remain), иначе читаем все
		uint32_t to_read = *block_length - pos > remain ? remain : *block_length - pos;
		int ret = m_fm->File_read_immediately(m_files[file_index++].file, &block[pos], offset, to_read);
		if (ret < 0)
			return ERR_INTERNAL;
		pos += to_read;
		offset = 0;
	}
	//printf("Put in cache\n");
	m_torrent->get_bc()->put(key, (block_cache::cache_element*)block);
	return ERR_NO_ERROR;

}


bool TorrentFile::check_piece_hash(uint32_t piece_index)
{
	if (read_piece(piece_index) != ERR_NO_ERROR)
	{
		m_torrent->event_piece_hash(piece_index, false, true);
		return ERR_INTERNAL;
	}
	unsigned char sha1[20];
	memset(sha1, 0, 20);
	m_csha1.Update((unsigned char*)m_piece_for_check_hash, m_piece_info[piece_index].length);
	m_csha1.Final();
	m_csha1.GetHash(sha1);
	m_csha1.Reset();
	bool ret = memcmp(sha1,m_pieces[piece_index], 20) == 0;
	m_torrent->event_piece_hash(piece_index, ret, false);
	return true;
}

int TorrentFile::read_piece(uint32_t piece_index)
{
	//printf("read piece %u\n", piece_index);
	if (piece_index >= (uint32_t)m_pieces_count)
		return ERR_BAD_ARG;
	uint64_t offset = m_piece_info[piece_index].offset;
	int file_index =  m_piece_info[piece_index].file_index;
	uint32_t piece_length = m_piece_info[piece_index].length;
	uint32_t to_read = piece_length;
	uint32_t pos = 0;


	while(pos < piece_length)
	{
		int r = m_fm->File_read_immediately(m_files[file_index++].file, &m_piece_for_check_hash[pos], offset, to_read);
		if (r == -1)
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
	return ERR_NO_ERROR;
}

void TorrentFile::ReleaseFiles()
{
	if (m_files == NULL)
		return;
	for(uint32_t i = 0; i < m_files_count; i++)
	{
		m_fm->File_delete(m_files[i].file);
	}
}


}
