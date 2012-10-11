/*
 * torrentfile.cpp
 *
 *  Created on: 28.02.2012
 *      Author: ruslan
 */
#include "torrent.h"

namespace dinosaur {
namespace torrent
{

TorrentFile::TorrentFile(const TorrentInterfaceInternalPtr & t)
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "TorrentFile default constructor";
#endif
	m_torrent = t;
	m_fm = t->get_fm();
}

/*
 * Exception::ERR_CODE_UNDEF
 * SyscallException
 */

void TorrentFile::init(const std::string & path, bool files_should_exists, uint32_t & files_exists)
{
	if (path == "" || path[0] != '/')
		throw Exception(Exception::ERR_CODE_UNDEF);

	std::string i_path(path);
	if (i_path[i_path.length() - 1] != '/')
		i_path.append("/");

	uint32_t files_count = m_torrent->get_files_count();

	if (files_count > 1)
	{
		m_torrent->get_dirtree().make_dir_tree(i_path);
		i_path.append(m_torrent->get_name() + "/");
	}
	files_exists = 0;
	for(uint32_t i = 0; i < files_count; i++)
	{
		info::file_stat * fi = m_torrent->get_file_info(i);
		file f;
		f.length = fi->length;
		f.name = i_path + fi->path;
		f.download = true;
		f.priority = DOWNLOAD_PRIORITY_NORMAL;
		f.downloaded = 0;
		try
		{
			if (m_fm->File_exists(f.name, f.length))
				files_exists++;
			m_fm->File_add(f.name, f.length, false, shared_from_this(), f.file_);
		}
		catch(Exception & e)
		{
			ReleaseFiles();
			files_exists = 0;
			throw Exception(Exception::ERR_CODE_UNDEF);
		}
		catch (SyscallException & e)
		{
			ReleaseFiles();
			files_exists = 0;
			throw SyscallException(e);
		}
		catch(...)
		{
			ReleaseFiles();
			files_exists = 0;
			throw Exception(Exception::ERR_CODE_UNDEF);
		}
		m_files.push_back(f);
	}
}

TorrentFile::~TorrentFile()
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "TorrentFile destructor";
#endif
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "TorrentFile destroyed";
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
		uint64_t remain = m_files[file_index].length - offset;
		//если данных для записи больше,чем это возможно, пишем в файл сколько можем(remain), иначе пишем все что есть
		uint64_t to_write = block_length - pos > remain ? remain : block_length - pos;
		try
		{
			m_fm->File_write(m_files[file_index++].file_, &block[pos], to_write, offset, block_id);
		}
		catch (Exception & e)
		{
			if (e.get_errcode() == Exception::ERR_CODE_NULL_REF || e.get_errcode() == Exception::ERR_CODE_FILE_NOT_EXISTS ||
					e.get_errcode() == Exception::ERR_CODE_UNDEF || e.get_errcode() ==  Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW ||
					e.get_errcode() == Exception::ERR_CODE_BLOCK_TOO_BIG)
			{
				torrent_failure tf;
				tf.exception_errcode = e.get_errcode();
				tf.errno_ = 0;
				tf.description = m_files[file_index - 1].name;
				tf.where = TORRENT_FAILURE_WRITE_FILE;
				m_torrent->set_failure(tf);
			}
			return ERR_INTERNAL;
		}
		catch (SyscallException & e)
		{
			torrent_failure tf;
			tf.exception_errcode = Exception::NO_ERROR;
			tf.errno_ = e.get_errno();
			tf.description = m_files[file_index - 1].name;
			tf.where = TORRENT_FAILURE_WRITE_FILE;
			m_torrent->set_failure(tf);
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
	try
	{
		m_torrent->get_bc()->get(key, block);
		return ERR_NO_ERROR;
	}
	catch (Exception & e) {
		if (e.get_errcode() != Exception::ERR_CODE_LRU_CACHE_NE)
		{
			torrent_failure tf;
			tf.exception_errcode = e.get_errcode();
			tf.errno_ = 0;
			tf.description = "";
			tf.where = TORRENT_FAILURE_GET_BLOCK_CACHE;
			m_torrent->set_failure(tf);
			return ERR_INTERNAL;
		}
	}

	FILE_INDEX file_index;
	FILE_OFFSET offset;
	m_torrent->get_block_info(piece, block_index, file_index, offset);

	uint32_t pos = 0;
	while(pos < block_length)
	{
		//определяем сколько возможно прочитать из файла
		uint64_t remain = m_files[file_index].length - offset;
		//если прочитать надо больше, чем это возможно, читаем сколько можем(remain), иначе читаем все
		uint64_t to_read = block_length - pos > remain ? remain : block_length - pos;
		try
		{
			m_fm->File_read_immediately(m_files[file_index++].file_, &block[pos], offset, to_read);
		}
		catch (Exception & e)
		{
			if (e.get_errcode() == Exception::ERR_CODE_NULL_REF || e.get_errcode() == Exception::ERR_CODE_FILE_NOT_EXISTS ||
					e.get_errcode() == Exception::ERR_CODE_UNDEF || e.get_errcode() ==  Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW ||
					e.get_errcode() == Exception::ERR_CODE_FILE_NOT_OPENED)
			{
				torrent_failure tf;
				tf.exception_errcode = e.get_errcode();
				tf.errno_ = 0;
				tf.description = m_files[file_index - 1].name;
				tf.where = TORRENT_FAILURE_WRITE_FILE;
				m_torrent->set_failure(tf);
			}
			return ERR_INTERNAL;
		}
		catch (SyscallException & e)
		{
			torrent_failure tf;
			tf.exception_errcode = Exception::NO_ERROR;
			tf.errno_ = e.get_errno();
			tf.description = m_files[file_index - 1].name;
			tf.where = TORRENT_FAILURE_WRITE_FILE;
			m_torrent->set_failure(tf);
			return ERR_INTERNAL;
		}
		pos += to_read;
		offset = 0;
	}
	try
	{
		m_torrent->get_bc()->put(key, block);
	}
	catch(Exception & e)
	{
		torrent_failure tf;
		tf.exception_errcode = e.get_errcode();
		tf.errno_ = 0;
		tf.description = "";
		tf.where = TORRENT_FAILURE_PUT_BLOCK_CACHE;
		m_torrent->set_failure(tf);
		return ERR_INTERNAL;
	}
	return ERR_NO_ERROR;

}

int TorrentFile::read_piece(PIECE_INDEX piece_index, unsigned char * dst)
{
	//logger::LOGGER() << "read piece %u\n", piece_index);
	FILE_OFFSET offset;
	FILE_INDEX file_index;
	uint32_t piece_length;
	uint32_t pos = 0;

	m_torrent->get_piece_offset(piece_index, offset);
	m_torrent->get_file_index_by_piece(piece_index, file_index);
	m_torrent->get_piece_length(piece_index, piece_length);


	while(pos < piece_length)
	{
		//определяем сколько возможно прочитать из файла
		uint64_t remain = m_files[file_index].length - offset;
		//если прочитать надо больше, чем это возможно, читаем сколько можем(remain), иначе читаем все
		uint64_t to_read = piece_length - pos > remain ? remain : piece_length - pos;
		try
		{
			m_fm->File_read_immediately(m_files[file_index++].file_, (char*)&dst[pos], offset, to_read);
		}
		catch (Exception & e)
		{
			if (e.get_errcode() == Exception::ERR_CODE_NULL_REF || e.get_errcode() == Exception::ERR_CODE_FILE_NOT_EXISTS ||
					e.get_errcode() == Exception::ERR_CODE_UNDEF || e.get_errcode() ==  Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW ||
					e.get_errcode() == Exception::ERR_CODE_FILE_NOT_OPENED)
			{
				torrent_failure tf;
				tf.exception_errcode = e.get_errcode();
				tf.errno_ = 0;
				tf.description = m_files[file_index - 1].name;
				tf.where = TORRENT_FAILURE_WRITE_FILE;
				m_torrent->set_failure(tf);
			}
			return ERR_INTERNAL;
		}
		catch (SyscallException & e)
		{
			torrent_failure tf;
			tf.exception_errcode = Exception::NO_ERROR;
			tf.errno_ = e.get_errno();
			tf.description = m_files[file_index - 1].name;
			tf.where = TORRENT_FAILURE_WRITE_FILE;
			m_torrent->set_failure(tf);
			return ERR_INTERNAL;
		}
		pos += to_read;
		offset = 0;
	}
	return ERR_NO_ERROR;
}

void TorrentFile::event_file_write(const fs::write_event & eo)
{
	m_torrent->event_file_write(eo);
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

void TorrentFile::update_file_downloaded(PIECE_INDEX piece)
{
	FILE_OFFSET offset;
	FILE_INDEX file_index;
	uint32_t piece_length;
	uint32_t pos = 0;

	m_torrent->get_piece_offset(piece, offset);
	m_torrent->get_file_index_by_piece(piece, file_index);
	m_torrent->get_piece_length(piece, piece_length);


	while(pos < piece_length)
	{
		uint64_t remain = m_files[file_index].length - offset;

		uint64_t bytes = piece_length - pos > remain ? remain : piece_length - pos;

		m_files[file_index++].downloaded += bytes;

		pos += bytes;
		offset = 0;
	}
}

void TorrentFile::update_file_downloaded(FILE_INDEX file_index, uint64_t bytes)
{
	m_files[file_index].downloaded += bytes;
}

void TorrentFile::clear_file_downloaded()
{
	for(FILE_INDEX i = 0; i < m_files.size(); i++)
	{
		m_files[i].downloaded = 0;
	}
}

void TorrentFile::get_file_downloaded(FILE_INDEX file_index, uint64_t & bytes_count)
{
	bytes_count = m_files[file_index].downloaded;
}

}
}
