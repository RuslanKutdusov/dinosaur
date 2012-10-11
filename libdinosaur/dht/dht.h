/*
 * dht.h
 *
 *  Created on: 28.08.2012
 *      Author: ruslan
 */

#ifndef DHT_H_
#define DHT_H_

#include "../consts.h"
#include "../types.h"
#include "../network/network.h"
#include "../utils/bencode.h"
#include "../utils/sha1.h"
#include "../lru_cache/lru_cache.h"
#include <map>
#include <set>
#include <deque>
#include <boost/shared_ptr.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/map.hpp>

namespace dinosaur
{
namespace dht
{

#define NODE_ID_HEX_LENGTH (NODE_ID_LENGTH * 2 + 1)
#define TRANSACTION_ID_LENGTH 2
#define PING_REQUEST_LENGTH 56
#define FIND_NODE_REQUEST_LENGTH 92
#define GET_PEERS_REQUEST_LENGTH 96
#define COMPACT_NODE_INFO_LENGTH 26

#define MESSAGE_TYPE_QUERY 'q'
#define MESSAGE_TYPE_REPLY 'r'
#define MESSAGE_TYPE_ERROR 'e'

#define ERROR_MESSAGE_LEN 2
#define ERROR_MESSAGE_ERR_CODE_INDEX 0
#define ERROR_MESSAGE_ERR_DESC_INDEX 1

#define BUCKET_K 8
#define BUCKETS_COUNT 160
#define BUCKET_UPDATE_PERIOD_SECS 900//15min

#define NODE_PING_TIMEOUT_SECS 10

typedef uint16_t	TRANSACTION_ID;

struct TOKEN
{
	char* 	token;
	size_t 	length;
};

class node_id : public SHA1_HASH
{
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		for(size_t i = 0; i < NODE_ID_LENGTH; i++)
			ar & m_data[i];
	}
public:
	node_id();
	node_id(const node_id & id);
	node_id(const char * id);
	node_id(const unsigned char * id);
	virtual ~node_id();
	const node_id operator ^(const node_id & id);
};
typedef std::list<node_id> node_list;

node_id distance(const node_id & id1, const node_id & id2);
node_id generate_random_node_id();
void generate_random_node_id(node_id & id);
size_t get_bucket(const node_id & id1, const node_id & id2);
void parse_compact_node_info(char * buf, node_id & id, sockaddr_in & addr);

class message_sender;
class routing_table;
typedef boost::shared_ptr<routing_table> routing_tablePtr;

class routing_table
{
private:
	struct node_info
	{
		node_id 	id;
		sockaddr_in addr;
	};
	struct replacement
	{
		node_info 	replacement_;
		int			ping_at;
		bool		do_replace;
	};
	struct sockaddr_in_serializable : public sockaddr_in
	{
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & this->sin_addr.s_addr;
			ar & this->sin_family;
			ar & this->sin_port;
		}
	};
	typedef lru_cache::LRU_Cache<node_id, sockaddr_in> bucket;
	typedef lru_cache::LRU_Cache<size_t, char > buckets_age;//char для экономии памяти, значение по ключи не используется, используется только ключ
	typedef std::map<node_id, replacement> pings;
	typedef std::map<node_id, sockaddr_in> queue;
	typedef std::pair<node_id, sockaddr_in_serializable> pair_serializable;
	typedef std::map<node_id, sockaddr_in_serializable> map_serializable;
	bucket*					m_buckets;
	buckets_age 			m_buckets_age;
	char					m_value_for_all_buckets_age;
	queue					m_queue4adding_nodes;
	pings					m_pings;
	node_id					m_own_node_id;
	message_sender*			m_ms;
	map_serializable		m_map4serialize;
	std::string				m_serialize_filepath;
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & m_own_node_id;
		ar & m_map4serialize;
	}
	uint64_t get_time();
	void update_bucket(size_t bucket);
	void replace(const node_id & old, const node_id & new_, const sockaddr_in & addr);
	void delete_node(const node_id & id);
	void add_node_(const node_id & id, const sockaddr_in & addr);
	void restore();
	void check_old_bucket(size_t old_bucket);
	routing_table(){}
	routing_table(const routing_table & rt){}
	routing_table & operator=(const routing_table & rt){ return *this;}
	routing_table(message_sender * ms, const std::string & work_dir);
public:
	static void CreateRoutingTable(message_sender * ms, const std::string & work_dir, routing_tablePtr & ptr)
	{
		ptr.reset(new routing_table(ms, work_dir));
		ptr->restore();
	}
	~routing_table();
	void add_node(const node_id & id, const sockaddr_in & addr);
	void clock();
	void update_node(const node_id & from);
	sockaddr_in & operator[](const node_id & node);
	bool node_exists(const node_id & node);
	void save();
	const node_id & get_node_id();
	void get_closest_ids(const node_id & close2, node_list & nodes);
};

