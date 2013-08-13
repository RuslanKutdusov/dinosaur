/*
 * file.cpp
 *
 *  Created on: 07.03.2012
 *      Author: ruslan
 */

#include "fs.h"

namespace dinosaur {
namespace fs
{

file::file()
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "file default constructor";
#endif
	m_length = 0;
	m_should_exists = false;
	m_fd = -1;
	m_instance2delete = false;
}

/*
 * Exception::ERR_CODE_NULL_REF;
 */

file::file(const char * fn, uint64_t length, bool should_exists, const FileEventInterface::ptr & assoc) throw (Exception)
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "file constructor fn=" << fn
			  << " length=" << length;
#endif
	if (fn == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	m_fn = fn;
	m_length = length;
	m_should_exists = should_exists;
	m_assoc = assoc;
	m_fd = -1;
	m_instance2delete = false;
}

file::~file()
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "file destructor fn=" << m_fn.c_str()
			  << " " << this;
#endif
	_close();
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "file destructed";
#endif
}

/*
 * Exception::ERR_CODE_FILE_NOT_EXISTS
 * SyscallException
 */

void file::_open() throw(Exception, SyscallException)
{
#ifdef FS_DEBUG
	logger::LOGGER() << "Opening file " << m_fn.c_str();
#endif
	bool not_exists = false;
	struct stat st;
	//если файла нет, говорим что новый
	if (stat(m_fn.c_str(), &st) == -1 || (uint64_t)st.st_size != m_length)
		not_exists = true;
	if (not_exists && m_should_exists)
	{
			#ifdef FS_DEBUG
				logger::LOGGER() << "Fail: file " << m_fn.c_str() <<" does not exists";
			#endif
		throw Exception(Exception::ERR_CODE_FILE_NOT_EXISTS);
	}
	if (!not_exists)
	{
		m_fd = open(m_fn.c_str(), O_RDWR | O_LARGEFILE);
		if (m_fd == -1)
		{
			#ifdef FS_DEBUG
				logger::LOGGER() << "Fail: " << sys_errlist[errno];
			#endif
			throw SyscallException();
		}
	}
	//создаем/открываем файл
	//если файл новый, то транкаем его
	if (not_exists)
	{
		m_fd = open(m_fn.c_str(), O_RDWR | O_CREAT | O_LARGEFILE, S_IRWXU | S_IRWXG | S_IRWXO);
		if (m_fd == -1)
		{
			#ifdef FS_DEBUG
				logger::LOGGER() << "Fail: " << sys_errlist[errno];
			#endif
			throw SyscallException();
		}
		//logger::LOGGER() << "truncating";
		if (ftruncate64(m_fd, m_length) == -1)
		{
			#ifdef FS_DEBUG
				logger::LOGGER() << "Fail: " << sys_errlist[errno];
			#endif
			throw SyscallException();
		}
	}
	#ifdef FS_DEBUG
		logger::LOGGER() << "OK";
	#endif
}

/*
 * Exception::ERR_CODE_FILE_NOT_OPENED
 * SyscallException
 */

size_t file::_write(const char * buf, uint64_t offset, uint64_t length) throw(Exception, SyscallException)
{
	#ifdef FS_DEBUG
		logger::LOGGER() << "Writing to file " <<  m_fn.c_str()
				  << " offset=" << offset
				  << " length=" << length;
	#endif
	int ret;
	if (!is_opened())
	{
		#ifdef FS_DEBUG
			logger::LOGGER() << "Fail: File is not opened";
		#endif
		throw Exception(Exception::ERR_CODE_FILE_NOT_OPENED);
	}
	ret = lseek ( m_fd , offset , SEEK_SET );
	if (ret == -1)
	{
		#ifdef FS_DEBUG
		logger::LOGGER() << "Fail: " << sys_errlist[errno];
		#endif
		throw SyscallException();
	}
	ret = write(m_fd, buf, length);
	if (ret == -1)
	{
		#ifdef FS_DEBUG
		logger::LOGGER() << "Fail: " << sys_errlist[errno];
		#endif
		throw SyscallException();
	}
	#ifdef FS_DEBUG
		logger::LOGGER() << "OK";
	#endif
	return ret;
}

/*
 * Exception::ERR_CODE_FILE_NOT_OPENED
 * SyscallException
 */

size_t file::_read(char * buf, uint64_t offset, uint64_t length) throw(Exception, SyscallException)
{
	int ret;
	if (!is_opened())
		throw Exception(Exception::ERR_CODE_FILE_NOT_OPENED);
	ret = lseek(m_fd, offset, SEEK_SET);
	if (ret == -1)
	{
		throw SyscallException();
	}
	ssize_t r = read(m_fd, buf, length);
	if (r == -1)
		throw SyscallException();
	//_close();
	return r;
}

void file::_close()
{
	close(m_fd);
	logger::LOGGER() << "file closed "<< m_fn.c_str();
	m_fd = -1;
}

bool file::is_opened()
{
	bool b = m_fd != -1;
	return b;
}

void file::get_name(std::string & name)
{
	name = m_fn;
}

const std::string & file::get_name()
{
	return m_fn;
}

uint64_t file::get_length()
{
	return m_length;
}

}
}
