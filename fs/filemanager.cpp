/*
 * FileManager.cpp
 *
 *  Created on: 03.03.2012
 *      Author: ruslan
 */

#include "fs.h"

namespace fs {

FileManager::FileManager()
{
	m_write_thread = 0;
}

int FileManager::Init(cfg::Glob_cfg * cfg) {
	// TODO Auto-generated constructor stub
	if (cfg == NULL)
		return ERR_NULL_REF;
	m_count = 0;
	m_cfg = cfg;
	m_cache_size = m_cfg->get_cache_size();
	m_write_cache.init_cache(m_cache_size);

	if (m_fd_cache.Init(MAX_OPENED_FILES) != ERR_NO_ERROR)
		return ERR_INTERNAL;

	m_thread_stop = false;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutexattr_t   mta;
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init (&m_cond, NULL);
	//pthread_mutex_init(&m_mutex_timeout_sockets, NULL);
	if (pthread_create(&m_write_thread, &attr, FileManager::cache_thread, (void *)this))
		return ERR_INTERNAL;
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
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init (&m_cond, NULL);
	//pthread_mutex_init(&m_mutex_timeout_sockets, NULL);
	if (pthread_create(&m_write_thread, &attr, FileManager::cache_thread, (void *)this))
		return ERR_INTERNAL;
	pthread_attr_destroy(&attr);
	return ERR_NO_ERROR;
}

FileManager::~FileManager() {
	// TODO Auto-generated destructor stub
	//printf("FileManager destructor\n");
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
}

int FileManager::File_add(const char * fn, uint64_t length, bool fictive, const FileAssociation::ptr & assoc, File & file_)
{
	if (fn == NULL)
		return ERR_BAD_ARG;
	try
	{
		file_.reset(new file(fn, length, fictive, assoc));
	}
	catch (Exception & e)
	{
		file_.reset();
		return ERR_INTERNAL;
	}
	pthread_mutex_lock(&m_mutex);
	m_files.insert(file_);
	pthread_mutex_unlock(&m_mutex);
	//printf("File added id=%d\n", f->m_id);
	return ERR_NO_ERROR;
}

int FileManager::prepare_file(File & file)
{
	if (file == NULL)
		return ERR_NULL_REF;
	int fd;
	//printf("preparing file id=%d\n", file_id);
	if (m_fd_cache.get(file,&fd) == ERR_LRU_CACHE_NE)
	{
		//pthread_mutex_lock(&m_mutex);
		if (file->_open() != ERR_NO_ERROR)
		{
			//pthread_mutex_unlock(&m_mutex);
			//printf("can not open file\n");
			return ERR_INTERNAL;
		}
		File deleted_file;
		int file_desc = file->m_fd;
		if (m_fd_cache.put(file, file_desc, deleted_file) != ERR_NO_ERROR)
		{
			//pthread_mutex_unlock(&m_mutex);
			//printf("can not put to fd cache\n");
			return ERR_INTERNAL;
		}
		if(deleted_file != NULL)
			deleted_file->_close();
		//pthread_mutex_unlock(&m_mutex);
	}
	//printf("prepare ok\n");
	return ERR_NO_ERROR;
}

int FileManager::File_write(File & file, const char * buf, uint32_t length, uint64_t offset, uint64_t block_id)
{
	//printf("File_write id=%d\n", file);
	pthread_mutex_lock(&m_mutex);
	if (buf == NULL || length == 0 )
	{
		pthread_mutex_unlock(&m_mutex);
		//printf("bad args\n");
		return ERR_BAD_ARG;
	}
	if (offset >= file->m_length)
	{
		pthread_mutex_unlock(&m_mutex);
		//printf("offset overflow\n");
		return ERR_BAD_ARG;
	}

	int ret = m_write_cache.push(file, buf, length, offset, block_id);
	//pthread_cond_signal(&m_cond);
	//printf("ok\n");
	pthread_mutex_unlock(&m_mutex);
	return ret;
}

int FileManager::File_read_immediately(File & file, char * buf, uint64_t offset, uint64_t length)
{
	pthread_mutex_lock(&m_mutex);
	if (buf == NULL || length == 0)
	{
		pthread_mutex_unlock(&m_mutex);
		return ERR_BAD_ARG;
	}
	if (offset >= file->m_length)
	{
		pthread_mutex_unlock(&m_mutex);
		return ERR_BAD_ARG;
	}
	if (prepare_file(file) != ERR_NO_ERROR)
	{
		pthread_mutex_unlock(&m_mutex);
		return ERR_INTERNAL;
	}
	int r = file->_read(buf, offset, length);
	pthread_mutex_unlock(&m_mutex);
	return r;
}

bool FileManager::get_write_event(write_event * id)
{
	pthread_mutex_lock(&m_mutex);
	if (m_write_event.size() == 0)
	{
		pthread_mutex_unlock(&m_mutex);
		return false;
	}
	//*id = m_write.front();
	memcpy(id, &m_write_event.front(), sizeof(write_event));
	m_write_event.pop_front();
	pthread_mutex_unlock(&m_mutex);
	return true;
}

void * FileManager::cache_thread(void * arg)
	{
		int ret = 1;
		FileManager * fm = (FileManager*)arg;
		//printf("CACHE_THREAD started\n");
		while(!fm->m_thread_stop)
		{
			//printf("cache_thread loop\n");
			pthread_mutex_lock(&fm->m_mutex);
			//printf("CACHE wait...\n");
			//pthread_cond_wait(&fm->m_cond, &fm->m_mutex);
			//printf("CACHE signal received\n");
			if (!fm->m_write_cache.empty())
			{
				write_cache_element * ce = fm->m_write_cache.front();
				write_event we;
				we.block_id = ce->block_id;
				we.file = ce->file;
				ce->file.reset();
				if (fm->prepare_file(we.file) != ERR_NO_ERROR)
					ret = -1;
				else
					ret = we.file->_write(ce->block, ce->offset, ce->length);
				//pthread_mutex_lock(&fm->m_mutex);
				fm->m_write_cache.pop();
				we.writted = ret;
				fm->m_write_event.push_back(we);
				we.file.reset();
			}
			pthread_mutex_unlock(&fm->m_mutex);
			usleep(50);
		}
		//printf("CACHE_THREAD stopped\n");
		return (void*)ret;
	}


} /* namespace file */
