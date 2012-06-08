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
	m_files = NULL;
	m_length = 0;
	m_files_count = 0;
	m_pieces_count = 0;
	m_piece_length = 0;
	m_pieces = NULL;
	m_piece_for_check_hash = NULL;
	//std::cout<<"TorrentFile created(default)\n";
}

int TorrentFile::Init(Torrent * t, std::string & path,bool _new)
{
	//std::cout<<"TorrentFile created\n";
	if (t == NULL || path == "" || path[0] != '/')
		return ERR_BAD_ARG;
	m_piece_for_check_hash = new(std::nothrow) char[t->m_piece_length];
	if (m_piece_for_check_hash == NULL)
	{
		t->m_error = GENERAL_ERROR_NO_MEMORY_AVAILABLE;
		return ERR_SYSCALL_ERROR;
	}

	m_torrent = t;
	m_fm = t->m_fm;
	m_pieces_count = t->m_piece_count;
	m_piece_length = t->m_piece_length;
	m_files_count  = t->m_files_count;
	m_length = t->m_length;
	m_pieces = new(std::nothrow) unsigned char*[m_pieces_count];
	if (m_pieces == NULL)
	{
		t->m_error = GENERAL_ERROR_NO_MEMORY_AVAILABLE;
		return ERR_SYSCALL_ERROR;
	}
	for(uint32_t i = 0; i < m_pieces_count; i++)
	{
		m_pieces[i]=new(std::nothrow) unsigned char[20];
		if (m_pieces[i] == NULL)
		{
			t->m_error = GENERAL_ERROR_NO_MEMORY_AVAILABLE;
			return ERR_SYSCALL_ERROR;
		}
		memcpy(m_pieces[i], &t->m_pieces[i*20], 20);
	}

	m_files = new file_info[m_files_count];
	std::string i_path(path);
	if (i_path[i_path.length() - 1] != '/')
		i_path.append("/");
	//std::cout<<i_path<<std::endl;

	if (m_torrent->m_multifile && m_torrent->m_dir_tree != NULL)
	{
		m_torrent->m_dir_tree->make_dir_tree(i_path);
		i_path.append(m_torrent->m_name + "/");
	}
	for(uint32_t i = 0; i < m_files_count; i++)
	{
		m_files[i].length = t->m_files[i].length;
		int l = strlen(t->m_files[i].name) + i_path.length();
		m_files[i].name = new(std::nothrow) char[l + 1];// + \0
		if (m_files[i].name  == NULL)
		{
			t->m_error = GENERAL_ERROR_NO_MEMORY_AVAILABLE;
			return ERR_SYSCALL_ERROR;
		}
		memset(m_files[i].name, 0, l + 1);
		strncat(m_files[i].name, i_path.c_str(), i_path.length());
		strncat(m_files[i].name, t->m_files[i].name, strlen(t->m_files[i].name));
		m_files[i].download = t->m_files[i].download;
		//std::cout<<i<<" "<<m_files[i].name<<std::endl;
		m_files[i].fm_id = m_fm->File_add(m_files[i].name, m_files[i].length, false, t);
		//if (m_files[i].fm_id < 0)
		//	printf("%s\n", m_files[i].name);
	}
	build_piece_offset_table();
	return ERR_NO_ERROR;
}

TorrentFile::~TorrentFile()
{
	if (m_pieces != NULL)
	{
		for(uint32_t i=0;i<m_pieces_count;i++)
		{
			delete[] m_pieces[i];
		}
		delete[] m_pieces;
	}
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
}

