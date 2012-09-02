/*
 * dht.cpp
 *
 *  Created on: 28.08.2012
 *      Author: ruslan
 */

#include "dht.h"

namespace dinosaur
{
namespace dht
{

void parse_compact_node_info(char * buf, node_id & id, sockaddr_in & addr)
{
	id = &buf[0];
	addr.sin_family = AF_INET;
	memcpy(&addr.sin_addr, &buf[20], sizeof(in_addr));
	memcpy(&addr.sin_port, &buf[24], sizeof(uint16_t));
}

dht::dht(network::NetworkManager* nm, const node_id & our_id)
{
#ifdef DHT_DEBUG
	node_id_hex hex;
	our_id.to_hex(hex);
	printf("Creating DHT, our node id=%s\n", hex);
#endif
	m_nm = nm;
	m_node_id = our_id;
}

dht::~dht()
{

}

void dht::InitSocket(const in_addr & listen_on, uint16_t port)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	memcpy(&addr.sin_addr, &listen_on, sizeof(in_addr));
	addr.sin_port = htons(port);
	m_sock_status.listen = false;
	m_sock_status.errno_ = 0;
	m_sock_status.exception_errcode = Exception::NO_ERROR;
	try
	{
		m_nm->Socket_add_UDP(shared_from_this(), m_sock, &addr);
	}
	catch(Exception & e)
	{
		m_sock_status.exception_errcode = e.get_errcode();
		return;
	}
	catch(SyscallException & e)
	{
		m_sock_status.errno_ = e.get_errno();
		return;
	}
	m_sock_status.listen = true;
}

const socket_status & dht::get_socket_status()
{
	return m_sock_status;
}

void dht::prepare2release()
{
	m_nm->Socket_delete(m_sock);
}

void dht::send_ping(const sockaddr_in & addr)
{
	uint16_t transaction_id = rand() % 65536;
	char buf[256] = "d1:ad2:id20:00000000000000000000e1:q4:ping1:t2:001:y1:qe";
	m_node_id.copy2(&buf[12]);
	memcpy(&buf[47], &transaction_id, TRANSACTION_ID_LENGHT);
	try
	{
		#ifdef DHT_DEBUG
				printf("Sending ping to %s:%d transaction=%u...", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), transaction_id);
		#endif
		m_nm->Socket_send(m_sock, buf, PING_REQUEST_LENGHT, addr);
		m_requests[ip_port(inet_ntoa(addr.sin_addr), addr.sin_port)][transaction_id] = REQUEST_TYPE_PING;
		#ifdef DHT_DEBUG
				printf("OK\n");
		#endif
	}
	catch (Exception & e) {
		#ifdef DHT_DEBUG
				printf("Fail: %s\n", exception_errcode2str(e).c_str());
		#endif
	}
	catch(SyscallException & e)
	{
		#ifdef DHT_DEBUG
				printf("Fail: %s\n", e.get_errno_str());
		#endif
	}
}

void dht::find_node(const node_id & recipient, const node_id & target)
{
	find_node(m_nodes[recipient], target);
}

void dht::find_node(const sockaddr_in & recipient, const node_id & target)
{
	uint16_t transaction_id = rand() % 65536;
	char buf[256] = "d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe";
	m_node_id.copy2(&buf[12]);
	target.copy2(&buf[43]);
	memcpy(&buf[83], &transaction_id, TRANSACTION_ID_LENGHT);
	try
	{
		#ifdef DHT_DEBUG
			node_id_hex hex;
			target.to_hex(hex);
			printf("Sending find node to %s:%d transaction=%u searching node id=%s...", inet_ntoa(recipient.sin_addr), ntohs(recipient.sin_port), transaction_id, hex);
		#endif
		m_nm->Socket_send(m_sock, buf, FIND_NODE_REQUEST_LENGHT, recipient);
		m_requests[ip_port(inet_ntoa(recipient.sin_addr), recipient.sin_port)][transaction_id] = REQUEST_TYPE_FIND_NODE;
		#ifdef DHT_DEBUG
			printf("OK\n");
		#endif
	}
	catch (Exception & e) {
		#ifdef DHT_DEBUG
			printf("Fail: %s\n", exception_errcode2str(e).c_str());
		#endif
	}
	catch(SyscallException & e)
	{
		#ifdef DHT_DEBUG
			printf("Fail: %s\n", e.get_errno_str());
		#endif
	}
}

