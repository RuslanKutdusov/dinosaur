/*
 * Blockcache.cpp
 *
 *  Created on: 17.04.2012
 *      Author: ruslan
 */

#include "Block_cache.h"

namespace block_cache {

Block_cache::Block_cache() {
	// TODO Auto-generated constructor stub
	m_elements_count = 0;
	m_elements = NULL;
}

Block_cache::~Block_cache() {
	// TODO Auto-generated destructor stub
	if (m_elements != NULL && m_elements_count > 0)
		delete[] m_elements;
}

uint64_t Block_cache::get_time()
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ((uint64_t)ts.tv_sec << 32) + ts.tv_nsec;
}

int Block_cache::Init(uint16_t size)//thrown
{
	try
	{
		m_elements_count = size;
		m_elements = new cache_element[size];//исключение возможно здесь
		for(uint16_t i = 0; i < size; i++)
			m_free_elements.push_front(&m_elements[i]);//исключение возможно здесь
	}
	catch(...)
	{
		return ERR_INTERNAL;
	}
	return ERR_NO_ERROR;
}

int Block_cache::put(void * torrent, uint64_t block_id, char * block)
{
	try
	{
		if (m_free_elements.empty())
		{
			//находим самый старый элемент
			time_queue_left_iterator tq_iter = m_time_queue.left.begin();
			//берем его ключ
			cache_key key = tq_iter->second;
			//находим ссылку на элемент
			hash_table_iterator ht_iter = m_hash_table.find(key);
			//добавляем ссылку в free_elements
			m_free_elements.push_back((*ht_iter).second);
			//удаляем из таблицы и очереди элемент
			m_hash_table.erase(ht_iter);
			m_time_queue.left.erase(tq_iter);
			//в итоге в free_elements есть свободный элемент, его и займем
			return put(torrent, block_id, block);
		}
		else
		{
			cache_element * value = m_free_elements.front();
			memcpy(value->block, block, BLOCK_LENGTH);
			cache_key key(torrent, block_id);
			m_hash_table[key] = value;
			m_free_elements.pop_front();
			//ищем справа данный свободный элемент(key)
			time_queue::right_iterator it = m_time_queue.right.find(key);
			//если его нет, добавляем
			if (it == m_time_queue.right.end() && m_time_queue.right.count(key) == 0)
				m_time_queue.insert(time_queue_value_type(get_time(), key));
			else
				//если он есть в time_queue, меняем слева время на текущее
				m_time_queue.right.replace_data(it, get_time());
			//m_time_queue[get_time()] = key;
			//m_time_queue
		}
	}
	catch(...)
	{
		return ERR_INTERNAL;
	}
	return ERR_NO_ERROR;
}

int Block_cache::get(void * torrent, uint64_t block_id, char * block)
{
	if (block == NULL)
		return ERR_NULL_REF;
	try
	{
		cache_key key(torrent, block_id);
		hash_table_iterator iter = m_hash_table.find(key);
		if (iter == m_hash_table.end() && m_hash_table.count(key) == 0)
			return ERR_BLOCK_CACHE_NE;

		cache_element * value = (*iter).second;
		memcpy(block, value->block, BLOCK_LENGTH);
		//ищем справа данный элемент(key)
		time_queue::right_iterator it = m_time_queue.right.find(key);
		//если его нет, добавляем
		if (it == m_time_queue.right.end() && m_time_queue.right.count(key) == 0)
			m_time_queue.insert(time_queue_value_type(get_time(), key));
		else
			//если он есть в time_queue, меняем слева время на текущее
			m_time_queue.right.replace_data(it, get_time());

	}
	catch(...)
	{
		return ERR_INTERNAL;
	}
	return ERR_NO_ERROR;
}


} /* namespace Bittorrent */
