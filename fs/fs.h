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
#include "../err/err_code.h"
#include "../exceptions/exceptions.h"
#include "../cfg/glob_cfg.h"
#include "../consts.h"
#include "../utils/utils.h"
#include "../lru_cache/lru_cache.h"
#define BUFFER_SIZE 32768
//#define CACHE_ELEMENT_SIZE BLOCK_LENGTH
#define MAX_OPENED_FILES 512

namespace fs {

struct buffer
{
	char data[BUFFER_SIZE];
	size_t length;
	size_t pos;
};

struct write_cache_element
{
	int file;
	char block[BLOCK_LENGTH];
	uint32_t length;
	uint64_t offset;
	uint64_t block_id;
};

class FileManager;
class file_event;


struct write_event
{
	uint64_t block_id;
	ssize_t writted;
	file_event * assoc;//файл куда писали ассоциирован с этим классом
};


class file_event
{
public:
	file_event(){};
	virtual ~file_event(){};
	virtual int event_file_write(write_event * eo) = 0;
};

class file
{
private:
	uint64_t m_length;
	uint16_t m_fictive;
	char * m_fn;
	int m_fd;
	int m_id;
	file_event * m_assoc;
public:
	file();
	file(const char * fn, uint64_t length, bool fictive, file_event * assoc);
	int _open();
	int _write(const char * buf, uint64_t offset, uint64_t length);
	int _read(char * buf, uint64_t offset, uint64_t length);
	void _close();
	bool is_opened();
	~file();
	friend class FileManager;
};

class FD_LRU_Cache : public lru_cache::LRU_Cache<int, int>//file id -> file descriptor
{
private:

public:
	FD_LRU_Cache(){}
	virtual ~FD_LRU_Cache(){}
	int put(int file_id, int file_desc, int * deleted_file_id);
	void dump();
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
	void init_cache(uint16_t size);
	~cache();
	write_cache_element * front();
	void pop();
	int push(int file, const char * buf, uint32_t length, uint64_t offset, uint64_t id);
	bool empty();
	uint16_t count(){return m_count;}
};

class FileManager {
private:
	int m_count;
	cfg::Glob_cfg * m_cfg;
	pthread_t m_write_thread;
	pthread_mutex_t m_mutex;
	pthread_cond_t m_cond;
	bool m_thread_stop;
	uint16_t m_cache_size;
	//lockable vars
	cache m_write_cache;
	FD_LRU_Cache m_fd_cache;
	std::map<int, file*> m_files;
	std::list<write_event> m_write_event;
	static void * cache_thread(void * arg);
	int prepare_file(file* file);
public:
	FileManager();
	int Init(cfg::Glob_cfg * cfg);
	int Init_for_tests(uint16_t write_cache_size, uint16_t fd_cache_size);
	~FileManager();
	int File_add(const char * fn, uint64_t length, bool fictive, file_event * assoc);
	int File_write(int file, const char * buf, uint32_t length, uint64_t offset, uint64_t block_id );
	int File_read_immediately(int file, char * buf, uint64_t offset, uint64_t length);
	bool get_write_event(write_event * id);
	void test_dump_fd_cache()
	{
		m_fd_cache.dump();
	}
};


}