void TorrentFile::build_piece_offset_table()
{
	m_piece_info.resize(m_pieces_count);
	for (uint32_t i = 0; i < m_pieces_count; i++)
	{
		uint64_t abs_offset = i * m_piece_length;//смещение до начала куска
		uint64_t piece_length = (i == m_pieces_count - 1) ? m_length - m_piece_length * i : m_piece_length; //размер последнего куска может быть меньше m_piece_length
		int file_index = 0;
		uint64_t last_files_length = 0;
		for(uint32_t j = 0; j < m_files_count; j++)
		{
			last_files_length += m_files[j].length;//считает размер файлов, когда он станет больше смещения => кусок начинается в j-ом файле
			if (last_files_length >= abs_offset)
			{
				file_index = j;
				break;
			}
		}
		uint64_t file_offset = last_files_length - m_files[file_index].length;//смещение до file_index файла
		m_piece_info[i].file_index = file_index;
		m_piece_info[i].length = piece_length;
		m_piece_info[i].offset = abs_offset - file_offset;//смещение до начала куска внутри файла
		m_piece_info[i].remain = piece_length;
		m_piece_info[i].block_count = (uint32_t)ceil((double)piece_length / (double)BLOCK_LENGTH);
		m_piece_info[i].blocks2download = std::map<uint32_t, Peer*>();
		m_piece_info[i].marked_blocks = 0;// marked <=> value != NULL
		for(uint32_t j = 0; j < m_piece_info[i].block_count; j++)
		{
			m_piece_info[i].blocks2download[j] = NULL;
		}
	}
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
		unmark_block(piece, block_index);
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
		//bad arg на этом этапе получить не реально, ошибку нехватки места в кэше не прокидываем,
		//т.к блок сможем скачать позже у кого угодно
		if (m_fm->File_write(m_files[file_index++].fm_id, &block[pos], to_write, offset, id) != ERR_NO_ERROR)
		{
			//printf("can not save block %u %u\n", piece, block_index);
			unmark_block(piece, block_index);
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
	block_cache::cache_key key(m_torrent, id);
	//if (m_torrent->m_bc->get(m_torrent, id, block) == ERR_NO_ERROR)
	if (m_torrent->m_bc->get(key, (block_cache::cache_element *)block) == ERR_NO_ERROR)
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
		int ret = m_fm->File_read_immediately(m_files[file_index++].fm_id, &block[pos], offset, to_read);
		if (ret < 0)
			return ERR_INTERNAL;
		pos += to_read;
		offset = 0;
	}
	//printf("Put in cache\n");
	m_torrent->m_bc->put(key, (block_cache::cache_element*)block);
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
		int r = m_fm->File_read_immediately(m_files[file_index++].fm_id, &m_piece_for_check_hash[pos], offset, to_read);
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

bool TorrentFile::piece_is_done(uint32_t piece_index)
{
	if (piece_index >= m_pieces_count)
		return false;
	return m_piece_info[piece_index].remain == 0;
}

uint32_t TorrentFile::get_blocks_count_in_piece(uint32_t piece)
{
	if (piece >= m_pieces_count)
		return 0;
	return m_piece_info[piece].block_count;
}

uint32_t TorrentFile::get_piece_length(uint32_t piece)
{
	if (piece >= m_pieces_count)
		return 0;
	return m_piece_info[piece].length;
}

int TorrentFile::blocks2download(uint32_t piece_index, uint32_t * block, uint32_t array_len)
{
	if (piece_index >= m_pieces_count || block == NULL)
		return -1;
	if (m_piece_info[piece_index].blocks2download.size() == 0)
		return 0;
	if (m_piece_info[piece_index].marked_blocks == m_piece_info[piece_index].blocks2download.size())
		return 0;
	uint32_t i = 0;
	for(std::map<uint32_t, Peer*>::iterator iter = m_piece_info[piece_index].blocks2download.begin();
		iter != m_piece_info[piece_index].blocks2download.end(); ++iter)
	{
		if ((*iter).second == NULL)
		{
			if (i >= array_len)
				return -1;
			//printf("Unmarked piece %u block %d\n", piece_index, (*iter).first);
			block[i++] = (*iter).first;
		}
	}
	return i;
}

int TorrentFile::blocks_in_progress(uint32_t piece_index)
{
	if (piece_index >= m_pieces_count)
		return -1;
	return m_piece_info[piece_index].blocks2download.size();
}

int TorrentFile::mark_block(uint32_t piece_index, uint32_t block_index, Peer * peer)
{
	if (piece_index >= m_pieces_count)
		return -1;
	if (block_index >= m_piece_info[piece_index].block_count)
		return -1;
	std::map<uint32_t, Peer*>::iterator iter = m_piece_info[piece_index].blocks2download.find(block_index);
	if (iter == m_piece_info[piece_index].blocks2download.end() && m_piece_info[piece_index].blocks2download.count(block_index) == 0)
		return -1;
	if ((*iter).second == NULL && peer != NULL)
	{
		//printf("block is marked %u %u\n", piece_index, block_index);
		m_piece_info[piece_index].marked_blocks++;
	}
	if ((*iter).second != NULL && peer == NULL)
	{
		//printf("block is unmarked %u %u\n", piece_index, block_index);
		m_piece_info[piece_index].marked_blocks--;
		(*iter).second->erase_requested_block(generate_block_id(piece_index, block_index));
	}
	(*iter).second = peer;
	return 0;
}

int TorrentFile::unmark_block(uint32_t piece_index, uint32_t block_index)
{
	return mark_block(piece_index, block_index, NULL);
}

Peer * TorrentFile::get_block_mark(uint32_t piece_index, uint32_t block_index)
{
	if (piece_index >= m_pieces_count)
		return NULL;
	if (block_index >= m_piece_info[piece_index].block_count)
		return NULL;
	std::map<uint32_t, Peer*>::iterator iter = m_piece_info[piece_index].blocks2download.find(block_index);
	if (iter == m_piece_info[piece_index].blocks2download.end() && m_piece_info[piece_index].blocks2download.count(block_index) == 0)
		return NULL;
	return (*iter).second;
}

int TorrentFile::unmark_if_match(uint32_t piece_index, uint32_t block_index, Peer * peer)
{
	if (piece_index >= m_pieces_count || peer == NULL)
		return -1;
	if (block_index >= m_piece_info[piece_index].block_count)
		return -1;
	std::map<uint32_t, Peer*>::iterator iter = m_piece_info[piece_index].blocks2download.find(block_index);
	if (iter == m_piece_info[piece_index].blocks2download.end() && m_piece_info[piece_index].blocks2download.count(block_index) == 0)
		return -1;
	if ((*iter).second == peer)
	{
		//printf("block is unmarked %u %u\n", piece_index, block_index);
		m_piece_info[piece_index].marked_blocks--;
		(*iter).second->erase_requested_block(generate_block_id(piece_index, block_index));
		(*iter).second = NULL;
	}
	return 0;
}

int TorrentFile::restore_piece2download(uint32_t piece_index)
{
	//printf("restore piece2download\n");
	if (piece_index >= m_pieces_count)
			return ERR_BAD_ARG;
	for(uint32_t j = 0; j < m_piece_info[piece_index].block_count; j++)
	{
		m_piece_info[piece_index].blocks2download[j] = NULL;
	}
	m_piece_info[piece_index].remain = m_piece_info[piece_index].length;
	m_piece_info[piece_index].marked_blocks = 0;
	return ERR_NO_ERROR;
}

int TorrentFile::block_done(uint32_t piece_index, uint32_t block_index)
{
	if (piece_index >= m_pieces_count)
		return -1;
	if (block_index >= m_piece_info[piece_index].block_count)
		return -1;
	std::map<uint32_t, Peer*>::iterator iter = m_piece_info[piece_index].blocks2download.find(block_index);
	if (iter == m_piece_info[piece_index].blocks2download.end() && m_piece_info[piece_index].blocks2download.count(block_index) == 0)
		return 0;
	if ((*iter).second != NULL)
	{
		//printf("block is done %u %u\n", piece_index, block_index);
		m_piece_info[piece_index].marked_blocks--;
		(*iter).second->erase_requested_block(generate_block_id(piece_index, block_index));
		m_piece_info[piece_index].blocks2download.erase(iter);
	}
	return 0;
}

int TorrentFile::get_block_index_by_offset(uint32_t piece_index, uint32_t block_offset, uint32_t * index)
{
	if (piece_index >= m_pieces_count)
		return ERR_BAD_ARG;
	uint32_t block_index = block_offset / BLOCK_LENGTH;
	if (block_index >= m_piece_info[piece_index].block_count)
		return ERR_BAD_ARG;
	*index = block_index;
	return ERR_NO_ERROR;
}

int TorrentFile::get_block_length_by_index(uint32_t piece_index, uint32_t block_index, uint32_t * block_length)
{
	if (piece_index >= m_pieces_count)
		return ERR_BAD_ARG;
	if (block_index >= m_piece_info[piece_index].block_count)
		return ERR_BAD_ARG;
	*block_length = BLOCK_LENGTH;
	if (piece_index == m_pieces_count - 1 && block_index == m_piece_info[piece_index].block_count - 1)
	{
		*block_length = m_piece_info[piece_index].length - BLOCK_LENGTH * block_index;
	}
	return ERR_NO_ERROR;
}

int TorrentFile::event_file_write(fs::write_event * eo)
{
	if (eo->writted >= 0)
	{
		uint32_t block_index = get_block_from_id(eo->block_id);//(eo->id & (uint32_t)4294967295);
		uint32_t piece = get_piece_from_id(eo->block_id);//(eo->id - block_index)>>32;
		//printf("WRITE %u %u\n", piece, block_index);
		if (block_done(piece, block_index) == 0)
			m_piece_info[piece].remain -= eo->writted;
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
		return unmark_block(piece, block_index);
	}
}


}
