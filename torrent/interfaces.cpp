/*
 * interfaces.cpp
 *
 *  Created on: 06.07.2012
 *      Author: ruslan
 */



#include "torrent.h"

namespace torrent
{

network::NetworkManager * TorrentInterfaceBase::get_nm()
{
	return m_nm;
}

cfg::Glob_cfg * TorrentInterfaceBase::get_cfg()
{
	return m_g_cfg;
}

fs::FileManager * TorrentInterfaceBase::get_fm()
{
	return m_fm;
}

uint32_t TorrentInterfaceBase::get_piece_count()
{
	return m_piece_count;
}

block_cache::Block_cache * TorrentInterfaceBase::get_bc()
{
	return m_bc;
}

uint32_t TorrentInterfaceBase::get_piece_length()
{
	return m_piece_length;
}


int TorrentInterfaceBase::get_files_count()
{
	return m_files_count;
}

uint64_t TorrentInterfaceBase::get_length()
{
	return m_length;
}

std::string TorrentInterfaceBase::get_name()
{
	return m_name;
}

uint64_t TorrentInterfaceBase::get_downloaded()
{
	return m_downloaded;
}

uint64_t TorrentInterfaceBase::get_uploaded()
{
	return m_uploaded;
}

void TorrentInterfaceBase::copy_infohash_bin(char * dst)
{
	memcpy(dst, m_info_hash_bin, SHA1_LENGTH);
}

void TorrentInterfaceBase::copy_infohash_bin(unsigned char * dst)
{
	memcpy(dst, m_info_hash_bin, SHA1_LENGTH);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

size_t TorrentInterfaceForPeer::get_bitfield_length()
{
	return m_bitfield_len;
}

void TorrentInterfaceForPeer::get_torrentfile(TorrentFilePtr & ptr)
{
	ptr = m_torrent_file;
}

int TorrentInterfaceForPeer::memcmp_infohash_bin(char * mem)
{
	return memcmp(mem, m_info_hash_bin, SHA1_LENGTH);
}

void TorrentInterfaceForPeer::copy_bitfield(char * dst)
{
	memcpy(dst, m_bitfield, m_bitfield_len);
}

void TorrentInterfaceForPeer::inc_uploaded(uint32_t bytes_num)
{
	m_uploaded += bytes_num;
}


void TorrentInterfaceForTorrentFile::set_error(std::string err)
{
	m_error = err;
}

dir_tree::DirTree * TorrentInterfaceForTorrentFile::get_dirtree()
{
	return m_dir_tree;
}

void TorrentInterfaceForTorrentFile::copy_piece_hash(unsigned char * dst, uint32_t piece_index)
{
	memcpy(dst, &m_pieces[piece_index * SHA1_LENGTH], SHA1_LENGTH);
}

file_info * TorrentInterfaceForTorrentFile::get_file_info(int file_index)
{
	return &m_files[file_index];
}


}
