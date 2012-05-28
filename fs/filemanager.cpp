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
	printf("FileManager destructor\n");
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
	if (f->_open() != ERR_NO_ERROR)
		return ERR_INTERNAL;

	f->m_id = m_count;
	pthread_mutex_lock(&m_mutex);
	m_files[m_count++] = f;
	pthread_mutex_unlock(&m_mutex);
	return f->m_id;
}

int FileManager::File_write(int file, const char * buf, uint32_t length, uint64_t offset, uint64_t id)
{
	//printf("File_write\n");
	pthread_mutex_lock(&m_mutex);
	if (buf == NULL || length == 0 || m_files.count(file) == 0)
	{
		pthread_mutex_unlock(&m_mutex);
		return ERR_BAD_ARG;
	}
	if (offset >= m_files[file]->m_length)
	{
		pthread_mutex_unlock(&m_mutex);
		return ERR_BAD_ARG;
	}

	int ret = m_write_cache.push(file, buf, length, offset, id);
	//pthread_cond_signal(&m_cond);
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


} /* namespace file */
