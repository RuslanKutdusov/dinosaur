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

namespace block_cache {

typedef std::pair<void*, uint64_t> cache_key;

struct cache_element
{
	char block[BLOCK_LENGTH];
};

class Block_cache {
private:
	typedef std::map<cache_key, cache_element *> hash_table;
	typedef hash_table::iterator hash_table_iterator;

	typedef boost::bimap<uint64_t, cache_key> time_queue;
	typedef time_queue::iterator time_queue_iterator;
	typedef time_queue::left_map::iterator time_queue_left_iterator;
	typedef time_queue::value_type time_queue_value_type;

	//typedef std::map<uint64_t, cache_key> time_queue;
	std::list<cache_element *> m_free_elements;
	hash_table m_hash_table;
	time_queue m_time_queue;
	cache_element * m_elements;
	uint16_t m_elements_count;
	uint64_t get_time();
public:
	Block_cache();
	~Block_cache();
	int Init(uint16_t size);
	int put(void * torrent, uint64_t block_id, char * block);
	int get(void * torrent, uint64_t block_id, char * block);
	/*void dump_elements()
	{
		printf("DUMP m_elements\n");
		for(uint16_t i = 0; i < m_elements_count; i++)
			printf("Element %u %X\n", i, &m_elements[i]);
		printf("==============================================\n");
	}
	void dump_hash_table()
	{
		printf("DUMP m_hash_table\n");
		for(hash_table_iterator iter = m_hash_table.begin(); iter != m_hash_table.end(); ++iter)
			printf("key=(%X,%llu), value=%X\n", (*iter).first.first, (*iter).first.second, (*iter).second);
		printf("==============================================\n");
	}
	void dump_time_queue()
	{
		printf("DUMP m_time_queue\n");
		for( time_queue_iterator iter = m_time_queue.begin(), iend = m_time_queue.end();
			        iter != iend; ++iter )
		{
			// iter->left  : data : int
			// iter->right : data : std::string
			printf("time = %llu key=(%X,%llu)\n", iter->left, iter->right.first, iter->right.second);
		}
		printf("==============================================\n");
	}
	void dump_all()
	{
		dump_hash_table();
		dump_time_queue();
	}*/
};

} /* namespace Bittorrent */
