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
	m_error = GENERAL_ERROR_NO_ERROR;
}

/* File_add возвращает
 * ERR_UNDEF - критическая
 * ERR_ON_ERROR - нет ошибки
 * описание ошибки +
 */

int FileManager::Init(cfg::Glob_cfg * cfg) {
	// TODO Auto-generated constructor stub
	if (cfg == NULL)
		return ERR_NULL_REF;
	m_count = 0;
	m_cfg = cfg;
	m_cache_size = m_cfg->get_write_cache_size();
	m_write_cache.init_cache(m_cache_size);

	if (m_fd_cache.Init(MAX_OPENED_FILES) != ERR_NO_ERROR)
	{
		m_error = FS_ERROR_CAN_NOT_CREATE_CACHE;
		return ERR_UNDEF;
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
	{
		m_error = GENERAL_ERROR_CAN_NOT_CREATE_THREAD;
		return ERR_UNDEF;
	}
	pthread_attr_destroy(&attr);
	return ERR_NO_ERROR;
}

int FileManager::Init_for_tests(uint16_t write_cache_size, uint16_t fd_cache_size)
{
	m_count = 0;
	m_cfg = NULL;
	m_cache_size = write_cache_size;
	m_write_cache.init_cache(m_cache_size);

	if (m_fd_cache.Init(fd_cache_size) != ERR_NO_ERROR)
		return ERR_INTERNAL;

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
		return ERR_INTERNAL;
	pthread_attr_destroy(&attr);
	return ERR_NO_ERROR;
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

/* File_add возвращает
 * ERR_UNDEF - критическая
 * ERR_ON_ERROR - нет ошибки
 * описание ошибки +
 */

int FileManager::File_add(const char * fn, uint64_t length, bool should_exists, const FileAssociation::ptr & assoc, File & file_)
{
	if (fn == NULL)
		return ERR_UNDEF;
	try
	{
		file_.reset(new file(fn, length, should_exists, assoc));
	}
	catch (Exception & e)
	{
		m_error = e.get_error();
		file_.reset();
		return ERR_UNDEF;
	}
	pthread_mutex_lock(&m_mutex);
	m_files.insert(file_);
	pthread_mutex_unlock(&m_mutex);
	//printf("File added id=%d\n", f->m_id);
	return ERR_NO_ERROR;
}

/* File_add возвращает
 * ERR_UNDEF - критическая
 * ERR_ON_ERROR - нет ошибки
 * описание ошибки +
 */

int FileManager::File_add(const std::string & fn, uint64_t length, bool should_exists, const FileAssociation::ptr & assoc, File & file)
{
	return File_add(fn.c_str(), length, should_exists, assoc, file);
}

/* prepare_file возвращает
 * ERR_UNDEF - критическая
 * ERR_SYSCALL_ERROR
 * ERR_FILE_NOT_EXISTS
 * ERR_ON_ERROR - нет ошибки
 * описание ошибки +
 */

int FileManager::prepare_file(File & file)
{
	if (file == NULL)
	{
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_UNDEF;
	}
	int fd;
	if (m_fd_cache.get(file, &fd) == ERR_LRU_CACHE_NE)
	{
		int ret = file->_open();
		if (ret == ERR_SYSCALL_ERROR)
		{
			m_error = GENERAL_ERROR_SYSCALL;
			m_error += sys_errlist[errno];
			return ret;
		}
		if (ret == ERR_FILE_NOT_EXISTS)
		{
			m_error = FS_ERROR_FILE_NOT_EXISTS;
			m_error += " ";
			m_error += file->m_fn;
			return ret;
		}
		File deleted_file;
		int file_desc = file->m_fd;
		if (m_fd_cache.put(file, file_desc, deleted_file) != ERR_NO_ERROR)
		{
			m_error = FS_ERROR_CACHE_ERROR;
			return ERR_UNDEF;
		}
		if(deleted_file != NULL)
			deleted_file->_close();
	}
	return ERR_NO_ERROR;
}

/*
 * File_write возвращает
 * ERR_UNDEF - критическая ошибка
 * ERR_FULL_CACHE - не критическая, повторить позже
 * ERR_SYSCALL_ERROR -критическая
 * ERR_FILE_NOT_EXISTS - критическая
 * ERR_ON_ERROR - нет ошибки
 * описание ошибки +
 */

int FileManager::File_write(File & file, const char * buf, uint32_t length, uint64_t offset, const BLOCK_ID & block_id)
{
	#ifdef FS_DEBUG
		printf("Write to file %s offset=%llu length=%u\n", file->m_fn,  offset, length);
	#endif
	pthread_mutex_lock(&m_mutex);
	if (buf == NULL || file == NULL)
	{
		pthread_mutex_unlock(&m_mutex);
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		#ifdef FS_DEBUG
			printf("Fail: buf = NULL or file = NULL\n");
		#endif
		return ERR_UNDEF;
	}
	if (length == 0 && offset > 0)
	{
		pthread_mutex_unlock(&m_mutex);
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		#ifdef FS_DEBUG
			printf("Fail: length = 0 and offset > 0\n");
		#endif
		return ERR_UNDEF;
	}
	if (length == 0)
	{
		int ret = prepare_file(file);
		if (ret != ERR_NO_ERROR)
			return ret;
		write_event we;
		we.block_id = block_id;
		we.file = file;
		we.writted = 0;
		m_write_event.push_back(we);
		we.file.reset();
		#ifdef FS_DEBUG
			printf("OK\n");
		#endif
		pthread_mutex_unlock(&m_mutex);
		return ERR_NO_ERROR;
	}
	if (offset >= file->m_length)
	{
		pthread_mutex_unlock(&m_mutex);
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		#ifdef FS_DEBUG
			printf("Fail: offset overflow\n");
		#endif
		return ERR_UNDEF;
	}

	int ret = m_write_cache.push(file, buf, length, offset, block_id);
	if (ret == ERR_FULL_CACHE)
		m_error = FS_ERROR_CACHE_FULL;
	#ifdef FS_DEBUG
			printf("OK\n");
	#endif
	pthread_mutex_unlock(&m_mutex);
	return ret;
}

/*
 * File_read_immediately возвращает
 * ERR_UNDEF - критическая ошибка
 * ERR_SYSCALL_ERROR -критическая
 * ERR_FILE_NOT_EXISTS - критическая
 * ERR_ON_ERROR - нет ошибки
 * описание ошибки +
 */

int FileManager::File_read_immediately(File & file, char * buf, uint64_t offset, uint64_t length)
{
	pthread_mutex_lock(&m_mutex);
	if (buf == NULL || file == NULL)
	{
		pthread_mutex_unlock(&m_mutex);
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_UNDEF;
	}
	if (length == 0 && offset > 0)
	{
		pthread_mutex_unlock(&m_mutex);
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_UNDEF;
	}
	if (length == 0)
	{
		pthread_mutex_unlock(&m_mutex);
		return ERR_NO_ERROR;
	}
	if (offset >= file->m_length)
	{
		pthread_mutex_unlock(&m_mutex);
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_UNDEF;
	}
	int ret = prepare_file(file);
	if (ret != ERR_NO_ERROR)
	{
		pthread_mutex_unlock(&m_mutex);
		return ret;
	}
	ret = file->_read(buf, offset, length);
	if (ret == ERR_FILE_NOT_OPENED)
	{
		pthread_mutex_unlock(&m_mutex);
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_UNDEF;
	}
	if (ret == ERR_SYSCALL_ERROR)
	{
		m_error = GENERAL_ERROR_SYSCALL;
		m_error += sys_errlist[errno];
		pthread_mutex_unlock(&m_mutex);
		return ret;
	}
	pthread_mutex_unlock(&m_mutex);
	return ret;
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

bool FileManager::File_exists(const char *fn) const
{
	if (fn == NULL)
		return false;
	int fd;
	fd = open(fn, O_LARGEFILE);
	if (fd != -1)
	{
		close(fd);
		return true;
	}
	close(fd);
	return false;
}

bool FileManager::File_exists(const std::string & fn) const
{
	return File_exists(fn.c_str());
}

bool FileManager::File_exists(const char *fn, uint64_t length) const
{
	if (fn == NULL)
		return false;
	struct stat st;
	if (stat(fn, &st) == -1 || (uint64_t)st.st_size != length)
		return false;
	return true;
}

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
				ret = fm->prepare_file(we.file);
				if (ret == ERR_NO_ERROR)
				{
					ret = we.file->_write(ce->block, ce->offset, ce->length);
					if (ret == ERR_SYSCALL_ERROR)
					{
						fm->m_error = GENERAL_ERROR_SYSCALL;
						fm->m_error += sys_errlist[errno];
					}
					if (ret == ERR_FILE_NOT_OPENED)
					{
						ret = ERR_UNDEF;
						fm->m_error = GENERAL_ERROR_UNDEF_ERROR;
					}
				}
				fm->m_write_cache.pop();
				we.writted = ret;
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
