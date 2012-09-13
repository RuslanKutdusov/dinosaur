/*
 * Blockcache.h
 *
 *  Created on: 17.04.2012
 *      Author: ruslan
 */

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <queue>
#include <list>
#include <map>
#include <boost/bimap.hpp>
#include "../consts.h"
#include "../err/err_code.h"
#include "../lru_cache/lru_cache.h"
#include "../utils/utils.h"

namespace dinosaur{
namespace block_cache {

//то что ссылка нормально, всего 4 байт вместо 60, если ставить хэш, ссылкой пользуемся лишь для сравнения
typedef std::pair<void*, BLOCK_ID> cache_key;//(reference to Torrent, piece+block)

class cache_element
{
private:
	char block[BLOCK_LENGTH];
public:
	cache_element()
	{

	}
	cache_element(const cache_element & ce)
	{
		memcpy(block, ce.block, BLOCK_LENGTH);
	}
	cache_element(const char * buf)
	{
		memcpy(block, buf, BLOCK_LENGTH);
	}
	cache_element(const unsigned char * buf)
	{
		memcpy(block, buf, BLOCK_LENGTH);
	}
	cache_element & operator=(const cache_element & ce)
	{
		if (this != &ce)
			memcpy(block, ce.block, BLOCK_LENGTH);
		return *this;
	}
	cache_element & operator=(const char * buf)
	{
		if (this != NULL)
			memcpy(block, buf, BLOCK_LENGTH);
		return *this;
	}
	cache_element & operator=(const unsigned char * buf)
	{
		if (this != NULL)
			memcpy(block, buf, BLOCK_LENGTH);
		return *this;
	}
	void copy2(char * buf)
	{
		memcpy(buf, block, BLOCK_LENGTH);
	}
};

class Block_cache : public lru_cache::LRU_Cache<cache_key, cache_element>
{
private:
	Block_cache(const Block_cache & bc){}
	Block_cache & operator=(const Block_cache &bc){return *this;}
public:
	Block_cache(){}
	virtual ~Block_cache(){}
	void get(const cache_key & key, char * buf)
	{
		hash_table_iterator iter = m_hash_table.find(key);
		if (iter == m_hash_table.end())
			throw Exception(Exception::ERR_CODE_LRU_CACHE_NE);
		iter->second.copy2(buf);
		//ищем справа данный элемент(key)
		typename time_queue::right_iterator it = m_time_queue.right.find(key);
		//если его нет, добавляем
		if (it == m_time_queue.right.end() && m_time_queue.right.count(key) == 0)
			m_time_queue.insert(time_queue_value_type(get_time(), key));
		else
			//если он есть в time_queue, меняем слева время на текущее
			m_time_queue.right.replace_data(it, get_time());
	}
};

} /* namespace Bittorrent */
}
