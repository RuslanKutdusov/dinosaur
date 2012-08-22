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
	LOG(INFO) << "cache default constructor";
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
		LOG(INFO) << "cache initializing...";
		LOG(INFO) << "cache_head=" << m_cache_head;
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
			LOG(INFO) << "cache element "
					<< i <<"=" << q
					<< " last=" << q->last
					<< " next=" << q->next;
	#endif
		}
		m_back = m_cache_head;
	#ifdef FS_DEBUG
		LOG(INFO) << "cache_head last= " << m_cache_head->last
				  << " cache_head next=" << m_cache_head->next;
		LOG(INFO) << "front=" << m_front
				  << " back=" << m_back;
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
	LOG(INFO) << "cache destructor";
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
	LOG(INFO) << "cache destructed";
#endif
}


const write_cache_element * const  cache::front() const
{
#ifdef FS_DEBUG
	LOG(INFO) << "cache::front front="<< m_front
			  << " back=" << m_back
			  << " count="<< m_count;
#endif
	if (m_count == 0)
	{
#ifdef FS_DEBUG
		LOG(INFO) << "cache is empty";
#endif
		return NULL;
	}
#ifdef FS_DEBUG
	LOG(INFO) << "front ok";
#endif
	return &m_front->ce;
}

void cache::pop()
{
#ifdef FS_DEBUG
	LOG(INFO) << "cache::pop front="<< m_front
			  << " back=" << m_back
			  << " count="<< m_count;
#endif
	if (m_count == 0)
	{
#ifdef FS_DEBUG
		LOG(INFO) << "cache is empty";
#endif
		return;
	}
	m_front->ce.file.reset();
	m_front = m_front->next;
	m_count--;
#ifdef FS_DEBUG
	LOG(INFO) << "cache::pop ok front="<< m_front
				  << " back=" << m_back
				  << " count="<< m_count;
#endif
}

/*
 * Exception::ERR_CODE_CACHE_FULL
 * Exception::ERR_CODE_BLOCK_TOO_BIG
 */

void cache::push(const File & file, const char * buf, uint32_t length, uint64_t offset, const BLOCK_ID & id) throw (Exception)
{
#ifdef FS_DEBUG
	LOG(INFO) << "cache push file=" << file->fn().c_str()
			<< " length=" << length
			<< " offset=" <<offset
			<< " piece=" << id.first
			<< " block=" << id.second;
#endif
	if (m_count >= m_size)
	{
#ifdef FS_DEBUG
		LOG(INFO) << "NO AVAILABLE MEMORY IN CACHE front=" << m_front << " back=" << m_back << " count=" << m_count;
#endif
		throw Exception(Exception::ERR_CODE_CACHE_FULL);
	}
	if (length > BLOCK_LENGTH || length == 0)
	{
#ifdef FS_DEBUG
		LOG(INFO) << "Invalid length";
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
	LOG(INFO) << "pushed in "<< m_back << " back=" << m_back->next << " front=" << m_front << " count=" << m_count + 1;
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
