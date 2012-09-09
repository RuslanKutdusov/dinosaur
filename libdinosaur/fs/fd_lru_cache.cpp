/*
 * fd_lru_cache.cpp
 *
 *  Created on: 02.06.2012
 *      Author: ruslan
 */


#include "fs.h"
namespace dinosaur {
namespace fs
{

/*
 * Exception::ERR_CODE_UNDEF;
 */

int FD_LRU_Cache::put(File & file, int file_desc, File & deleted_file)  throw(Exception)
{
	if (file_desc < 0)
		throw Exception(Exception::ERR_CODE_UNDEF);
	try
	{
		if (m_hash_table.size() >= m_max_size)
		{
			typename time_queue::right_iterator time_queue_iter_to_element = m_time_queue.right.find(file);
			//обновляемся если есть элемент с таким ключом
			if (time_queue_iter_to_element != m_time_queue.right.end())
			{
				m_hash_table[file] = file_desc;
				m_time_queue.right.replace_data(time_queue_iter_to_element, get_time());
				return ERR_NO_ERROR;
			}
			//находим самый старый элемент
			time_queue_left_iterator time_queue_iter_to_old_element = m_time_queue.left.begin();
			//берем его ключ
			deleted_file = time_queue_iter_to_old_element->second;

			//удаляем из таблицы и очереди элемент
			m_hash_table.erase(deleted_file);
			m_time_queue.left.erase(time_queue_iter_to_old_element);
			//в итоге в free_elements есть свободный элемент, его и займем
			return put(file, file_desc, deleted_file);
		}
		else
		{
			m_hash_table[file] = file_desc;
			//ищем справа данный свободный элемент(key)
			typename time_queue::right_iterator time_queue_iter_to_element = m_time_queue.right.find(file);
			//если его нет, добавляем
			if (time_queue_iter_to_element == m_time_queue.right.end())
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
		throw Exception(Exception::ERR_CODE_UNDEF);
	}
	return ERR_NO_ERROR;
}


}
}
