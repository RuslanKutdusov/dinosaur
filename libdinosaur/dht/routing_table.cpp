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
#ifdef DHT_DEBUG
	printf("Update bucket %u\n", bucket);
#endif
	m_buckets_age.update_element_time(bucket);
}

void routing_table::replace(const node_id & old, const node_id & new_, const sockaddr_in & addr)
{
	size_t bucket = get_bucket(old, m_own_node_id);
#ifdef DHT_DEBUG
	printf("Replace\n	");
	printf("	%s\n", old.hex());
	printf("	%s\n", new_.hex());
#endif
	m_buckets[bucket].remove(old);
	if (m_buckets[bucket].size() < BUCKET_K)
		m_buckets[bucket].put(new_, addr);
	update_bucket(bucket);
#ifdef DHT_DEBUG
	dump_bucket(bucket);
#endif
}

void routing_table::delete_node(const node_id & id)
{
#ifdef DHT_DEBUG
	printf("Delete node id = %s\n", id.hex());
#endif
	size_t bucket = get_bucket(id, m_own_node_id);
	m_buckets[bucket].remove(id);
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
	printf("	no place in the bucket, found old node id=%s\n", old.hex());
#endif
	pings::iterator iter2old = m_pings.find(old);
	if (iter2old == m_pings.end())
	{
		replacement r;
		r.ping_at = time(NULL);
		r.replacement_.id = id;
		r.replacement_.addr = addr;
		r.do_replace = true;
		m_pings.insert(std::make_pair(old, r));
		m_ms->send_ping(m_buckets[bucket][old]);
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

routing_table::routing_table(message_sender * psi, const std::string & work_dir)
{
	m_ms = psi;
	m_buckets_age.Init(BUCKETS_COUNT);
	m_buckets = new bucket[BUCKETS_COUNT];
	for(size_t i = 0; i < BUCKETS_COUNT; i++)
	{
		m_buckets[i].Init(BUCKET_K);
		m_buckets_age.put(i, m_value_for_all_buckets_age);
	}
	m_serialize_filepath = work_dir;
	if (m_serialize_filepath[m_serialize_filepath.length() - 1] != '/')
		m_serialize_filepath += "/";
	m_serialize_filepath += "dht_rt";
}

routing_table::~routing_table()
{
	if (m_buckets != NULL)
		delete[] m_buckets;
}

void routing_table::add_node(const node_id & id, const sockaddr_in & addr)
{
#ifdef DHT_DEBUG
	printf("Adding node id = %s\n", id.hex()); 
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
#ifdef DHT_DEBUG
	//printf("routing table clock, queue size=%u\n", m_queue4adding_nodes.size());
#endif
	for(pings::iterator iter = m_pings.begin(), iter2 = iter; iter != m_pings.end(); iter = iter2)
	{
		++iter2;
		if (time(NULL) - iter->second.ping_at > NODE_PING_TIMEOUT_SECS)
		{
#ifdef DHT_DEBUG
			printf("No ping response from node id = %s\n", iter->first.hex());
#endif
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
	printf("Adding node id = %s\n", id.hex()); 
#endif
		add_node_(id, addr);
	}

	size_t old_bucket = m_buckets_age.get_old();
	timeval tv;
	m_buckets_age.get_element_time(old_bucket, tv);
	if (time(NULL) - tv.tv_sec > BUCKET_UPDATE_PERIOD_SECS)
	{
		if (m_buckets[old_bucket].size() == 0)
			update_bucket(old_bucket);
		check_old_bucket(old_bucket);
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

bool routing_table::node_exists(const node_id & node)
{
	size_t bucket = get_bucket(node, m_own_node_id);
	return m_buckets[bucket].exists(node);
}

void routing_table::save()
{
	std::ofstream ofs(m_serialize_filepath.c_str());
	if (ofs.fail())
		return;
	m_map4serialize.clear();
	for(size_t bucket = 0; bucket < BUCKETS_COUNT; bucket++)
		for(bucket::iterator iter = m_buckets[bucket].begin(); iter != m_buckets[bucket].end(); ++iter)
		{
			sockaddr_in_serializable addr;
			addr.sin_family = iter->second.sin_family;
			addr.sin_addr = iter->second.sin_addr;
			addr.sin_port = iter->second.sin_port;
			m_map4serialize[iter->first] = addr;
		}
	try
	{
		boost::archive::text_oarchive oa(ofs);
		oa << *this;
	}
	catch(...)
	{

	}
	ofs.close();
}

void routing_table::restore()
{
	try
	{
		std::ifstream ifs(m_serialize_filepath.c_str());
		if (ifs.fail())
			throw 1;
		m_map4serialize.clear();
		boost::archive::text_iarchive ia(ifs);
		ia >> *this;
		ifs.close();
	}
	catch(...)
	{
		m_own_node_id = generate_random_node_id();
#ifdef DHT_DEBUG
		printf("Can not restore routing table %s\n", m_own_node_id.hex()); 
#endif
		return;
	}
#ifdef DHT_DEBUG
	printf("Restoring routing table...\n");
	printf("	own node id = %s\n", m_own_node_id.hex());
#endif
	for(map_serializable::iterator iter = m_map4serialize.begin(); iter != m_map4serialize.end(); ++iter)
	{
		add_node(iter->first, iter->second);
	}
	for(size_t bucket = 0; bucket < BUCKETS_COUNT; bucket++)
	{
		check_old_bucket(bucket);
	}
}

const node_id & routing_table::get_node_id()
{
	return m_own_node_id;
}

void routing_table::get_closest_ids(const node_id & close2, node_list & nodes)
{
	size_t bucket = get_bucket(close2, m_own_node_id);
	size_t bucket_iter = 0;
	int sign = -1;
	nodes.clear();
	size_t buckets_passed = 0;
	int current_bucket = bucket;
	while(buckets_passed != BUCKETS_COUNT)
	{
		current_bucket +=  bucket_iter * sign;
		bucket_iter++;
		sign = -sign;
		if (current_bucket < 0 || current_bucket > BUCKETS_COUNT - 1)
			continue;
		buckets_passed++;
		for(bucket::iterator iter = m_buckets[current_bucket].begin(); iter != m_buckets[current_bucket].end(); ++iter)
		{
			nodes.push_back(iter->first);
			if (nodes.size() == BUCKET_K)
				return;
		}
	}
}

void routing_table::check_old_bucket(size_t old_bucket)
{
#ifdef DHT_DEBUG
		printf("Old bucket %u\n", old_bucket);
#endif
	for(bucket::iterator iter = m_buckets[old_bucket].begin(); iter != m_buckets[old_bucket].end(); ++iter)
	{
		const node_id & current_node = iter->first;
		pings::iterator iter2old = m_pings.find(current_node);
		if (iter2old != m_pings.end())
			return;

		m_pings[current_node].ping_at = time(NULL);
		m_pings[current_node].do_replace = false;

		m_ms->send_ping(iter->second);
#ifdef DHT_DEBUG
		printf("	old node push to m_pings and we send ping to him\n");
#endif
	}
}

size_t routing_table::bucket_size(const size_t & bucket_index) const
{
	return m_buckets[bucket_index].size();
}

bool routing_table::is_bucket_full(const size_t & bucket_index) const
{
	return m_buckets[bucket_index].size() == BUCKET_K;
}

bool routing_table::is_bucket_old(const size_t & bucket_index) const
{
	timeval tv; 
	m_buckets_age.get_element_time(bucket_index, tv);
	return time(NULL) - tv.tv_sec > BUCKET_UPDATE_PERIOD_SECS;
}

void routing_table::dump_bucket(const size_t & bucket) const
{
	printf("	nodes in the bucket:\n");
	for(bucket::iterator iter = m_buckets[bucket].begin(); iter != m_buckets[bucket].end(); ++iter)
		printf( "	%s\n", iter->first.hex() );
}

}
}


