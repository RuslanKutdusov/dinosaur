/*
 * FileManager.cpp
 *
 *  Created on: 03.03.2012
 *      Author: ruslan
 */

#include "fs.h"
namespace dinosaur {
namespace fs {

FileManager::FileManager()
{
#ifdef BITTORRENT_DEBUG
	printf("FileManager default constructor\n");
#endif
	m_write_thread = 0;
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_NO_MEMORY_AVAILABLE
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_CAN_NOT_CREATE_THREAD
 */

void FileManager::Init(cfg::Glob_cfg * cfg) throw (Exception)
{
	// TODO Auto-generated constructor stub
	if (cfg == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	m_count = 0;
	m_cfg = cfg;
	m_cache_size = m_cfg->get_write_cache_size();
	try
	{
		m_write_cache.init_cache(m_cache_size);
		m_fd_cache.Init(MAX_OPENED_FILES);
	}
	catch (Exception & e) {
		throw Exception(e);
	}

	m_thread_stop = false;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutexattr_t   mta;
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, &mta);
	pthread_cond_init (&m_cond, NULL);
	//pthread_mutex_init(&m_mutex_timeout_sockets, NULL);
	if (pthread_create(&m_write_thread, &attr, FileManager::cache_thread, (void *)this))
		throw Exception(Exception::ERR_CODE_CAN_NOT_CREATE_THREAD);
	pthread_attr_destroy(&attr);
}

FileManager::~FileManager() {
#ifdef BITTORRENT_DEBUG
	printf("FileManager destructor\n");
#endif
	if (m_write_thread != 0)
	{
		m_thread_stop = true;
		void * status;
		pthread_join(m_write_thread, &status);
		pthread_mutex_destroy(&m_mutex);
	}
	for(std::set<File>::iterator iter = m_files.begin(); iter != m_files.end(); ++iter)
	{
		(*iter)->m_assoc.reset();
	}
#ifdef BITTORRENT_DEBUG
	printf("FileManager destroyed\n");
#endif
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

int FileManager::File_add(const char * fn, uint64_t length, bool should_exists, const FileAssociation::ptr & assoc, File & file_) throw (Exception)
{
	if (fn == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	file_.reset(new file(fn, length, should_exists, assoc));
	pthread_mutex_lock(&m_mutex);
	m_files.insert(file_);
	pthread_mutex_unlock(&m_mutex);
	//printf("File added id=%d\n", f->m_id);
	return ERR_NO_ERROR;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

int FileManager::File_add(const std::string & fn, uint64_t length, bool should_exists, const FileAssociation::ptr & assoc, File & file) throw (Exception)
{
	return File_add(fn.c_str(), length, should_exists, assoc, file);
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_FILE_NOT_EXISTS
 * Exception::ERR_CODE_UNDEF
 * SyscallException
 */

void FileManager::prepare_file(File & file) throw (Exception, SyscallException)
{
	if (file == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	int fd;
	try
	{
		m_fd_cache.get(file, &fd);
	}
	catch (Exception & e)
	{
		if (e.get_errcode() == Exception::ERR_CODE_LRU_CACHE_NE)
		{
			file->_open();
			File deleted_file;
			int file_desc = file->m_fd;
			m_fd_cache.put(file, file_desc, deleted_file);
			if(deleted_file != NULL)
				deleted_file->_close();
		}
		else
			throw Exception(e);
	}
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_FILE_NOT_EXISTS
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW
 * Exception::ERR_CODE_CACHE_FULL
 * Exception::ERR_CODE_BLOCK_TOO_BIG
 * SyscallException
 */

void FileManager::File_write(File & file, const char * buf, uint32_t length, uint64_t offset, const BLOCK_ID & block_id) throw (Exception, SyscallException)
{
	#ifdef FS_DEBUG
		printf("Write to file %s offset=%llu length=%u\n", file->m_fn.c_str(),  offset, length);
	#endif
	pthread_mutex_lock(&m_mutex);
	try
	{
		if (buf == NULL || file == NULL)
		{
			#ifdef FS_DEBUG
				printf("Fail: buf = NULL or file = NULL\n");
			#endif
			throw Exception(Exception::ERR_CODE_NULL_REF);
		}
		if (length == 0 && offset > 0)
		{
			#ifdef FS_DEBUG
				printf("Fail: length = 0 and offset > 0\n");
			#endif
			throw Exception(Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW);
		}
		if (length == 0)
		{
			prepare_file(file);
			write_event we;
			we.block_id = block_id;
			we.file = file;
			we.writted = 0;
			m_write_event.push_back(we);
			we.file.reset();
			#ifdef FS_DEBUG
				printf("OK\n");
			#endif
		}
		if (offset >= file->m_length)
		{
			#ifdef FS_DEBUG
				printf("Fail: offset overflow\n");
			#endif
			throw Exception(Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW);
		}

		m_write_cache.push(file, buf, length, offset, block_id);
		#ifdef FS_DEBUG
				printf("OK\n");
		#endif
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
	catch (SyscallException & e)
	{
		pthread_mutex_unlock(&m_mutex);
		SyscallException(e);
	}
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_FILE_NOT_EXISTS
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW
 * Exception::ERR_CODE_FILE_NOT_OPENED
 * SyscallException
 */

int FileManager::File_read_immediately(File & file, char * buf, uint64_t offset, uint64_t length) throw (Exception, SyscallException)
{
	pthread_mutex_lock(&m_mutex);
	try
	{
		if (buf == NULL || file == NULL)
		{
			throw Exception(Exception::ERR_CODE_NULL_REF);
		}
		if (length == 0 && offset > 0)
		{
			throw Exception(Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW);
		}
		if (length == 0)
		{
			pthread_mutex_unlock(&m_mutex);
			return 0;
		}
		if (offset >= file->m_length)
		{
			throw Exception(Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW);
		}
		prepare_file(file);
		size_t ret = file->_read(buf, offset, length);
		pthread_mutex_unlock(&m_mutex);
		return ret;
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
	catch (SyscallException & e)
	{
		pthread_mutex_unlock(&m_mutex);
		SyscallException(e);
	}
	return 0;
}

/* Ошибки в оповещении
 * ERR_UNDEF - критическая
 * ERR_SYSCALL_ERROR - критическая
 * ERR_FILE_NOT_EXISTS критическая
 * ERR_ON_ERROR - нет ошибки
 * описание ошибки +
 */

void FileManager::notify()
{
	pthread_mutex_lock(&m_mutex);
	while(!m_write_event.empty())
	{
		write_event & we =  m_write_event.front();
		if (!we.file->m_instance2delete)
			we.file->m_assoc->event_file_write(we);
		m_write_event.pop_front();
	}
	pthread_mutex_unlock(&m_mutex);
}

void FileManager::File_delete(File & file)
{
	pthread_mutex_lock(&m_mutex);
	m_files.erase(file);
	m_fd_cache.remove(file);
	if (file != NULL)
		file->m_instance2delete = true;
	file.reset();
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_NULL_REF
 * SyscallException
 */

bool FileManager::File_exists(const char *fn) const
{
	if (fn == NULL)
		throw (Exception::ERR_CODE_NULL_REF);
	int fd;
	fd = open(fn, O_LARGEFILE);
	if (fd != -1 && (errno == EISDIR || errno == ENOENT))
	{
		close(fd);
		return true;
	}
	else
	{
		int err = errno;
		close(fd);
		throw SyscallException(err);
	}
	close(fd);
	return false;
}

/*
 * Exception::ERR_CODE_NULL_REF
 * SyscallException
 */

bool FileManager::File_exists(const std::string & fn) const
{
	return File_exists(fn.c_str());
}

/*
 * Exception::ERR_CODE_NULL_REF
 * SyscallException
 */

bool FileManager::File_exists(const char *fn, uint64_t length) const
{
	if (fn == NULL)
		throw (Exception::ERR_CODE_NULL_REF);
	struct stat st;
	if (stat(fn, &st) == -1)
	{
		if (errno == ENOENT)
			throw SyscallException();
		if ((uint64_t)st.st_size != length)
			return false;
	}
	return true;
}

/*
 * Exception::ERR_CODE_NULL_REF
 * SyscallException
 */

bool FileManager::File_exists(const std::string & fn, uint64_t length) const
{
	return File_exists(fn.c_str(), length);
}

void * FileManager::cache_thread(void * arg)
	{
		int ret = 1;
		FileManager * fm = (FileManager*)arg;
		while(!fm->m_thread_stop)
		{
			//printf("cache_thread loop\n");
			pthread_mutex_lock(&fm->m_mutex);
			if (!fm->m_write_cache.empty())
			{
				const write_cache_element * ce = fm->m_write_cache.front();
				if (ce->file->m_instance2delete)
				{
					fm->m_write_cache.pop();
					continue;
				}
				write_event we;
				we.block_id = ce->block_id;
				we.file = ce->file;
				we.errno_ = 0;
				we.exception_errcode = Exception::NO_ERROR;
				try
				{
					fm->prepare_file(we.file);
					we.writted = we.file->_write(ce->block, ce->offset, ce->length);
				}
				catch (Exception & e) {
					we.writted = -1;
					we.exception_errcode = e.get_errcode();
				}
				catch (SyscallException & e)
				{
					we.writted = -1;
					we.errno_ = e.get_errno();
				}
				fm->m_write_cache.pop();
				fm->m_write_event.push_back(we);
				we.file.reset();
			}
			pthread_mutex_unlock(&fm->m_mutex);
			usleep(1000);
		}
		return (void*)ret;
	}

}
} /* namespace file */
