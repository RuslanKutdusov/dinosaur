/*
 * interfaces.cpp
 *
 *  Created on: 06.07.2012
 *      Author: ruslan
 */



#include "torrent.h"

namespace torrent
{

network::NetworkManager * TorrentInterfaceInternal::get_nm()
{
	return m_nm;
}

cfg::Glob_cfg * TorrentInterfaceInternal::get_cfg()
{
	return m_g_cfg;
}

fs::FileManager * TorrentInterfaceInternal::get_fm()
{
	return m_fm;
}

uint32_t TorrentInterfaceInternal::get_piece_count()
{
	return m_metafile.piece_count;
}

block_cache::Block_cache * TorrentInterfaceInternal::get_bc()
{
	return m_bc;
}

uint32_t TorrentInterfaceInternal::get_piece_length()
{
	return m_metafile.piece_length;
}


int TorrentInterfaceInternal::get_files_count()
{
	return m_metafile.files.size();
}

uint64_t TorrentInterfaceInternal::get_length()
{
	return m_metafile.length;
}

std::string TorrentInterfaceInternal::get_name()
{
	return m_metafile.name;
}

uint64_t TorrentInterfaceInternal::get_downloaded()
{
	return m_downloaded;
}

uint64_t TorrentInterfaceInternal::get_uploaded()
{
	return m_uploaded;
}

size_t TorrentInterfaceInternal::get_bitfield_length()
{
	return m_piece_manager->get_bitfield_length();
}

base_file_info * TorrentInterfaceInternal::get_file_info(uint32_t file_index)
{
	return file_index >= m_metafile.files.size() ? NULL : &m_metafile.files[file_index];
}

dir_tree::DirTree * TorrentInterfaceInternal::get_dirtree()
{
	return &m_metafile.dir_tree;
}

int TorrentInterfaceInternal::get_blocks_count_in_piece(uint32_t piece, uint32_t & blocks_count)
{
	return m_piece_manager->get_blocks_count_in_piece(piece, blocks_count);
}

int TorrentInterfaceInternal::get_piece_length(uint32_t piece, uint32_t & piece_length)
{
	return m_piece_manager->get_piece_length(piece, piece_length);
}

int TorrentInterfaceInternal::get_block_index_by_offset(uint32_t piece_index, uint32_t block_offset, uint32_t & index)
{
	return m_piece_manager->get_block_index_by_offset(piece_index, block_offset, index);
}

int TorrentInterfaceInternal::get_block_length_by_index(uint32_t piece_index, uint32_t block_index, uint32_t & block_length)
{
	return m_piece_manager->get_block_length_by_index(piece_index, block_index, block_length);
}

int TorrentInterfaceInternal::get_piece_offset(uint32_t piece_index, uint64_t & offset)
{
	return m_piece_manager->get_piece_offset(piece_index, offset);
}

int TorrentInterfaceInternal::get_file_index_by_piece(uint32_t piece_index, int & index)
{
	return m_piece_manager->get_file_index_by_piece(piece_index, index);
}

void TorrentInterfaceInternal::copy_infohash_bin(SHA1_HASH dst)
{
	memcpy(dst, m_metafile.info_hash_bin, SHA1_LENGTH);
}

int TorrentInterfaceInternal::memcmp_infohash_bin(SHA1_HASH mem)
{
	return memcmp(mem, m_metafile.info_hash_bin, SHA1_LENGTH);
}

void TorrentInterfaceInternal::copy_bitfield(BITFIELD dst)
{
	m_piece_manager->copy_bitfield((unsigned char*)dst);
}

void TorrentInterfaceInternal::inc_uploaded(uint32_t bytes_num)
{
	m_uploaded += bytes_num;
}

void TorrentInterfaceInternal::inc_downloaded(uint32_t bytes_num)
{
	m_downloaded += bytes_num;
}

void TorrentInterfaceInternal::set_error(std::string err)
{
	m_error = err;
}

void TorrentInterfaceInternal::copy_piece_hash(SHA1_HASH dst, uint32_t piece_index)
{
	memcpy(dst, &m_metafile.pieces[piece_index * SHA1_LENGTH], SHA1_LENGTH);
}

int TorrentInterfaceInternal::save_block(uint32_t piece, uint32_t block_offset, uint32_t block_length, char * block)
{
	return m_torrent_file->save_block(piece, block_offset, block_length, block);
}

int TorrentInterfaceInternal::read_block(uint32_t piece, uint32_t block_index, char * block, uint32_t & block_length)
{
	return m_torrent_file->read_block(piece, block_index, block, block_length);
}

}
