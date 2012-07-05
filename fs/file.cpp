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
#ifdef FS_DEBUG
	printf("file default constructor\n");
#endif
	m_length = 0;
	m_fictive = false;
	m_fn = NULL;
	m_fd = -1;
	m_instance2delete = false;
}

file::file(const char * fn, uint64_t length, bool fictive, const FileAssociation::ptr & assoc)
{
#ifdef FS_DEBUG
	printf("file constructor fn=%s length=%llu\n", fn, length);
#endif
	if (fn == NULL || length == 0)
		throw Exception("Can not create file");
	int fn_str_len = strlen(fn);
	m_fn = new char[fn_str_len + 1];
	//memset(m_fn, 0, strlen(fn) + 1);
	strncpy(m_fn, fn, fn_str_len);
	m_fn[fn_str_len] = '\0';
	m_length = length;
	m_fictive = fictive;
	m_assoc = assoc;
	m_fd = -1;
	m_instance2delete = false;
}

file::~file()
{
#ifdef FS_DEBUG
	printf("file destructor fn=%s %X\n",m_fn, this);
#endif
	_close();
	if (m_fn != NULL)
		delete[] m_fn;
#ifdef FS_DEBUG
	printf("file destructed\n");
#endif
}

int file::_open()
{
	//printf("opening file id=%d\n", m_id);
	bool _new = false;
	struct stat st;
	//если файла нет, говорим что новый
	if (stat(m_fn, &st) == -1 || (uint64_t)st.st_size != m_length)
		_new = true;
	if (!_new)
	{
		//printf("file is not new, opening\n");
		m_fd = open(m_fn, O_RDWR | O_LARGEFILE);
		if (m_fd == -1)
		{
			return ERR_SYSCALL_ERROR;
		}
	}
	//создаем/открываем файл
	//если файл новый, то транкаем его
	if (_new)
	{
		//printf("file is new, creating and opening\n");
		m_fd = open(m_fn, O_RDWR | O_CREAT | O_LARGEFILE, S_IRWXU | S_IRWXG | S_IRWXO);
		if (m_fd == -1)
		{
			return ERR_SYSCALL_ERROR;
		}
		//printf("truncating\n");
		if (ftruncate64(m_fd, m_length) == -1)
		{
			return ERR_SYSCALL_ERROR;
		}
	}
	//printf("ok\n");
	return ERR_NO_ERROR;
}

int file::_write(const char * buf, uint64_t offset, uint64_t length)
{
	//pFile = fopen ( m_files[file_index++].name , "r+"
	int ret;
	if (!is_opened())
		return ERR_FILE_NOT_OPENED;
	//printf("write id=%d %llu %llu\n", m_id, offset, length);
	ret = lseek64 ( m_fd , offset , SEEK_SET );
	if (ret == -1)
	{
		//_close();
		//printf("lseek error\n");
		return ERR_SYSCALL_ERROR;
	}
	ret = write(m_fd, buf, length);
	if (ret == -1)
	{
		//_close();
		return ERR_SYSCALL_ERROR;
	}
	//_close();
	return ret;
}

int file::_read(char * buf, uint64_t offset, uint64_t length)
{
	int ret;
	if (!is_opened())
		return ERR_FILE_NOT_OPENED;
	ret = lseek64(m_fd, offset, SEEK_SET);
	if (ret == -1)
	{
		//_close();
		return ERR_SYSCALL_ERROR;
	}
	ssize_t r = read(m_fd, buf, length);
	if (r == -1)
	{
		return ERR_SYSCALL_ERROR;
	}
	//_close();
	return r;
}

void file::_close()
{
	close(m_fd);
	//printf("file closed id=%d\n", m_id);
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

uint64_t file::get_length()
{
	return m_length;
}


}
