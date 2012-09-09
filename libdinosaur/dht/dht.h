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
#include <boost/shared_ptr.hpp>

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

struct TOKEN
{
	char* 	token;
	size_t 	length;
};

typedef char node_id_hex[NODE_ID_HEX_LENGTH];
class node_id
{
private:
	unsigned char m_data[NODE_ID_LENGTH];
	void clear();
public:
	node_id();
	node_id(const node_id & id);
	node_id(const char * id);
	virtual ~node_id();
	node_id & operator=(const node_id & id);
	node_id & operator=(const char * id);
	unsigned char & operator[](size_t i);
	const unsigned char & operator[](size_t i) const;
	const node_id operator ^(const node_id & id);
	bool operator<(const node_id & id) const;
	bool operator==(const node_id & id) const;
	bool operator!=(const node_id & id) const;
	void copy2(char * dst) const;
	void copy2(unsigned char * dst) const;
	void print() const;
	void to_hex(node_id_hex id) const ;
};

node_id distance(const node_id & id1, const node_id & id2);
node_id generate_random_node_id();
void generate_random_node_id(node_id & id);
size_t get_bucket(const node_id & id1, const node_id & id2);
void parse_compact_node_info(char * buf, node_id & id, sockaddr_in & addr);

class ping_sender_interface;

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
	typedef lru_cache::LRU_Cache<node_id, sockaddr_in> bucket;
	typedef lru_cache::LRU_Cache<size_t, char > buckets_age;//char для экономии памяти, значение по ключи не используется, используется только ключ
	typedef std::map<node_id, replacement> pings;
	typedef std::map<node_id, sockaddr_in> queue;
	bucket*					m_buckets;
	buckets_age 			m_buckets_age;
	char					m_value_for_all_buckets_age;
	queue					m_queue4adding_nodes;
	pings					m_pings;
	node_id					m_own_node_id;
	ping_sender_interface*	m_psi;
	uint64_t get_time();
	void update_bucket(size_t bucket);
	void replace(const node_id & old, const node_id & new_, const sockaddr_in & addr);
	void delete_node(const node_id & id);
	void add_node_(const node_id & id, const sockaddr_in & addr);
public:
	routing_table(const node_id & own_node_id, ping_sender_interface * psi);
	~routing_table();
	void add_node(const node_id & id, const sockaddr_in & addr);
	void clock();
	void update_node(const node_id & from);
	sockaddr_in & operator[](const node_id & node);
};

class ping_sender_interface
{
public:
	ping_sender_interface(){}
	virtual ~ping_sender_interface(){}
private:
	virtual void send_ping(const sockaddr_in & addr) = 0;
	friend class routing_table;
};

class dht;
typedef boost::shared_ptr<dht> dhtPtr;

class dht : public network::SocketAssociation, ping_sender_interface
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
		uint16_t		transaction_id;
	};
	typedef std::pair<std::string, uint16_t> ip_port;
	typedef std::map<ip_port, std::map<uint16_t, REQUEST_TYPE> > request_container;
	request_container						m_requests;
	node_id									m_node_id;
	network::NetworkManager*				m_nm;
	network::Socket							m_sock;
	socket_status							m_sock_status;
	routing_table*							m_rt;
	dht(){}
	dht(const dht & dht){}
	dht & operator=(const dht & dht){ return *this;}
	dht(network::NetworkManager* nm, const node_id & our_id);
	ip_port sockaddr2ip_port(const sockaddr_in & addr);
	void response_handler(bencode::be_node * message_bencoded, REQUEST_TYPE request_type, const sockaddr_in & addr);
	void ping_handler(const node_id & id, const sockaddr_in & addr);
	void find_node_handler(bencode::be_node * r);
	void get_peers_handler();
	void announce_peer_handler();
	void error_handler(bencode::be_node * node);
public:
	void send_ping(const sockaddr_in & addr);
	void find_node(const node_id & recipient, const node_id & target);
	void find_node(const sockaddr_in & recipient, const node_id & target);
	void get_peers(const node_id & recipient, const SHA1_HASH infohash);
	void get_peers(const sockaddr_in & recipient, const SHA1_HASH infohash);
	void announce_peer(const node_id & recipient, const SHA1_HASH infohash, uint16_t port, const TOKEN & token);
	void announce_peer(const sockaddr_in & recipient, const SHA1_HASH infohash, uint16_t port, const TOKEN & token);
	static void CreateDHT(const in_addr & listen_on, uint16_t port, network::NetworkManager* nm, dhtPtr & ptr, const node_id & our_id = generate_random_node_id())
	{
		if (nm == NULL)
			throw Exception(Exception::ERR_CODE_NULL_REF);
		ptr.reset(new dht(nm, our_id));
		if (ptr == NULL)
			throw Exception(Exception::ERR_CODE_UNDEF);
		ptr->InitSocket(listen_on, port);
	}
	virtual ~dht();
	void InitSocket(const in_addr & listen_on, uint16_t port);
	const socket_status & get_socket_status();
	void prepare2release();
	int event_sock_ready2read(network::Socket sock);
	int event_sock_closed(network::Socket sock);
	int event_sock_sended(network::Socket sock);
	int event_sock_connected(network::Socket sock);
	int event_sock_accepted(network::Socket sock, network::Socket accepted_sock);
	int event_sock_timeout(network::Socket sock);
	int event_sock_unresolved(network::Socket sock);
};

}
}


#endif /* DHT_H_ */
