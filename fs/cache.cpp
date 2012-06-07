/*
 * cache.cpp
 *
 *  Created on: 07.03.2012
 *      Author: ruslan
 */

#include "fs.h"

namespace fs
{
cache::cache()
{
	m_cache_head = NULL;
	m_back = NULL;
	m_front = NULL;
	m_count = 0;
	m_size = 0;
}


void cache::init_cache(uint16_t size)
{
	if (size == 0)
		size = 1;
	m_size = size;
	m_count = 0;
	m_front = NULL;
	m_cache_head = new _queue;
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
	}
	m_back = m_cache_head;
}


cache::~cache()
{
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
}


write_cache_element * cache::front()
{
	if (m_count == 0)
		return NULL;
	return &m_front->ce;
}

void cache::pop()
{
	if (m_count == 0)
		return;
	m_front = m_front->next;
	m_count--;
}


int cache::push(int file, const char * buf, uint32_t length, uint64_t offset, uint64_t id)
{
	if (m_count >= m_size)
	{
		printf("NO AVAILABLE MEMORY IN CACHE\n");
		return ERR_INTERNAL;
	}
	if (m_count == 0)
		m_front = m_back;
	m_back->ce.file = file;
	m_back->ce.block_id = id;
	m_back->ce.length = length;
	m_back->ce.offset = offset;
	memcpy(m_back->ce.block, buf, length);
	m_back = m_back->next;
	m_count++;
	//printf("pushed\n");
	return ERR_NO_ERROR;
}


bool cache::empty()
{
	return m_count == 0;
}

}
