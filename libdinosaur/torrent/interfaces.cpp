/*
 * interfaces.cpp
 *
 *  Created on: 06.07.2012
 *      Author: ruslan
 */



#include "torrent.h"

namespace dinosaur {
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


size_t TorrentInterfaceInternal::get_files_count()
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

info::file_stat * TorrentInterfaceInternal::get_file_info(FILE_INDEX file_index)
{
	return file_index >= (FILE_INDEX)m_metafile.files.size() ? NULL : &m_metafile.files[file_index];
}

dir_tree::DirTree & TorrentInterfaceInternal::get_dirtree()
{
	return m_metafile.dir_tree;
}

void TorrentInterfaceInternal::get_blocks_count_in_piece(uint32_t piece, uint32_t & blocks_count)
{
	m_piece_manager->get_blocks_count_in_piece(piece, blocks_count);
}

void TorrentInterfaceInternal::get_piece_length(uint32_t piece, uint32_t & piece_length)
{
	m_piece_manager->get_piece_length(piece, piece_length);
}

void TorrentInterfaceInternal::get_block_index_by_offset(uint32_t piece_index, uint32_t block_offset, uint32_t & index)
{
	m_piece_manager->get_block_index_by_offset(piece_index, block_offset, index);
}

void TorrentInterfaceInternal::get_block_length_by_index(uint32_t piece_index, uint32_t block_index, uint32_t & block_length)
{
	m_piece_manager->get_block_length_by_index(piece_index, block_index, block_length);
}

void TorrentInterfaceInternal::get_piece_offset(uint32_t piece_index, uint64_t & offset)
{
	m_piece_manager->get_piece_offset(piece_index, offset);
}

void TorrentInterfaceInternal::get_block_info(PIECE_INDEX piece_index, BLOCK_INDEX block_index, FILE_INDEX & file_index, FILE_OFFSET & file_offset)
{
	 m_piece_manager->get_block_info(piece_index, block_index, file_index, file_offset);
}

void TorrentInterfaceInternal::get_file_index_by_piece(uint32_t piece_index, FILE_INDEX & index)
{
	m_piece_manager->get_file_index_by_piece(piece_index, index);
}

SHA1_HASH & TorrentInterfaceInternal::get_infohash()
{
	return m_metafile.info_hash_bin;
}

bool TorrentInterfaceInternal::memcmp_infohash_bin(const SHA1_HASH & hash)
{
	return hash == m_metafile.info_hash_bin;
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

SHA1_HASH & TorrentInterfaceInternal::get_piece_hash(uint32_t piece_index)
{
	return m_metafile.pieces[piece_index];
}

int TorrentInterfaceInternal::save_block(uint32_t piece, uint32_t block_offset, uint32_t block_length, char * block)
{
	return m_torrent_file->save_block(piece, block_offset, block_length, block);
}

int TorrentInterfaceInternal::read_block(uint32_t piece, uint32_t block_index, char * block, uint32_t & block_length)
{
	return m_torrent_file->read_block(piece, block_index, block, block_length);
}

int TorrentInterfaceInternal::read_piece(uint32_t piece, unsigned char * dst)
{
	return m_torrent_file->read_piece(piece, dst);
}

void TorrentInterfaceInternal::update_file_downloaded(FILE_INDEX file_index, uint64_t bytes)
{
	m_torrent_file->update_file_downloaded(file_index, bytes);
}

}
}
