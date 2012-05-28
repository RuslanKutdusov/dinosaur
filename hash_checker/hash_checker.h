/*
 * hash_checker.h
 *
 *  Created on: 03.04.2012
 *      Author: ruslan
 */

#pragma once
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "../consts.h"
#include "../fs/fs.h"
#include "../utils/sha1.h"

namespace HashChecker
{
	class HashChecker_event
	{
	public:
		HashChecker_event(){};
		virtual ~HashChecker_event(){};
		virtual int event_piece_hash(uint32_t piece_index, bool ok, bool error) = 0;
	};

	struct element
	{
		unsigned char sha1[20];
		int * files;//список файлов, где содержится кусок
		int files_count;
		uint64_t offset;//смещение в первом файле до начала куска
		uint32_t piece;
		uint32_t piece_length;
		HashChecker_event * hc_e;
	};

	struct event_on
	{
		HashChecker_event * hc_e;
		uint32_t piece_index;
		bool ok;
		bool error;
	};

	class HashChecker
	{
	private:
		struct _queue
		{
			element e;
			_queue * last;
			_queue * next;
		};
		_queue * m_head;
		uint16_t m_count;//кол-во хэшей на проверку
		uint16_t m_size;//размер
		_queue * m_back;//ссылка на первый свободный элемент
		_queue * m_front;
		fs::FileManager * m_fm;
		std::list<event_on> m_event;
		pthread_t m_thread;
		pthread_mutex_t m_mutex;
		pthread_cond_t m_cond;
		bool m_thread_stop;
		element * front();
		void pop();
		bool empty();
		static void * thread(void * arg)
			{
				int ret = 1;
				/*HashChecker * hc = (HashChecker*)arg;
				CSHA1 csha1;
				char * piece = new char[10485760];
				while(!hc->m_thread_stop)
				{
					pthread_mutex_lock(&hc->m_mutex);
					if (!hc->empty())
					{
						element e;
						event_on eo;
						memcpy(&e, hc->front(), sizeof(element));
						e.files = new int [e.files_count];
						memcpy(e.files, hc->front()->files, sizeof(int) * e.files_count);
						hc->pop();
						pthread_mutex_unlock(&hc->m_mutex);
						eo.hc_e = e.hc_e;
						eo.piece_index = e.piece;

						int file_index =  0;
						uint32_t to_read = e.piece_length;
						uint32_t pos = 0;
						while(pos < e.piece_length)
						{
							int r = hc->m_fm->File_read_immediately(e.files[file_index++], &piece[pos], e.offset, to_read);
							if (r == -1)
							{
								eo.ok = false;
								eo.error = true;
								pthread_mutex_lock(&hc->m_mutex);
								hc->m_event.push_back(eo);
								pthread_mutex_unlock(&hc->m_mutex);
								delete[] e.files;
								break;
							}
							pos += r;
							to_read -= r;
							e.offset = 0;
						}
						unsigned char sha1[20];
						memset(sha1, 0, 20);
						csha1.Update((unsigned char*)piece,e.piece_length);
						csha1.Final();
						csha1.GetHash(sha1);
						csha1.Reset();
						eo.error = false;
						eo.ok =  memcmp(sha1,e.sha1, 20) == 0;
						pthread_mutex_lock(&hc->m_mutex);
						hc->m_event.push_back(eo);
						pthread_mutex_unlock(&hc->m_mutex);
						delete[] e.files;
						usleep(50);
					}
					else
						pthread_mutex_unlock(&hc->m_mutex);
					usleep(50);
				}
				delete[] piece;*/
				return (void*)ret;
			}
	public:
		int Init(uint16_t size, fs::FileManager * fm);
		int push(int * files, int files_count, unsigned char * sha1, uint64_t offset, uint32_t piece, uint32_t piece_length, HashChecker_event * hc_e);
		bool get_event(event_on * eo);
		HashChecker();
		~HashChecker();
	};
}
