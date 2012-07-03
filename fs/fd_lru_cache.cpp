/*
 * fd_lru_cache.cpp
 *
 *  Created on: 02.06.2012
 *      Author: ruslan
 */


#include "fs.h"

namespace fs
{

/*FD_LRU_Cache::FD_LRU_Cache()
	: lru_cache::LRU_Cache<int, int>()
{
	Init(512);
}*/

int FD_LRU_Cache::put(File & file, int file_desc, File & deleted_file)
{
	if (file_desc < 0)
		return ERR_BAD_ARG;
	try
	{
		if (m_free_elements.empty())
		{
			typename time_queue::right_iterator time_queue_iter_to_element = m_time_queue.right.find(file);
			//обновляемся если есть элемент с таким ключом
			if (time_queue_iter_to_element != m_time_queue.right.end() || m_time_queue.right.count(file) != 0)
			{
				memcpy(m_hash_table[file], &file_desc, sizeof(int));
				//m_hash_table[file_id] = file_desc;
				m_time_queue.right.replace_data(time_queue_iter_to_element, get_time());
				return ERR_NO_ERROR;
			}
			//находим самый старый элемент
			time_queue_left_iterator time_queue_iter_to_old_element = m_time_queue.left.begin();
			//берем его ключ
			File old_file = time_queue_iter_to_old_element->second;
			if (deleted_file != NULL)
				deleted_file = old_file;
			//находим ссылку на элемент
			hash_table_iterator hash_table_iter_to_old_element = m_hash_table.find(old_file);
			//добавляем ссылку в free_elements
			m_free_elements.push_back((*hash_table_iter_to_old_element).second);
			//удаляем из таблицы и очереди элемент
			m_hash_table.erase(hash_table_iter_to_old_element);
			m_time_queue.left.erase(time_queue_iter_to_old_element);
			//в итоге в free_elements есть свободный элемент, его и займем
			return put(file, file_desc, deleted_file);
		}
		else
		{
			int * free_element = m_free_elements.front();
			memcpy(free_element, &file_desc, sizeof(int));
			//cache_key key(torrent, block_id);
			m_hash_table[file] = free_element;
			m_free_elements.pop_front();
			//ищем справа данный свободный элемент(key)
			typename time_queue::right_iterator time_queue_iter_to_element = m_time_queue.right.find(file);
			//если его нет, добавляем
			if (time_queue_iter_to_element == m_time_queue.right.end() && m_time_queue.right.count(file) == 0)
				m_time_queue.insert(time_queue_value_type(get_time(), file));
			else
				//если он есть в time_queue, меняем слева время на текущее
				m_time_queue.right.replace_data(time_queue_iter_to_element, get_time());
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

void FD_LRU_Cache::dump()
{
	/*printf("HASHTABLE:\n");
	for (hash_table_iterator iter = m_hash_table.begin(); iter != m_hash_table.end(); ++iter)
	{
		printf("  key=%d value=%d\n",(*iter).first, *(*iter).second);
	}
	printf("TIME QUEUE:\n");
	for(typename time_queue::left_iterator iter = m_time_queue.left.begin(); iter != m_time_queue.left.end(); ++iter)
	{
		printf("  key=%d value=%llu\n", (*iter).second, (*iter).first);
	}*/
}



}