int dht::event_sock_ready2read(network::Socket sock)
{
	char buf[1024];
	sockaddr_in addr;
	size_t len = m_nm->Socket_recv(m_sock, buf, 1024, &addr);
	#ifdef DHT_DEBUG
		printf("Incomming message from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	#endif
	bencode::be_node * node = bencode::decode(buf, len, false);
	if (node == NULL)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return 0;
	}

	bencode::be_str * str;
	if (bencode::get_str(node, "t", &str) != 0 || str->len != TRANSACTION_ID_LENGHT)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return 0;
	}

	uint16_t transaction_id;
	memcpy(&transaction_id, str->ptr, TRANSACTION_ID_LENGHT);

	bencode::be_str * y_key;
	if (bencode::get_str(node, "y", &y_key) != 0)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return 0;
	}

	if (*y_key->ptr == 'r')
	{
		#ifdef DHT_DEBUG
			printf("	it`s response, transaction id=%u\n", transaction_id);
		#endif
		bencode::be_node * r;
		if (bencode::get_node(node, "r", &r) != 0)
		{
			#ifdef DHT_DEBUG
				printf("	reject, invalid format\n");
			#endif
			return 0;
		}

		bencode::be_str * id_str;
		if (bencode::get_str(r, "id", &id_str) == -1 || id_str->len != NODE_ID_LENGHT)
		{
			#ifdef DHT_DEBUG
				printf("	reject, invalid format\n");
			#endif
			return 0;
		}
		node_id id = id_str->ptr;
		#ifdef DHT_DEBUG
				node_id_hex hex;
				id.to_hex(hex);
				printf("	from node id=%s\n", hex);
		#endif

		request_container::iterator request_container_iter = m_requests.find(ip_port(inet_ntoa(addr.sin_addr), addr.sin_port));
		if (request_container_iter == m_requests.end())
		{
			#ifdef DHT_DEBUG
				printf("	such query does not exists or invalid transaction id\n");
			#endif
			return 0;
		}
		std::map<uint16_t, REQUEST_TYPE>::iterator iter = request_container_iter->second.find(transaction_id);
		if (iter == request_container_iter->second.end())
		{
			#ifdef DHT_DEBUG
				printf("	invalid transaction id\n");
			#endif
			return 0;
		}
		REQUEST_TYPE request_type = iter->second;
		request_container_iter->second.erase(iter);

		if (request_type == REQUEST_TYPE_PING)
		{
			#ifdef DHT_DEBUG
				printf("	it`s 'ping' response\n");
			#endif
			m_nodes[id] = addr;
		}

		if (request_type == REQUEST_TYPE_FIND_NODE)
		{
			bencode::be_str * nodes;
			#ifdef DHT_DEBUG
				printf("	it`s 'find node' response\n");
			#endif
			if (bencode::get_str(r, "nodes", &nodes) == -1 || nodes->len % COMPACT_NODE_INFO_LENGHT != 0)
			{
				#ifdef DHT_DEBUG
					printf("	reject, invalid format\n");
				#endif
				return 0;
			}
			for(ssize_t i = 0; i < nodes->len / COMPACT_NODE_INFO_LENGHT; i++)
			{
				node_id found_id;
				sockaddr_in addr;
				parse_compact_node_info(&nodes->ptr[i * COMPACT_NODE_INFO_LENGHT], found_id, addr);
				m_nodes[found_id] = addr;
				#ifdef DHT_DEBUG
					node_id_hex hex;
					found_id.to_hex(hex);
					int bucket = get_bucket(found_id, m_node_id);
					printf("	node id=%s bucket = %d %s:%d\n", hex,  bucket, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
				#endif
			}
		}
	}
	if (*y_key->ptr == 'q')
	{

	}

	if (*y_key->ptr == 'e')
	{
		#ifdef DHT_DEBUG
				printf("	it`s error message for query with transaction id=%u\n", transaction_id);
		#endif

		request_container::iterator request_container_iter = m_requests.find(ip_port(inet_ntoa(addr.sin_addr), addr.sin_port));
		if (request_container_iter == m_requests.end())
		{
			#ifdef DHT_DEBUG
				printf("	such query does not exists or invalid transaction id\n");
			#endif
			return 0;
		}
		std::map<uint16_t, REQUEST_TYPE>::iterator iter = request_container_iter->second.find(transaction_id);
		if (iter == request_container_iter->second.end())
		{
			#ifdef DHT_DEBUG
				printf("	invalid transaction id\n");
			#endif
			return 0;
		}
		request_container_iter->second.erase(iter);


		bencode::be_node * l;
		if (bencode::get_node(node, "e", &l) != 0 || !bencode::is_list(l) || l->val.l.count != 2)
		{
			#ifdef DHT_DEBUG
				printf("	reject, invalid format\n");
			#endif
			return 0;
		}
		uint64_t code;
		bencode::be_str * desc;
		if (bencode::get_int(l, 0, &code) != 0 || bencode::get_str(l, 1, &desc) != 0)
		{
			#ifdef DHT_DEBUG
				printf("	reject, invalid format\n");
			#endif
			return 0;
		}
		#ifdef DHT_DEBUG
				printf("	%llu, %s\n", code, desc->ptr);
		#endif
	}

}

int dht::event_sock_closed(network::Socket sock)
{

}

int dht::event_sock_sended(network::Socket sock)
{

}

int dht::event_sock_connected(network::Socket sock)
{

}

int dht::event_sock_accepted(network::Socket sock, network::Socket accepted_sock)
{

}

int dht::event_sock_timeout(network::Socket sock)
{

}

int dht::event_sock_unresolved(network::Socket sock)
{

}


}
}


