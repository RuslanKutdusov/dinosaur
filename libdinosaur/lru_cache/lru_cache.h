/*
 * lru_cache.h
 *
 *  Created on: 28.05.2012
 *      Author: ruslan
 */
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>
#include <queue>
#include <list>
#include <map>
#include <boost/bimap.hpp>
#include "../consts.h"
#include "../err/err_code.h"


namespace dinosaur {
namespace lru_cache
{

template <class cache_key, class cache_element>
class LRU_Cache {
protected:
	typedef std::map<cache_key, cache_element *> hash_table;
	typedef typename hash_table::iterator hash_table_iterator;

	typedef boost::bimap<uint64_t, cache_key> time_queue;
	typedef typename time_queue::iterator time_queue_iterator;
	typedef typename time_queue::left_map::iterator time_queue_left_iterator;
	typedef typename time_queue::value_type time_queue_value_type;

	//typedef std::map<uint64_t, cache_key> time_queue;
	std::list<cache_element *> m_free_elements;
	hash_table m_hash_table;
	time_queue m_time_queue;
	cache_element * m_elements;
	uint16_t m_elements_count;
	uint64_t get_time();
public:
	LRU_Cache();
	virtual ~LRU_Cache();
	void Init(uint16_t size);
	void put(cache_key key, cache_element * value);
	void get(cache_key key, cache_element * value);
	bool empty();
	void remove(cache_key key);
};

template <class cache_key, class cache_element>
LRU_Cache<cache_key, cache_element>::LRU_Cache() {
	// TODO Auto-generated constructor stub
	m_elements_count = 0;
	m_elements = NULL;
}

template <class cache_key, class cache_element>
LRU_Cache<cache_key, cache_element>::~LRU_Cache() {
	// TODO Auto-generated destructor stub
	if (m_elements != NULL && m_elements_count > 0)
		delete[] m_elements;
}

template <class cache_key, class cache_element>
uint64_t LRU_Cache<cache_key, cache_element>::get_time()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_sec << 32) + tv.tv_usec;
}

/*
 * Exception::ERR_CODE_NO_MEMORY_AVAILABLE
 * Exception::ERR_CODE_UNDEF
 */

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::Init(uint16_t size)//thrown
{
	try
	{
		m_elements_count = size;
		m_elements = new cache_element[size];//исключение возможно здесь
		for(uint16_t i = 0; i < size; i++)
			m_free_elements.push_front(&m_elements[i]);//исключение возможно здесь
	}
	catch (std::bad_alloc & e) {
		throw Exception(Exception::ERR_CODE_NO_MEMORY_AVAILABLE);
	}
	catch(...)
	{
		throw Exception(Exception::ERR_CODE_UNDEF);
	}
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_UNDEF
 */

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::put(cache_key key, cache_element * value)
{
	if (value == NULL || m_elements == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	try
	{
		if (m_free_elements.empty())
		{
			typename time_queue::right_iterator time_queue_iter_to_element = m_time_queue.right.find(key);
			//обновляемся если есть элемент с таким ключом
			if (time_queue_iter_to_element != m_time_queue.right.end() || m_time_queue.right.count(key) != 0)
			{
				memcpy(m_hash_table[key], value, sizeof(cache_element));
				m_time_queue.right.replace_data(time_queue_iter_to_element, get_time());
				return;
			}
			//находим самый старый элемент
			time_queue_left_iterator time_queue_iter_to_old_element = m_time_queue.left.begin();
			//берем его ключ
			cache_key key_of_old_element = time_queue_iter_to_old_element->second;
			//находим ссылку на элемент
			hash_table_iterator hash_table_iter_to_old_element = m_hash_table.find(key_of_old_element);
			//добавляем ссылку в free_elements
			m_free_elements.push_back((*hash_table_iter_to_old_element).second);
			//удаляем из таблицы и очереди элемент
			m_hash_table.erase(hash_table_iter_to_old_element);
			m_time_queue.left.erase(time_queue_iter_to_old_element);
			//в итоге в free_elements есть свободный элемент, его и займем
			return put(key, value);
		}
		else
		{
			cache_element * free_element = m_free_elements.front();
			memcpy(free_element, value, sizeof(cache_element));
			//cache_key key(torrent, block_id);
			m_hash_table[key] = free_element;
			m_free_elements.pop_front();
			//ищем справа данный свободный элемент(key)
			typename time_queue::right_iterator time_queue_iter_to_element = m_time_queue.right.find(key);
			//если его нет, добавляем
			if (time_queue_iter_to_element == m_time_queue.right.end() && m_time_queue.right.count(key) == 0)
				m_time_queue.insert(time_queue_value_type(get_time(), key));
			else
				//если он есть в time_queue, меняем слева время на текущее
				m_time_queue.right.replace_data(time_queue_iter_to_element, get_time());
			//m_time_queue[get_time()] = key;
			//m_time_queue
		}
	}
	catch(...)
	{
		throw Exception(Exception::ERR_CODE_UNDEF);
	}
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_LRU_CACHE_NE
 * Exception::ERR_CODE_UNDEF
 */

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::get(cache_key key, cache_element * value)
{
	if (value == NULL || m_elements == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	try
	{
		//cache_key key(torrent, block_id);
		hash_table_iterator iter = m_hash_table.find(key);
		if (iter == m_hash_table.end() && m_hash_table.count(key) == 0)
			throw Exception(Exception::ERR_CODE_LRU_CACHE_NE);

		cache_element * element = (*iter).second;
		memcpy(value, element, sizeof(cache_element));
		//ищем справа данный элемент(key)
		typename time_queue::right_iterator it = m_time_queue.right.find(key);
		//если его нет, добавляем
		if (it == m_time_queue.right.end() && m_time_queue.right.count(key) == 0)
			m_time_queue.insert(time_queue_value_type(get_time(), key));
		else
			//если он есть в time_queue, меняем слева время на текущее
			m_time_queue.right.replace_data(it, get_time());

	}
	catch(Exception & e)
	{
		throw Exception(e);
	}
	catch(...)
	{
		throw Exception(Exception::ERR_CODE_UNDEF);
	}
}

template <class cache_key, class cache_element>
bool LRU_Cache<cache_key, cache_element>::empty()
{
	return m_hash_table.empty();
}

/*
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_LRU_CACHE_NE
 */

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::remove(cache_key key)
{
	try
	{
		//cache_key key(torrent, block_id);
		hash_table_iterator hash_table_iter_to_element = m_hash_table.find(key);
		if (hash_table_iter_to_element == m_hash_table.end() && m_hash_table.count(key) == 0)
			throw Exception(Exception::ERR_CODE_LRU_CACHE_NE);
		typename time_queue::right_iterator time_queue_iter_to_element = m_time_queue.right.find(key);
		m_free_elements.push_back((*hash_table_iter_to_element).second);
		//удаляем из таблицы и очереди элемент
		m_hash_table.erase(hash_table_iter_to_element);
		m_time_queue.right.erase(time_queue_iter_to_element);
	}
	catch(Exception & e)
	{
		throw Exception(e);
	}
	catch(...)
	{
		throw Exception(Exception::ERR_CODE_UNDEF);
	}
}

}
} /* namespace Bittorrent */
