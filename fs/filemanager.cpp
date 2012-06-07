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
	for(std::map<int, file*>::iterator iter = m_files.begin(); iter != m_files.end(); ++iter)
	{
		delete (*iter).second;
	}
}

int FileManager::File_add(const char * fn, uint64_t length, bool fictive, file_event * assoc)
{
	if (fn == NULL)
		return ERR_BAD_ARG;
	file * f;
	try
	{
		f = new file(fn, length, fictive, assoc);
	}
	catch (Exception & e)
	{
		return ERR_INTERNAL;
	}
	f->m_id = m_count;
	pthread_mutex_lock(&m_mutex);
	m_files[m_count++] = f;
	pthread_mutex_unlock(&m_mutex);
	//printf("File added id=%d\n", f->m_id);
	return f->m_id;
}

int FileManager::prepare_file(file * file)
{
	if (file == NULL)
		return ERR_NULL_REF;
	int fd;
	int file_id = file->m_id;
	//printf("preparing file id=%d\n", file_id);
	if (m_fd_cache.get(file_id,&fd) == ERR_LRU_CACHE_NE)
	{
		//pthread_mutex_lock(&m_mutex);
		if (file->_open() != ERR_NO_ERROR)
		{
			//pthread_mutex_unlock(&m_mutex);
			//printf("can not open file\n");
			return ERR_INTERNAL;
		}
		int deleted_file_id = -1;
		int file_desc = file->m_fd;
		if (m_fd_cache.put(file_id, file_desc, &deleted_file_id) != ERR_NO_ERROR)
		{
			//pthread_mutex_unlock(&m_mutex);
			//printf("can not put to fd cache\n");
			return ERR_INTERNAL;
		}
		if(deleted_file_id != -1)
			m_files[deleted_file_id]->_close();
		//pthread_mutex_unlock(&m_mutex);
	}
	//printf("prepare ok\n");
	return ERR_NO_ERROR;
}

int FileManager::File_write(int file, const char * buf, uint32_t length, uint64_t offset, uint64_t block_id)
{
	//printf("File_write id=%d\n", file);
	pthread_mutex_lock(&m_mutex);
	if (buf == NULL || length == 0 || m_files.count(file) == 0)
	{
		pthread_mutex_unlock(&m_mutex);
		//printf("bad args\n");
		return ERR_BAD_ARG;
	}
	if (offset >= m_files[file]->m_length)
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

int FileManager::File_read_immediately(int _file, char * buf, uint64_t offset, uint64_t length)
{
	pthread_mutex_lock(&m_mutex);
	if (buf == NULL || length == 0 || m_files.count(_file) == 0)
	{
		pthread_mutex_unlock(&m_mutex);
		return ERR_BAD_ARG;
	}
	file * f = m_files[_file];
	if (offset >= f->m_length)
	{
		pthread_mutex_unlock(&m_mutex);
		return ERR_BAD_ARG;
	}
	if (prepare_file(f) != ERR_NO_ERROR)
	{
		pthread_mutex_unlock(&m_mutex);
		return ERR_INTERNAL;
	}
	int r = f->_read(buf, offset, length);
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
			//printf("thread loop\n");
			pthread_mutex_lock(&fm->m_mutex);
			//printf("CACHE wait...\n");
			//pthread_cond_wait(&fm->m_cond, &fm->m_mutex);
			//printf("CACHE signal received\n");
			if (!fm->m_write_cache.empty())
			{
				write_cache_element * ce = fm->m_write_cache.front();
				//memcpy(&ce, fm->m_write_cache.front(), sizeof(write_cache_element));
				//fm->m_write_cache.pop();
				//pthread_mutex_unlock(&fm->m_mutex);
				file * f = fm->m_files[ce->file];
				write_event we;
				we.block_id = ce->block_id;
				we.assoc = f->m_assoc;
				if (fm->prepare_file(f) != ERR_NO_ERROR)
					ret = -1;
				else
					ret = f->_write(ce->block, ce->offset, ce->length);
				//pthread_mutex_lock(&fm->m_mutex);
				fm->m_write_cache.pop();
				we.writted = ret;
				fm->m_write_event.push_back(we);
			}
			pthread_mutex_unlock(&fm->m_mutex);
			usleep(50);
		}
		//printf("CACHE_THREAD stopped\n");
		return (void*)ret;
	}


} /* namespace file */
