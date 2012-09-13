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

dht::dht(network::NetworkManager* nm, const std::string & work_dir)
{
#ifdef DHT_DEBUG
	printf("Creating DHT\n");
#endif
	m_nm = nm;
}

dht::~dht()
{
	m_rt->save();
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

dht::ip_port dht::sockaddr2ip_port(const sockaddr_in & addr)
{
	return ip_port(inet_ntoa(addr.sin_addr), addr.sin_port);
}

void dht::send_ping(const sockaddr_in & addr)
{
	TRANSACTION_ID transaction_id = rand() % 65536;
	char buf[256] = "d1:ad2:id20:00000000000000000000e1:q4:ping1:t2:001:y1:qe";
	m_rt->get_node_id().copy2(&buf[12]);
	memcpy(&buf[47], &transaction_id, TRANSACTION_ID_LENGTH);
	try
	{
		#ifdef DHT_DEBUG
				printf("Sending ping to %s:%d transaction=%u...", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), transaction_id);
		#endif
		m_nm->Socket_send(m_sock, buf, PING_REQUEST_LENGTH, addr);
		m_requests[sockaddr2ip_port(addr)][transaction_id] = REQUEST_TYPE_PING;
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
	find_node((*m_rt)[recipient], target);
}

void dht::find_node(const sockaddr_in & recipient, const node_id & target)
{
	TRANSACTION_ID transaction_id = rand() % 65536;
	char buf[256] = "d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe";
	m_rt->get_node_id().copy2(&buf[12]);
	target.copy2(&buf[43]);
	memcpy(&buf[83], &transaction_id, TRANSACTION_ID_LENGTH);
	try
	{
		#ifdef DHT_DEBUG
			std::string hex;
			target.to_hex(hex);
			printf("Sending find node to %s:%d transaction=%u searching node id=%s...", inet_ntoa(recipient.sin_addr), ntohs(recipient.sin_port), transaction_id, hex.c_str());
		#endif
		m_nm->Socket_send(m_sock, buf, FIND_NODE_REQUEST_LENGTH, recipient);
		m_requests[sockaddr2ip_port(recipient)][transaction_id] = REQUEST_TYPE_FIND_NODE;
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

void dht::get_peers(const node_id & recipient, const SHA1_HASH infohash)
{
	get_peers((*m_rt)[recipient], infohash);
}

void dht::get_peers(const sockaddr_in & recipient, const SHA1_HASH infohash)
{
	TRANSACTION_ID transaction_id = rand() % 65536;
	char buf[256] = "d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe";
	m_rt->get_node_id().copy2(&buf[12]);
	infohash.copy2(&buf[46]);
	memcpy(&buf[86], &transaction_id, TRANSACTION_ID_LENGTH);
	try
	{
		#ifdef DHT_DEBUG
			SHA1_HASH_HEX hex;
			infohash.to_hex(hex);
			printf("Sending get peers to %s:%d transaction=%u infohash=%s...", inet_ntoa(recipient.sin_addr), ntohs(recipient.sin_port), transaction_id, hex.c_str());
		#endif
		m_nm->Socket_send(m_sock, buf, GET_PEERS_REQUEST_LENGTH, recipient);
		m_requests[sockaddr2ip_port(recipient)][transaction_id] = REQUEST_TYPE_GET_PEERS;
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

void dht::announce_peer(const node_id & recipient, const SHA1_HASH infohash, uint16_t port, const TOKEN & token)
{
	announce_peer((*m_rt)[recipient], infohash, port, token);
}

void dht::announce_peer(const sockaddr_in & recipient, const SHA1_HASH infohash, uint16_t port, const TOKEN & token)
{
	TRANSACTION_ID transaction_id = rand() % 65536;
	bencode::be_node * message_bencoded = bencode::create_dict();
	bencode::dict_add_str(message_bencoded, "t", 1, (const char*)&transaction_id, 2);
	bencode::dict_add_str(message_bencoded, "y", 1, "q", 1);
	bencode::dict_add_str(message_bencoded, "q", 1, "announce_peer", 13);
	bencode::be_node * message_body_encoded = bencode::create_dict();
	bencode::dict_add_str(message_body_encoded, "id", 2, (const char*)&m_rt->get_node_id()[0], NODE_ID_LENGTH);
	bencode::dict_add_str(message_body_encoded, "infohash", 8, (const char*)&infohash[0], SHA1_LENGTH);
	bencode::dict_add_int(message_body_encoded, "port", 4, port);
	bencode::dict_add_str(message_body_encoded, "token", 5, token.token, token.length);
	bencode::dict_add_node(message_bencoded, "a", 1, message_body_encoded);
	bencode::dump(message_bencoded);

	char * buf = new char[1024];
	uint32_t encoded_message_len;
	bencode::encode(message_bencoded, &buf, 1024, &encoded_message_len);
	try
	{
		#ifdef DHT_DEBUG
			SHA1_HASH_HEX hex;
			infohash.to_hex(hex);
			printf("Sending announce_peer to %s:%d transaction=%u infohash=%s...", inet_ntoa(recipient.sin_addr), ntohs(recipient.sin_port), transaction_id, hex.c_str());
		#endif
		m_nm->Socket_send(m_sock, buf, encoded_message_len, recipient);
		m_requests[sockaddr2ip_port(recipient)][transaction_id] = REQUEST_TYPE_ANNOUNCE_PEER;
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
	delete[] buf;
}

void dht::response_handler(bencode::be_node * message_bencoded, REQUEST_TYPE request_type, const sockaddr_in & addr)
{
	bencode::be_node * reply_bencoded;
	if (bencode::get_node(message_bencoded, "r", &reply_bencoded) != 0)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return;
	}

	bencode::be_str * id_str;
	if (bencode::get_str(reply_bencoded, "id", &id_str) == -1 || id_str->len != NODE_ID_LENGTH)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return;
	}
	node_id id = id_str->ptr;
	#ifdef DHT_DEBUG
			std::string hex;
			id.to_hex(hex);
			printf("	from node id=%s\n", hex.c_str());
	#endif
	m_rt->update_node(id);

	if (request_type == REQUEST_TYPE_PING)
		ping_handler(id, addr);

	if (request_type == REQUEST_TYPE_FIND_NODE)
		find_node_handler(reply_bencoded);
}

void dht::ping_handler(const node_id & id, const sockaddr_in & addr)
{
	#ifdef DHT_DEBUG
		printf("	it`s 'ping' response\n");
	#endif
	if (!m_rt->node_exists(id))
	{
		m_rt->add_node(id, addr);
		find_node(id, m_rt->get_node_id());
	}
}

void dht::find_node_handler(bencode::be_node * reply_bencoded)
{
	bencode::be_str * nodes_list_bencoded;
	#ifdef DHT_DEBUG
		printf("	it`s 'find node' response\n");
	#endif
	if (bencode::get_str(reply_bencoded, "nodes", &nodes_list_bencoded) == -1 || nodes_list_bencoded->len % COMPACT_NODE_INFO_LENGTH != 0)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return;
	}
	for(ssize_t i = 0; i < nodes_list_bencoded->len / COMPACT_NODE_INFO_LENGTH; i++)
	{
		node_id node_from_list;
		sockaddr_in addr;
		parse_compact_node_info(&nodes_list_bencoded->ptr[i * COMPACT_NODE_INFO_LENGTH], node_from_list, addr);
		m_rt->add_node(node_from_list, addr);
		#ifdef DHT_DEBUG
			std::string hex;
			node_from_list.to_hex(hex);
			int bucket = get_bucket(node_from_list, m_rt->get_node_id());
			printf("	node id=%s bucket = %d %s:%d\n", hex.c_str(),  bucket, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		#endif
	}
}

void dht::error_handler(bencode::be_node * message_bencoded)
{
	bencode::be_node * error_message_bencoded;
	if (bencode::get_node(message_bencoded, "e", &error_message_bencoded) != 0 || !bencode::is_list(error_message_bencoded) || error_message_bencoded->val.l.count != ERROR_MESSAGE_LEN)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return;
	}
	uint64_t err_code;
	bencode::be_str * err_desc;
	if (bencode::get_int(error_message_bencoded, ERROR_MESSAGE_ERR_CODE_INDEX, &err_code) != 0 || bencode::get_str(error_message_bencoded, ERROR_MESSAGE_ERR_DESC_INDEX, &err_desc) != 0)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return;
	}
	#ifdef DHT_DEBUG
			printf("	%llu, %s\n", err_code, err_desc->ptr);
	#endif
}

int dht::event_sock_ready2read(network::Socket sock)
{
	char buf[1024];
	sockaddr_in addr;
	size_t len = m_nm->Socket_recv(m_sock, buf, 1024, &addr);
	#ifdef DHT_DEBUG
		printf("Incomming message from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	#endif
	bencode::be_node * message_bencoded = bencode::decode(buf, len, false);
	if (message_bencoded == NULL)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return 0;
	}

	bencode::be_str * str;
	if (bencode::get_str(message_bencoded, "t", &str) != 0 || str->len != TRANSACTION_ID_LENGTH)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return 0;
	}
	TRANSACTION_ID transaction_id;
	memcpy(&transaction_id, str->ptr, TRANSACTION_ID_LENGTH);

	bencode::be_str * message_type;
	if (bencode::get_str(message_bencoded, "y", &message_type) != 0)
	{
		#ifdef DHT_DEBUG
			printf("	reject, invalid format\n");
		#endif
		return 0;
	}

	if (*message_type->ptr == MESSAGE_TYPE_REPLY || *message_type->ptr == MESSAGE_TYPE_ERROR)
	{
		request_container::iterator request_container_iter = m_requests.find(sockaddr2ip_port(addr));
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

		if (*message_type->ptr == MESSAGE_TYPE_REPLY)
		{
			#ifdef DHT_DEBUG
				printf("	it`s response, transaction id=%u\n", transaction_id);
			#endif
			response_handler(message_bencoded, request_type, addr);
		}
		if (*message_type->ptr == MESSAGE_TYPE_ERROR)
		{
			#ifdef DHT_DEBUG
					printf("	it`s error message for query with transaction id=%u\n", transaction_id);
			#endif
			error_handler(message_bencoded);
		}

	}
	if (*message_type->ptr == MESSAGE_TYPE_QUERY)
	{

	}

}

void dht::add_node(const sockaddr_in & addr)
{
	send_ping(addr);
}

void dht::clock()
{
	m_rt->clock();
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


