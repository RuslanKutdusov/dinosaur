/*
 * file.cpp
 *
 *  Created on: 07.03.2012
 *      Author: ruslan
 */

#include "fs.h"

namespace fs
{

file::file()
{
	m_length = 0;
	m_fictive = false;
	m_fn = NULL;
	m_fd = -1;
	m_assoc = NULL;
	memset(&m_wr_buffer,0,sizeof(struct buffer));
	memset(&m_rd_buffer,0,sizeof(struct buffer));
	pthread_mutex_init(&m_mutex, NULL);
}

file::file(const char * fn, uint64_t length, bool fictive, file_event * assoc)
{
	if (fn == NULL || length == 0)
		throw Exception("Can not create file");
	m_fn = new char[strlen(fn) + 1];
	memset(m_fn, 0, strlen(fn) + 1);
	strcpy(m_fn, fn);
	m_length = length;
	m_fictive = fictive;
	m_assoc = assoc;
	memset(&m_wr_buffer,0,sizeof(struct buffer));
	memset(&m_rd_buffer,0,sizeof(struct buffer));
	pthread_mutex_init(&m_mutex, NULL);
	m_fd = -1;
}

file::~file()
{
	_close();
	if (m_fn != NULL)
		delete[] m_fn;
	pthread_mutex_destroy(&m_mutex);
}

int file::_open()
{
	pthread_mutex_lock(&m_mutex);
	bool _new = false;
	struct stat st;
	//если файла нет, говорим что новый
	if (stat(m_fn, &st) == -1 || (uint64_t)st.st_size != m_length)
		_new = true;
	if (!_new)
	{
		m_fd = open(m_fn, O_RDWR | O_LARGEFILE);
		if (m_fd == -1)
		{
			pthread_mutex_unlock(&m_mutex);
			return ERR_SYSCALL_ERROR;
		}
	}
	//создаем/открываем файл
	/*long old_flags = fcntl(m_fd, F_GETFL);
	if (old_flags == -1)
		return ERR_SYSCALL_ERROR;
	if (fcntl(m_fd, F_SETFL, old_flags | O_NONBLOCK) == -1)
		return ERR_SYSCALL_ERROR;*/
	//если файл новый, то транкаем его
	if (_new)
	{
		char dir[MAX_FILENAME_LENGTH];
		memset(dir, 0, MAX_FILENAME_LENGTH);
		dir[0] = '/';
		char * pch=strchr(&m_fn[1], '/');
		char * last = &m_fn[0];
		if (pch == NULL)
		{
			pthread_mutex_unlock(&m_mutex);
			return ERR_FILE_ERROR;
		}
		while (pch != NULL)
		{
			int len = pch - last;
			strncat(dir, last + 1, len);
			mkdir(dir, S_IRWXU);
			last = pch;
			pch = strchr(pch + 1,'/');
		}
		m_fd = open(m_fn, O_RDWR | O_CREAT | O_LARGEFILE, S_IRWXU | S_IRWXG | S_IRWXO);
		if (m_fd == -1)
		{
			pthread_mutex_unlock(&m_mutex);
			return ERR_SYSCALL_ERROR;
		}
		if (ftruncate64(m_fd, m_length) == -1)
		{
			pthread_mutex_unlock(&m_mutex);
			return ERR_SYSCALL_ERROR;
		}
	}
	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

int file::_write(const char * buf, uint64_t offset, uint64_t length)
{
	//pFile = fopen ( m_files[file_index++].name , "r+"
	int ret;
	if (!is_opened())
	{
		ret = _open();
		if (ret != ERR_NO_ERROR)
			return ret;
	}
	pthread_mutex_lock(&m_mutex);
	//printf("write %llu %llu\n", offset, length);
	ret = lseek ( m_fd , offset , SEEK_SET );
	if (ret == -1)
	{
		//_close();
		printf("lseek error\n");
		pthread_mutex_unlock(&m_mutex);
		return ERR_SYSCALL_ERROR;
	}
	ret = write(m_fd, buf, length);
	if (ret == -1)
	{
		//_close();
		pthread_mutex_unlock(&m_mutex);
		return ERR_SYSCALL_ERROR;
	}
	//_close();
	pthread_mutex_unlock(&m_mutex);
	return ret;
}

int file::_read(char * buf, uint64_t offset, uint64_t length)
{
	int ret;
	if (!is_opened())
	{
		ret = _open();
		if (ret != ERR_NO_ERROR)
			return ret;
	}
	pthread_mutex_lock(&m_mutex);
	ret = lseek(m_fd, offset, SEEK_SET);
	if (ret == -1)
	{
		//_close();
		pthread_mutex_unlock(&m_mutex);
		return ERR_SYSCALL_ERROR;
	}
	ssize_t r = read(m_fd, buf, length);
	if (r == -1)
	{
		//_close();
		pthread_mutex_unlock(&m_mutex);
		return ERR_SYSCALL_ERROR;
	}
	//_close();
	pthread_mutex_unlock(&m_mutex);
	return r;
}

void file::_close()
{
	pthread_mutex_lock(&m_mutex);
	close(m_fd);
	m_fd = -1;
	pthread_mutex_unlock(&m_mutex);
}

bool file::is_opened()
{

	pthread_mutex_lock(&m_mutex);
	bool b = m_fd != -1;
	pthread_mutex_unlock(&m_mutex);
	return b;
}

}