class searcher//SEARCHING!!! SEEK AND DESTROY!
{
private:
	routing_tablePtr		m_rt;
	message_sender*			m_ms;
	SHA1_HASH				m_infohash;
	std::deque<node_list>	m_fronts;
	searcher(){}
	searcher(const searcher & s){}
	searcher & operator=(const searcher & s){return *this;}
public:
	searcher(const SHA1_HASH & infohash, routing_tablePtr & rt, message_sender * ms);
	virtual ~searcher();
};

class message_sender
{
public:
	message_sender(){}
	virtual ~message_sender(){}
private:
	virtual void send_ping(const sockaddr_in & addr) = 0;
	virtual void find_node(const node_id & recipient, const node_id & target) = 0;
	virtual void find_node(const sockaddr_in & recipient, const node_id & target) = 0;
	virtual void get_peers(const node_id & recipient, const SHA1_HASH & infohash) = 0;
	virtual void get_peers(const sockaddr_in & recipient, const SHA1_HASH & infohash) = 0;
	virtual void announce_peer(const node_id & recipient, const SHA1_HASH & infohash, uint16_t port, const TOKEN & token) = 0;
	virtual void announce_peer(const sockaddr_in & recipient, const SHA1_HASH & infohash, uint16_t port, const TOKEN & token) = 0;
	friend class searcher;
	friend class routing_table;
};

class dht;
typedef boost::shared_ptr<dht> dhtPtr;

class dht : public network::SocketAssociation, message_sender
{
private:
	enum REQUEST_TYPE
	{
		REQUEST_TYPE_PING,
		REQUEST_TYPE_FIND_NODE,
		REQUEST_TYPE_GET_PEERS,
		REQUEST_TYPE_ANNOUNCE_PEER
	};
	struct request
	{
		REQUEST_TYPE 	type;
		TRANSACTION_ID	transaction_id;
	};
	typedef std::pair<std::string, uint16_t> ip_port;
	typedef std::map<ip_port, std::map<TRANSACTION_ID, REQUEST_TYPE> > request_container;
	typedef std::map<node_id, searcher> searcher_container;
	request_container						m_requests;
	network::NetworkManager*				m_nm;
	network::Socket							m_sock;
	socket_status							m_sock_status;
	routing_tablePtr						m_rt;
	searcher_container						m_searcher_contatiner;
	dht(){}
	dht(const dht & dht){}
	dht & operator=(const dht & dht){ return *this;}
	dht(network::NetworkManager* nm, const std::string & work_dir);
	ip_port sockaddr2ip_port(const sockaddr_in & addr);
	void response_handler(bencode::be_node * message_bencoded, REQUEST_TYPE request_type, const sockaddr_in & addr);
	void ping_handler(const node_id & id, const sockaddr_in & addr);
	void find_node_handler(bencode::be_node * r);
	void get_peers_handler();
	void announce_peer_handler();
	void error_handler(bencode::be_node * node);
	void event_sock_ready2read(network::Socket sock);
	void event_sock_error(network::Socket sock, int errno_);
	void event_sock_sended(network::Socket sock);
	void event_sock_connected(network::Socket sock);
	void event_sock_accepted(network::Socket sock, network::Socket accepted_sock);
	void event_sock_timeout(network::Socket sock);
	void event_sock_unresolved(network::Socket sock);
public:
	void send_ping(const sockaddr_in & addr);
	void find_node(const node_id & recipient, const node_id & target);
	void find_node(const sockaddr_in & recipient, const node_id & target);
	void get_peers(const node_id & recipient, const SHA1_HASH & infohash);
	void get_peers(const sockaddr_in & recipient, const SHA1_HASH & infohash);
	void announce_peer(const node_id & recipient, const SHA1_HASH & infohash, uint16_t port, const TOKEN & token);
	void announce_peer(const sockaddr_in & recipient, const SHA1_HASH & infohash, uint16_t port, const TOKEN & token);
	static void CreateDHT(const in_addr & listen_on, uint16_t port, network::NetworkManager* nm, const std::string & work_dir, dhtPtr & ptr)
	{
		if (nm == NULL)
			throw Exception(Exception::ERR_CODE_NULL_REF);
		ptr.reset(new dht(nm, work_dir));
		if (ptr == NULL)
			throw Exception(Exception::ERR_CODE_UNDEF);
		ptr->InitSocket(listen_on, port);
		routing_table::CreateRoutingTable(ptr.get(), work_dir, ptr->m_rt);
	}
	virtual ~dht();
	void InitSocket(const in_addr & listen_on, uint16_t port);
	const socket_status & get_socket_status();
	void prepare2release();
	void add_node(const sockaddr_in & addr);
	void clock();
};

}
}


#endif /* DHT_H_ */
