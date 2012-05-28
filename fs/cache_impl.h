/*
 * cache.cpp
 *
 *  Created on: 07.03.2012
 *      Author: ruslan
 */

#include "fs.h"

namespace fs
{
template <class cache_element>
cache<cache_element>::cache()
{
	m_cache_head = NULL;
	m_back = NULL;
	m_front = NULL;
	m_count = 0;
	m_size = 0;
}

template <class cache_element>
void cache<cache_element>::init_cache(uint16_t size)
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

template <class cache_element>
cache<cache_element>::~cache()
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

template <class cache_element>
cache_element * cache<cache_element>::front()
{
	if (m_count == 0)
		return NULL;
	return &m_front->ce;
}

template <class cache_element>
void cache<cache_element>::pop()
{
	if (m_count == 0)
		return;
	m_front = m_front->next;
	m_count--;
}

template <class cache_element>
int cache<cache_element>::push(cache_element & ce)
{
	//uint32_t block_index = get_block_from_id(id);//(eo->id & (uint32_t)4294967295);
	//uint32_t piece = get_piece_from_id(id);//(eo->id - block_index)>>32;
	//if (buf == NULL || length == 0 || length > CACHE_ELEMENT_SIZE)
	//	return ERR_BAD_ARG;
	if (m_count >= m_size)
	{
		printf("NO AVAILABLE MEMORY IN CACHE\n");
		return ERR_INTERNAL;
	}
	if (m_count == 0)
		m_front = m_back;
	//m_back->ce.file = file;
	//m_back->ce.length = length;
	//m_back->ce.offset = offset;
	//m_back->ce.id = id;
	memcpy(&m_back->ce, &ce, sizeof(cache_element));
	//memcpy(m_back->ce.block, buf, length);
	m_back = m_back->next;
	m_count++;
	//printf("pushed\n");
	return ERR_NO_ERROR;
}

template <class cache_element>
bool cache<cache_element>::empty()
{
	return m_count == 0;
}

}
