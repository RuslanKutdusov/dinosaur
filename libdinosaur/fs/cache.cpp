/*
 * cache.cpp
 *
 *  Created on: 07.03.2012
 *      Author: ruslan
 */

#include "fs.h"

namespace dinosaur {
namespace fs
{
cache::cache()
{
#ifdef BITTORRENT_DEBUG
	printf("cache default constructor\n");
#endif
	m_cache_head = NULL;
	m_back = NULL;
	m_front = NULL;
	m_count = 0;
	m_size = 0;
}

/*
 * Exception::ERR_CODE_NO_MEMORY_AVAILABLE
 * Exception::ERR_CODE_UNDEF
 */

void cache::init_cache(uint16_t size) throw (Exception)
{
	if (size == 0)
		size = 1;
	m_size = size;
	m_count = 0;
	m_front = NULL;
	try
	{
		m_cache_head = new _queue;
	#ifdef FS_DEBUG
		printf("cache initializing...\n");
		printf("cache_head=%X\n", m_cache_head);
	#endif
		memset(m_cache_head, 0, sizeof(_queue));
		m_cache_head->last = m_cache_head;
		m_cache_head->next = m_cache_head;
		_queue * last = m_cache_head;
		for(uint16_t i = 1; i < size; i++)
		{
			_queue * q = new _queue;
			memset(q, 0, sizeof(_queue));
			last->next = q;
			q->last = last;
			q->next = m_cache_head;
			m_cache_head->last = q;
			last = q;
	#ifdef FS_DEBUG
			printf("cache element %u=%X, last=%X, next=%X\n", i, q, q->last, q->next);
	#endif
		}
		m_back = m_cache_head;
	#ifdef FS_DEBUG
		printf("cache_head last=%X, cache_head next=%X\n", m_cache_head->last, m_cache_head->next);
		printf("front=%X, back=%X\n", m_front, m_back);
	#endif
	}
	catch (std::bad_alloc & e) {
		throw Exception(Exception::ERR_CODE_NO_MEMORY_AVAILABLE);
	}
	catch(...)
	{
		throw Exception(Exception::ERR_CODE_UNDEF);
	}
}


cache::~cache()
{
#ifdef BITTORRENT_DEBUG
	printf("cache destructor\n");
#endif
	if (m_cache_head != NULL)
	{
		_queue * q = m_cache_head;
		q->last->next = NULL;
		while(q != NULL)
		{
			_queue * next = q->next;
			delete q;
			q = next;
		}
	}
#ifdef BITTORRENT_DEBUG
	printf("cache destructed\n");
#endif
}


const write_cache_element * const  cache::front() const
{
#ifdef FS_DEBUG
	printf("cache::front front=%X, back=%X count=%u\n", m_front, m_back, m_count);
#endif
	if (m_count == 0)
	{
#ifdef FS_DEBUG
		printf("cache is empty\n");
#endif
		return NULL;
	}
#ifdef FS_DEBUG
	printf("front ok\n");
#endif
	return &m_front->ce;
}

void cache::pop()
{
#ifdef FS_DEBUG
	printf("cache pop front=%X, back=%X count=%u\n", m_front, m_back, m_count);
#endif
	if (m_count == 0)
	{
#ifdef FS_DEBUG
		printf("cache is empty\n");
#endif
		return;
	}
	m_front->ce.file.reset();
	m_front = m_front->next;
	m_count--;
#ifdef FS_DEBUG
	printf("pop ok front=%X, back=%X count=%u\n", m_front, m_back, m_count);
#endif
}

/*
 * Exception::ERR_CODE_CACHE_FULL
 * Exception::ERR_CODE_BLOCK_TOO_BIG
 */

void cache::push(const File & file, const char * buf, uint32_t length, uint64_t offset, const BLOCK_ID & id) throw (Exception)
{
#ifdef FS_DEBUG
	printf("cache push file=%s length=%u offset=%llu piece=%u block=%u\n", file->fn().c_str(), length, offset, id.first, id.second);
#endif
	if (m_count >= m_size)
	{
#ifdef FS_DEBUG
		printf("NO AVAILABLE MEMORY IN CACHE front=%X, back=%X count=%u\n", m_front, m_back, m_count);
#endif
		throw Exception(Exception::ERR_CODE_CACHE_FULL);
	}
	if (length > BLOCK_LENGTH || length == 0)
	{
#ifdef FS_DEBUG
		printf("Invalid length\n");
#endif
		throw Exception(Exception::ERR_CODE_BLOCK_TOO_BIG);
	}
	if (m_count == 0)
		m_front = m_back;
	m_back->ce.file = file;
	m_back->ce.block_id = id;
	m_back->ce.length = length;
	m_back->ce.offset = offset;
	memcpy(m_back->ce.block, buf, length);
#ifdef FS_DEBUG
	printf("pushed in %X, back=%X front=%X count=%u\n", m_back, m_back->next, m_front, m_count + 1);
#endif
	m_back = m_back->next;
	m_count++;
}


bool cache::empty() const
{
	return m_count == 0;
}

}
}
