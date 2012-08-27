/*
 * FileManager.h
 *
 *  Created on: 03.03.2012
 *      Author: ruslan
 */

#pragma once
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>
#include <queue>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "../err/err_code.h"
#include "../exceptions/exceptions.h"
#include "../cfg/glob_cfg.h"
#include "../consts.h"
#include "../utils/utils.h"
#include "../lru_cache/lru_cache.h"
#include "../log/log.h"
#define BUFFER_SIZE 32768
//#define CACHE_ELEMENT_SIZE BLOCK_LENGTH
#define MAX_OPENED_FILES 512

namespace dinosaur {
namespace fs {

struct buffer
{
	char data[BUFFER_SIZE];
	size_t length;
	size_t pos;
};

class FileManager;
class file;
typedef boost::shared_ptr<file> File;

struct write_cache_element
{
	File 		file;
	char 		block[BLOCK_LENGTH];
	uint32_t 	length;
	uint64_t	offset;
	BLOCK_ID 	block_id;
};

struct write_event
{
	BLOCK_ID 				block_id;
	ssize_t 				writted;
	int 					errno_;
	Exception::ERR_CODES 	exception_errcode;
	File 					file;
};

class FileAssociation : public boost::enable_shared_from_this<FileAssociation>
{
public:
	typedef boost::shared_ptr<FileAssociation> ptr;
	FileAssociation(){}
	virtual ~FileAssociation(){}
	ptr get_ptr()
	{
		return shared_from_this();
	}
	virtual int event_file_write(const write_event & eo) = 0;
};


class file
{
private:
	uint64_t 				m_length;
	bool 					m_should_exists;
	std::string				m_fn;
	int 					m_fd;
	FileAssociation::ptr 	m_assoc;
	bool 					m_instance2delete;
public:
	file();
	file(const char * fn, uint64_t length, bool should_exists, const FileAssociation::ptr & assoc) throw(Exception);
	void _open() throw(Exception, SyscallException);
	size_t _write(const char * buf, uint64_t offset, uint64_t length) throw(Exception, SyscallException);
	size_t _read(char * buf, uint64_t offset, uint64_t length) throw(Exception, SyscallException);
	void _close();
	bool is_opened();
#ifdef FS_DEBUG
	const std::string & fn() const {return m_fn;}
	int get_fd() const {return m_fd;}
	void set_fd(int fd) { m_fd = fd; }
#endif
	void get_name(std::string & name);
	const std::string & get_name();
	uint64_t get_length();
	~file();
	friend class FileManager;
};

class FD_LRU_Cache : public lru_cache::LRU_Cache<File, int>//file id -> file descriptor
{
private:

public:
	FD_LRU_Cache(){}
	virtual ~FD_LRU_Cache(){}
	int put(File & file_id, int file_desc, File & deleted_file)  throw(Exception);
};

class cache
{
private:
	struct _queue
	{
		write_cache_element ce;
		_queue * last;
		_queue * next;
	};
	_queue * m_cache_head;
	uint16_t m_count;//объем данных в кэше
	uint16_t m_size;//размер кэша
	_queue * m_back;//ссылка на первый свободный элемент
	_queue * m_front;
public:
	cache();
	void init_cache(uint16_t size) throw (Exception);
	~cache();
	const write_cache_element * const front() const;
	void pop();
	void push(const File & file, const char * buf, uint32_t length, uint64_t offset, const BLOCK_ID & id) throw (Exception);
	bool empty() const;
	uint16_t count() const
	{return m_count;}
};

class FileManager {
private:
	int m_count;
	cfg::Glob_cfg * m_cfg;
	pthread_t m_write_thread;
	pthread_mutex_t m_mutex;
	bool m_thread_stop;
	uint16_t m_cache_size;
	//lockable vars
	cache m_write_cache;
	FD_LRU_Cache m_fd_cache;
	std::set<File> m_files;
	std::list<write_event> m_write_event;
	static void * cache_thread(void * arg);
	void prepare_file(File & file)  throw (Exception, SyscallException);
public:
	FileManager();
	void Init(cfg::Glob_cfg * cfg) throw (Exception);
	~FileManager();
	int File_add(const char * fn, uint64_t length, bool should_exists, const FileAssociation::ptr & assoc, File & file) throw (Exception);
	int File_add(const std::string & fn, uint64_t length, bool should_exists, const FileAssociation::ptr & assoc, File & file) throw (Exception);
	void File_write(File & file, const char * buf, uint32_t length, uint64_t offset, const BLOCK_ID & block_id ) throw (Exception, SyscallException);
	int File_read_immediately(File & file, char * buf, uint64_t offset, uint64_t length) throw (Exception, SyscallException);
	void File_delete(File & file);
	bool File_exists(const char *fn) const;
	bool File_exists(const std::string & fn) const;
	bool File_exists(const char *fn, uint64_t length) const;
	bool File_exists(const std::string & fn, uint64_t length) const;
	void notify();
};

}
}

