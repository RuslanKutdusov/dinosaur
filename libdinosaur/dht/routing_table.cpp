/*
 * routing_table.cpp
 *
 *  Created on: 09.09.2012
 *      Author: ruslan
 */

#include "dht.h"

namespace dinosaur{
namespace dht{


uint64_t routing_table::get_time()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_sec << 32) + tv.tv_usec;
}

void routing_table::update_bucket(size_t bucket)
{
	m_buckets_age.update_element_time(bucket);
}

void routing_table::replace(const node_id & old, const node_id & new_, const sockaddr_in & addr)
{
	size_t bucket = get_bucket(old, m_own_node_id);
#ifdef DHT_DEBUG
	printf("Replace\n	");
	old.print();
	printf("	");
	new_.print();
#endif
	m_buckets[bucket].remove(old);
	if (m_buckets[bucket].size() < BUCKET_K)
		m_buckets[bucket].put(new_, addr);
	update_bucket(bucket);
#ifdef DHT_DEBUG
	printf("	nodes in the bucket:\n");
	for(bucket::iterator iter = m_buckets[bucket].begin(); iter != m_buckets[bucket].end(); ++iter)
	{
		printf("	%llu ", iter->first);iter->second.print();
	}
#endif
}

void routing_table::delete_node(const node_id & id)
{
	size_t bucket = get_bucket(id, m_own_node_id);
	m_buckets[bucket].remove(id);
	if (m_buckets[bucket].size() == 0)
		update_bucket(bucket);
}

void routing_table::add_node_(const node_id & id, const sockaddr_in & addr)
{
	size_t bucket = get_bucket(id, m_own_node_id);
#ifdef DHT_DEBUG
	printf("	bucket=%u\n", bucket);
#endif
	try
	{
		m_buckets[bucket].update_element_time(id);
		update_bucket(bucket);
#ifdef DHT_DEBUG
		printf("	node already exists\n");
#endif
		return;
	}
	catch(Exception & e)
	{
		if (e.get_errcode() != Exception::ERR_CODE_LRU_CACHE_NE)
			return;
	}
	if (m_buckets[bucket].size() < BUCKET_K)
	{
		m_buckets[bucket].put(id, addr);
		update_bucket(bucket);
#ifdef DHT_DEBUG
		printf("	added, nodes in the bucket=%u\n", m_buckets[bucket].size());
#endif
		return;
	}
	node_id old = m_buckets[bucket].get_old();
#ifdef DHT_DEBUG
	printf("	no place in the bucket, found old node id="); old.print();
#endif
	pings::iterator iter2old = m_pings.find(old);
	if (iter2old == m_pings.end())
	{
		m_pings[old].ping_at = time(NULL);
		m_pings[old].replacement_.id = id;
		m_pings[old].replacement_.addr = addr;
		m_pings[old].do_replace = true;
		m_psi->send_ping(m_buckets[bucket][old]);
#ifdef DHT_DEBUG
		printf("	old node push to m_pings and we send ping to him\n");
#endif
		return;
	}
#ifdef DHT_DEBUG
	printf("	node pushed in to the queue\n");
#endif
	m_queue4adding_nodes[id] = addr;
}

routing_table::routing_table(const node_id & own_node_id, ping_sender_interface * psi)
{
	m_own_node_id = own_node_id;
	m_psi = psi;
	m_buckets_age.Init(BUCKETS_COUNT);
	m_buckets = new bucket[BUCKETS_COUNT];
	for(size_t i = 0; i < BUCKETS_COUNT; i++)
	{
		m_buckets[i].Init(BUCKET_K);
		m_buckets_age.put(i, m_value_for_all_buckets_age);
	}
}

routing_table::~routing_table()
{
	if (m_buckets != NULL)
		delete[] m_buckets;
}

void routing_table::add_node(const node_id & id, const sockaddr_in & addr)
{
#ifdef DHT_DEBUG
	printf("Adding node id=	"); id.print();
#endif
	if (m_queue4adding_nodes.count(id) != 0)
	{
#ifdef DHT_DEBUG
		printf("	abort, already exists in the queue\n");
#endif
		return;
	}
	add_node_(id, addr);
}

void routing_table::clock()
{
	for(pings::iterator iter = m_pings.begin(), iter2 = iter; iter != m_pings.end(); iter = iter2)
	{
		++iter2;
		if (time(NULL) - iter->second.ping_at > NODE_PING_TIMEOUT_SECS)
		{
			if (iter->second.do_replace)
				replace(iter->first, iter->second.replacement_.id, iter->second.replacement_.addr);
			else
				delete_node(iter->first);
			m_pings.erase(iter);
		}
	}

	for(queue::iterator iter = m_queue4adding_nodes.begin(), iter2 = iter; iter != m_queue4adding_nodes.end(); iter = iter2)
	{
		++iter2;
		node_id id = iter->first;
		sockaddr_in addr = iter->second;
		m_queue4adding_nodes.erase(iter);
#ifdef DHT_DEBUG
	printf("Adding node id=	"); id.print();
#endif
		add_node_(id, addr);
	}

	size_t old_bucket = m_buckets_age.get_old();
	timeval tv;
	m_buckets_age.get_element_time(old_bucket, tv);
	if (time(NULL) - tv.tv_sec > BUCKET_UPDATE_PERIOD_SECS)
	{
#ifdef DHT_DEBUG
		printf("Old bucket found %u\n", old_bucket);
#endif
		for(bucket::iterator iter = m_buckets[old_bucket].begin(); iter != m_buckets[old_bucket].end(); ++iter)
		{
			node_id current_node = iter->second;
			pings::iterator iter2old = m_pings.find(current_node);
			if (iter2old != m_pings.end())
				return;

			m_pings[current_node].ping_at = time(NULL);
			m_pings[current_node].do_replace = false;
			m_psi->send_ping(m_buckets[old_bucket][current_node]);
#ifdef DHT_DEBUG
			printf("	old node push to m_pings and we send ping to him\n");
#endif
		}
	}
}

void routing_table::update_node(const node_id & from)
{
	try
	{
		m_pings.erase(from);
		size_t bucket = get_bucket(from, m_own_node_id);
		m_buckets[bucket].update_element_time(from);
		update_bucket(bucket);
	}
	catch(Exception & e)
	{
		return;
	}
}

sockaddr_in & routing_table::operator[](const node_id & node)
{
	size_t bucket = get_bucket(node, m_own_node_id);
	try
	{
		return m_buckets[bucket][node];
	}
	catch(Exception & e)
	{
		throw Exception(Exception::ERR_CODE_NODE_NOT_EXISTS);
	}
}

}
}


