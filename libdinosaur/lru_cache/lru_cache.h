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

#define DEFAULT_MAX_LRU_SIZE 512

template <class cache_key, class cache_element>
class LRU_Cache {
protected:
	typedef std::map<cache_key, cache_element> hash_table;
	typedef typename hash_table::iterator hash_table_iterator;

	typedef boost::bimap<uint64_t, cache_key> time_queue;
	typedef typename time_queue::iterator time_queue_iterator;
	typedef typename time_queue::left_map::iterator time_queue_left_iterator;
	typedef typename time_queue::value_type time_queue_value_type;

	hash_table m_hash_table;
	time_queue m_time_queue;
	size_t	m_max_size;
	uint64_t get_time();
	LRU_Cache(const LRU_Cache & copy){};
	LRU_Cache & operator=(const LRU_Cache & dht){ return *this; }
public:
	LRU_Cache();
	virtual ~LRU_Cache();
	void Init(uint16_t size);
	void put(const cache_key & key, const cache_element & value);
	void get(const cache_key & key, cache_element & value);
	bool exists(const cache_key & key);
	cache_element & operator[](const cache_key & key);
	void get_without_time_update(const cache_key & key, cache_element & value);
	const cache_key & get_old();
	void get_element_time(const cache_key & key, timeval & t);
	void update_element_time(const cache_key & key);
	bool empty();
	size_t size();
	void remove(const cache_key & key);
	typedef hash_table_iterator iterator;
	iterator begin()
	{
		return m_hash_table.begin();
	}
	iterator end()
	{
		return m_hash_table.end();
	}
};

template <class cache_key, class cache_element>
LRU_Cache<cache_key, cache_element>::LRU_Cache() {
	// TODO Auto-generated constructor stub
	m_max_size = DEFAULT_MAX_LRU_SIZE;
}

template <class cache_key, class cache_element>
LRU_Cache<cache_key, cache_element>::~LRU_Cache() {
	// TODO Auto-generated destructor stub
}

template <class cache_key, class cache_element>
uint64_t LRU_Cache<cache_key, cache_element>::get_time()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_sec << 32) + tv.tv_usec;
}

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::Init(uint16_t size)//thrown
{
	m_max_size = size;
}

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::put(const cache_key & key, const cache_element & value)
{
	if (m_hash_table.size() >= m_max_size)
	{
		typename time_queue::right_iterator time_queue_iter_to_element = m_time_queue.right.find(key);
		//обновляемся если есть элемент с таким ключом
		if (time_queue_iter_to_element != m_time_queue.right.end())
		{
			m_hash_table[key] = value;
			m_time_queue.right.replace_data(time_queue_iter_to_element, get_time());
			return;
		}
		//находим самый старый элемент
		time_queue_left_iterator time_queue_iter_to_old_element = m_time_queue.left.begin();
		//берем его ключ
		cache_key key_of_old_element = time_queue_iter_to_old_element->second;
		//удаляем из таблицы и очереди элемент
		m_hash_table.erase(key_of_old_element);
		m_time_queue.left.erase(time_queue_iter_to_old_element);
		//в итоге есть свободный элемент, его и займем
		return put(key, value);
	}
	else
	{
		m_hash_table[key] = value;
		//ищем справа данный свободный элемент(key)
		typename time_queue::right_iterator time_queue_iter_to_element = m_time_queue.right.find(key);
		//если его нет, добавляем
		if (time_queue_iter_to_element == m_time_queue.right.end())
			m_time_queue.insert(time_queue_value_type(get_time(), key));
		else
			//если он есть в time_queue, меняем слева время на текущее
			m_time_queue.right.replace_data(time_queue_iter_to_element, get_time());
	}
}

/*
 * Exception::ERR_CODE_LRU_CACHE_NE
 */

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::get(const cache_key & key, cache_element & value)
{
	get_without_time_update(key, value);
	update_element_time(key);
}

template <class cache_key, class cache_element>
bool LRU_Cache<cache_key, cache_element>::exists(const cache_key & key)
{
	hash_table_iterator iter = m_hash_table.find(key);
	return iter != m_hash_table.end();
}

/*
 * Exception::ERR_CODE_LRU_CACHE_NE
 */

template <class cache_key, class cache_element>
cache_element & LRU_Cache<cache_key, cache_element>::operator[](const cache_key & key)
{
	update_element_time(key);
	return m_hash_table[key];
}

/*
 * Exception::ERR_CODE_LRU_CACHE_NE
 */

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::get_without_time_update(const cache_key & key, cache_element & value)
{
	hash_table_iterator iter = m_hash_table.find(key);
	if (iter == m_hash_table.end())
		throw Exception(Exception::ERR_CODE_LRU_CACHE_NE);
	value = iter->second;
}

/*
 * Exception::ERR_CODE_LRU_CACHE_NE
 */

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::get_element_time(const cache_key & key, timeval & t)
{
	typename time_queue::right_iterator it = m_time_queue.right.find(key);
	if (it == m_time_queue.right.end())
		throw Exception(Exception::ERR_CODE_LRU_CACHE_NE);
	uint64_t time = it->second;
	t.tv_usec = time & (uint32_t)0xFFFFFFFF;
	t.tv_sec = (time - t.tv_usec) >> 32;
}

/*
 * Exception::ERR_CODE_LRU_CACHE_NE
 */

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::update_element_time(const cache_key & key)
{
	typename time_queue::right_iterator it = m_time_queue.right.find(key);
	if (it == m_time_queue.right.end())
		throw Exception(Exception::ERR_CODE_LRU_CACHE_NE);
	m_time_queue.right.replace_data(it, get_time());
}

template <class cache_key, class cache_element>
const cache_key & LRU_Cache<cache_key, cache_element>::get_old()
{
	time_queue_left_iterator time_queue_iter_to_old_element = m_time_queue.left.begin();
	return time_queue_iter_to_old_element->second;
}

template <class cache_key, class cache_element>
bool LRU_Cache<cache_key, cache_element>::empty()
{
	return m_hash_table.empty();
}

template <class cache_key, class cache_element>
size_t LRU_Cache<cache_key, cache_element>::size()
{
	return m_hash_table.size();
}

template <class cache_key, class cache_element>
void LRU_Cache<cache_key, cache_element>::remove(const cache_key & key)
{
	m_hash_table.erase(key);
	m_time_queue.right.erase(key);
}

}
}
