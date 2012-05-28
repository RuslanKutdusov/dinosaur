/*
 * hash_checker.cpp
 *
 *  Created on: 03.04.2012
 *      Author: ruslan
 */


#include "hash_checker.h"


namespace HashChecker
{

HashChecker::HashChecker()
{
	m_thread = 0;
	m_head = NULL;
}

HashChecker::~HashChecker()
{
	printf("HashChecker destructor\n");
	if (m_thread != 0)
	{
		m_thread_stop = true;
		void * status;
		pthread_join(m_thread, &status);
		pthread_mutex_destroy(&m_mutex);
	}
	if (m_head != NULL)
	{
		_queue * q = m_head;
		q->last->next = NULL;
		while(q != NULL)
		{
			_queue * next = q->next;
			if (q->e.files != NULL)
				delete[] q->e.files;
			delete q;
			q = next;
		}
	}
}

int HashChecker::Init(uint16_t size, fs::FileManager * fm)
{
	m_head = NULL;
	m_back = NULL;
	m_front = NULL;
	m_count = 0;
	m_size = 0;
	m_thread_stop = false;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init (&m_cond, NULL);
	//pthread_mutex_init(&m_mutex_timeout_sockets, NULL);
	if (pthread_create(&m_thread, &attr, HashChecker::thread, (void *)this))
		return ERR_SYSCALL_ERROR;
	pthread_attr_destroy(&attr);
	if (size == 0)
		size = 1;
	m_fm = fm;
	m_size = size;
	m_count = 0;
	m_front = NULL;
	m_head = new _queue;
	memset(m_head, 0, sizeof(_queue));
	m_head->last = m_head;
	m_head->next = m_head;
	_queue * last = m_head;
	for(uint16_t i = 1; i < size; i++)
	{
		_queue * q = new _queue;
		memset(q, 0, sizeof(_queue));
		last->next = q;
		q->last = last;
		q->next = m_head;
		m_head->last = q;
		last = q;
	}
	m_back = m_head;
	return ERR_NO_ERROR;
}

element * HashChecker::front()
{
	if (m_count == 0)
		return NULL;
	return &m_front->e;
}

void HashChecker::pop()
{
	if (m_count == 0)
		return;
	if (m_front->e.files != NULL)
	{
		delete[] m_front->e.files;
		m_front->e.files = NULL;
	}
	m_front = m_front->next;
	m_count--;
}

int HashChecker::push(int * files, int files_count, unsigned char * sha1, uint64_t offset, uint32_t piece,uint32_t piece_length, HashChecker_event * hc_e)
{
	pthread_mutex_lock(&m_mutex);
	if (files == NULL || sha1 == NULL || hc_e == NULL)
		return ERR_BAD_ARG;
	if (m_count >= m_size)
	{
		printf("NO AVAILABLE SPACE IN HASH_CHECKER\n");
		pthread_mutex_unlock(&m_mutex);
		return ERR_INTERNAL;
	}
	if (m_count == 0)
		m_front = m_back;
	m_back->e.files_count = files_count;
	m_back->e.offset = offset;
	m_back->e.piece = piece;
	m_back->e.piece_length = piece_length;
	m_back->e.hc_e = hc_e;
	m_back->e.files = new int[files_count];
	memcpy(m_back->e.files, files, sizeof(int) * files_count);
	memcpy(m_back->e.sha1, sha1, 20);
	m_back = m_back->next;
	m_count++;
	//printf("pushed\n");
	//pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

bool HashChecker::empty()
{
	return m_count == 0;
}

bool HashChecker::get_event(event_on * eo)
{
	pthread_mutex_lock(&m_mutex);
	if (m_event.size() == 0)
	{
		pthread_mutex_unlock(&m_mutex);
		return false;
	}
	memcpy(eo, &m_event.front(), sizeof(event_on));
	m_event.pop_front();
	pthread_mutex_unlock(&m_mutex);
	return true;
}

}
