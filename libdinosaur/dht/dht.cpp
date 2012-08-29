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

dht::dht(network::NetworkManager* nm, const node_id & our_id)
{
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

int dht::event_sock_ready2read(network::Socket sock)
{

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


