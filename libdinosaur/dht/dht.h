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
#include <map>
#include <set>
#include <boost/shared_ptr.hpp>

namespace dinosaur
{
namespace dht
{

class node_id
{
private:
	unsigned char m_data[NODE_ID_LENGHT];
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
	bool operator<(const node_id & id);
	bool operator==(const node_id & id);
	bool operator!=(const node_id & id);
	void print();
};

node_id distance(const node_id & id1, const node_id & id2);
node_id generate_random_node_id();
void generate_random_node_id(node_id & id);
size_t get_bucket(const node_id & id1, const node_id & id2);

class dht;
typedef boost::shared_ptr<dht> dhtPtr;

class dht : public network::SocketAssociation
{
private:
	enum REQUEST
	{
		REQUEST_PING,
		REQUEST_FIND_NODE,
		REQUEST_GET_PEERS,
		REQUEST_ANNOUNCE_PEER
	};
	std::map<node_id, std::set<uint16_t> > 	m_requests;
	node_id									m_node_id;
	network::NetworkManager*				m_nm;
	network::Socket							m_sock;
	socket_status							m_sock_status;
	dht(){}
	dht(const dht & dht){}
	dht & operator=(const dht & dht){ return *this;}
	dht(network::NetworkManager* nm, const node_id & our_id);
public:
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
