/*
 * network.cpp
 *
 *  Created on: 21.01.2012
 *      Author: ruslan
 */
#include "network.h"

namespace dinosaur {
namespace network
{

ssize_t buffer_remain(const buffer & buf)
{
	return buf.length - buf.pos;
}

double get_time()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec / (double)POW10_6;
}

NetworkManager::NetworkManager()
	: m_thread_stop(false), m_thread(&NetworkManager::timeout_thread, this)
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "NetworkManager default constructor";
#endif
	m_epoll_fd = epoll_create(10);
	if (m_epoll_fd == -1)
		throw SyscallException();
}

/*
 * SyscallException
 */

void NetworkManager::Init()
{

}

NetworkManager::~NetworkManager()
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "NetworkManager destructor";
#endif
	m_thread_stop = true;
	m_thread.join();
	socket_set temp_sockets = m_sockets;
	for(socket_set_iter iter = temp_sockets.begin(); iter != temp_sockets.end(); ++iter)
	{
		(*iter)->m_assoc.reset();
	}
	m_sockets.clear();
	temp_sockets.clear();
	m_timeout_sockets.clear();
	m_unresolved_sockets.clear();
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "NetworkManager destroyed";
#endif
}

/*
 * SyscallException
 * Exception::ERR_CODE_INVALID_FORMAT
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_add(const std::string & ip, uint16_t port, const SocketEventInterfacePtr & assoc, Socket & sock)
{
	sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (port);
	int ret = inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);
	if (ret == 0)
		throw Exception(Exception::ERR_CODE_INVALID_FORMAT);
	if (ret == -1)
		throw Exception(Exception::ERR_CODE_UNDEF);
	Socket_add(sockaddr, assoc, sock);
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_add_domain(const std::string & domain_name, uint16_t port, const SocketEventInterfacePtr & assoc, Socket & sock)
{
	#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Adding socket " << domain_name;
#endif
	sock.reset(new socket_());
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	sock->m_assoc = assoc;
	sock->m_peer.sin_port = htons (port);
	sock->m_need2resolved = true;
	sock->m_domain = domain_name;
	sock->m_timer = 0;
	std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
	m_sockets.insert(sock);
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Socket is added " << domain_name;
#endif
}

/*
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 * SyscallException
 */

void NetworkManager::Socket_add(const sockaddr_in & addr, const SocketEventInterfacePtr & assoc, Socket & sock)
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Adding socket " <<  inet_ntoa(addr.sin_addr);
#endif
	sock.reset(new socket_());
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	struct epoll_event event;
	sock->m_assoc = assoc;
	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	{
		sock.reset();
		throw SyscallException();
	}
	event.data.ptr=(void*)sock.get();
	event.events = EPOLLOUT;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		int err = errno;
		close(sock->m_socket);
		sock.reset();
		throw SyscallException(err);
	}
	sock->m_peer = addr;
	if (connect(sock->m_socket, (const struct sockaddr *)&sock->m_peer, sizeof(sockaddr_in)) == -1)
	{
		if (errno == EINPROGRESS)
		{
			sock->m_state = SOCKET_STATE_CONNECTION;
			goto end;
		}
		int err = errno;
		close(sock->m_socket);
		sock.reset();
		throw SyscallException(err);
	}
	sock->m_state = SOCKET_STATE_TRANSMISSION;
	epollout(sock.get(), false);
	epollin(sock.get(), true);
	sock->event_sock_connected();
end:
	sock->m_timer = time(NULL);
	std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
	m_sockets.insert(sock);
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Socket is added " <<  sock->m_socket << " " << inet_ntoa(sock->m_peer.sin_addr);
#endif
}

/*
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 * SyscallException
 */

void NetworkManager::Socket_add(int sock_fd, const sockaddr_in & addr, const SocketEventInterfacePtr & assoc, Socket & sock)
{
	sock.reset(new socket_());
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	struct epoll_event event;
	sock->m_assoc = assoc;
	sock->m_socket = sock_fd;
	sock->m_peer = addr;
	event.data.ptr=(void*)sock.get();
	event.events = EPOLLIN;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		int err = errno;
		close(sock->m_socket);
		sock.reset();
		throw SyscallException(err);
	}
	sock->m_state = SOCKET_STATE_TRANSMISSION;

	sock->m_timer = time(NULL);
	std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
	m_sockets.insert(sock);
}

/*
 * Exception::ERR_CODE_UNDEF
 * SyscallException
 */

void NetworkManager::Socket_add_UDP(const SocketEventInterfacePtr & assoc, Socket & sock, const sockaddr_in * bind_)
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Adding UDP socket ";
#endif
	sock.reset(new socket_());
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	struct epoll_event event;
	sock->m_assoc = assoc;
	sock->m_udp = true;
	if ((sock->m_socket = socket (AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) == -1)
	{
		sock.reset();
		throw SyscallException();
	}
	if (bind_ != NULL)
		if (bind(sock->m_socket, (const sockaddr *)bind_, sizeof(sockaddr_in)) == -1)
		{
			sock.reset();
			throw SyscallException();
		}
	event.data.ptr=(void*)sock.get();
	event.events = EPOLLIN;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		int err = errno;
		close(sock->m_socket);
		sock.reset();
		throw SyscallException(err);
	}
	memset((void *)&sock->m_peer, 0, sizeof(struct sockaddr_in));
	sock->m_state = SOCKET_STATE_TRANSMISSION;
	sock->m_timer = time(NULL);
	std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
	m_sockets.insert(sock);
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "UDP Socket is added " <<  sock->m_socket;
#endif
}

/*
 * SyscallException
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::ListenSocket_add(const sockaddr_in & addr, const SocketEventInterfacePtr & assoc, Socket & sock)
{
	struct epoll_event event;
	const unsigned int yes = 1;
	sock.reset(new socket_());
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);

	sock->m_assoc = assoc;

	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	{
		sock.reset();
		throw SyscallException();
	}

	if (setsockopt (sock->m_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) == -1)
	{
		int err = errno;
		close(sock->m_socket);
		sock.reset();
		throw SyscallException(err);
	}

	//memcpy((void *)&sock->m_peer, (void *)addr, sizeof(struct sockaddr_in));
	sock->m_peer = addr;
	if (bind (sock->m_socket, (const struct sockaddr *)&sock->m_peer, sizeof (struct sockaddr)))
	{
		int err = errno;
		close(sock->m_socket);
		sock.reset();
		throw SyscallException(err);
	}

	if (listen (sock->m_socket, SOMAXCONN) == -1)
	{
		int err = errno;
		close(sock->m_socket);
		sock.reset();
		throw SyscallException(err);
	}

	event.data.ptr = (void*)sock.get();
	event.events = EPOLLIN;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		int err = errno;
		close(sock->m_socket);
		sock.reset();
		throw SyscallException(err);
	}

	sock->m_state = SOCKET_STATE_LISTEN;
	sock->m_timer = time(NULL);
	std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
	m_sockets.insert(sock);
}

/*
 * SyscallException
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::ListenSocket_add(uint16_t port, const SocketEventInterfacePtr & assoc, Socket & sock)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	if (inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr) == -1)
		throw Exception(Exception::ERR_CODE_UNDEF);
	ListenSocket_add(addr, assoc, sock);
}

/*
 * SyscallException
 * Exception::ERR_CODE_UNDEF
 */

void NetworkManager::ListenSocket_add(uint16_t port, const in_addr & addr, const SocketEventInterfacePtr & assoc, Socket & sock)
{
	sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (port);
	//memcpy(&sockaddr.sin_addr, addr, sizeof(in_addr));
	sockaddr.sin_addr = addr;
	ListenSocket_add(sockaddr, assoc, sock);
}

/*
 * SyscallException
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_INVALID_FORMAT
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::ListenSocket_add(uint16_t port, const std::string & ip, const SocketEventInterfacePtr & assoc, Socket & sock)
{
	ListenSocket_add(port, ip.c_str(), assoc, sock);
}

/*
 * SyscallException
 */

void NetworkManager::handler_connection(Socket & sock)
{
	int connect_result;
	socklen_t opt_len = sizeof(int);
	//ошибка в errno
	if (getsockopt (sock->m_socket, SOL_SOCKET, SO_ERROR, &connect_result, &opt_len) == -1)
		throw SyscallException();

	if (connect_result != 0)
		throw SyscallException(connect_result);

	sock->m_state = SOCKET_STATE_TRANSMISSION;

	epollin(sock.get(), true);
	epollout(sock.get(), buffer_remain(sock->m_send_buffer) != 0);

	sock->event_sock_connected();
}

/*
 * SyscallException
 */

void NetworkManager::socket_state_transmission_out_handler(Socket & sock)
{
	if (sock->m_send_buffer.length == 0)
		return;
	while(1)
	{
		ssize_t ret = sock->m_send_buffer.length - sock->m_send_buffer.pos;
		if (ret == 0)
		{
			sock->m_send_buffer.length = 0;
			sock->m_send_buffer.pos = 0;
			epollout(sock.get(), false);
			sock->event_sock_sended();
			return;
		}
		ssize_t sended = send(sock->m_socket, sock->m_send_buffer.data + sock->m_send_buffer.pos, ret, MSG_NOSIGNAL);
		if (sended == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
		if (sended == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK))
			throw SyscallException();
		if (sended != -1)
		{
			sock->m_send_buffer.pos += sended;
			sock->m_tx += sended;
		}
	}
}

void NetworkManager::socket_state_transmission_in_handler(Socket & sock)
{
	sock->event_sock_ready2read();
}

/*
 * SyscallException
 */

void NetworkManager::handle_event(Socket & sock, uint32_t events)
{
	if ((events & EPOLLOUT) != 0)
	{
		if (sock->m_state == SOCKET_STATE_CONNECTION)
		{
			handler_connection(sock);
		}
		if (sock->m_state == SOCKET_STATE_TRANSMISSION)
		{
			socket_state_transmission_out_handler(sock);
		}
	}
	if ((events & EPOLLIN) != 0)
	{
		if (sock->m_state == SOCKET_STATE_TRANSMISSION)
		{
			socket_state_transmission_in_handler(sock);
		}
		if (sock->m_state == SOCKET_STATE_LISTEN)
		{
			int client_sock;
			sockaddr_in client_addr;
			socklen_t client_addr_len = sizeof(client_addr);

			memset(&client_addr, '\0', sizeof(client_addr));
			client_sock = accept4(sock->m_socket, (struct sockaddr *)&client_addr, &client_addr_len, SOCK_NONBLOCK);
			if (client_sock == -1)
			{
				if (errno != EAGAIN && errno != EWOULDBLOCK)
					throw SyscallException();
				throw SyscallException();
			}
			SocketEventInterfacePtr assoc;
			Socket new_sock;
			try
			{
				Socket_add(client_sock, client_addr, assoc, new_sock);
				sock->event_sock_accepted(new_sock);
			}
			catch (...) {

			}
		}
	}
}

/*
 * SyscallException
 */


int NetworkManager::clock()
{
	struct epoll_event events[MAX_EPOLL_EVENT];
	int nfds = epoll_wait(m_epoll_fd, events, MAX_EPOLL_EVENT, 10);
	if (nfds == -1)
	{
		throw SyscallException();
	}
	std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
	for(int i = 0; i < nfds; i++)
	{
		Socket  sock = ((socket_ *)events[i].data.ptr)->shared_from_this();
		sock->m_timer = time(NULL);
		try
		{
			handle_event(sock, events[i].events);
		}
		catch(SyscallException & e)
		{
			sock->close_();
			sock->event_sock_error(e.get_errno());
			sock->m_errno = e.get_errno();
		}
	}
	//for(socket_set::iterator iter = m_connected_after_resolving_sockets.begin(); iter != m_connected_after_resolving_sockets.end(); ++iter)
	ERASABLE_LOOP(m_connected_after_resolving_sockets, iter, inc_iter)
	{
		++inc_iter;
		(*iter)->event_sock_connected();
	}
	//for(socket_set::iterator iter = m_unresolved_sockets.begin(); iter != m_unresolved_sockets.end(); ++iter)
	ERASABLE_LOOP(m_unresolved_sockets, iter, inc_iter)
	{
		++inc_iter;
		(*iter)->event_sock_unresolved();
	}
	//for(socket_set::iterator iter = m_timeout_sockets.begin(); iter != m_timeout_sockets.end(); ++iter)
	ERASABLE_LOOP(m_timeout_sockets, iter, inc_iter)
	{
		++inc_iter;
		(*iter)->event_sock_timeout();
	}
	m_connected_after_resolving_sockets.clear();
	m_unresolved_sockets.clear();
	m_timeout_sockets.clear();
	return ERR_NO_ERROR;
}


/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_SOCKET_CLOSED
 * Exception::ERR_CODE_SOCKET_CAN_NOT_SEND
 * Exception::ERR_CODE_INVALID_OPERATION
 */

size_t NetworkManager::Socket_send(Socket & sock, const void * data, size_t len, bool full)
{
	if (sock == NULL  || data == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	if (sock->m_udp)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);
	if (sock->closed())
		throw Exception(Exception::ERR_CODE_SOCKET_CLOSED);
	if (len == 0)
		return 0;
	size_t can_send = BUFFER_SIZE - sock->m_send_buffer.length;
	if (full && can_send < len)
		throw Exception(Exception::ERR_CODE_SOCKET_CAN_NOT_SEND);
	can_send = can_send < len ? can_send : len;
	memcpy(sock->m_send_buffer.data + sock->m_send_buffer.length, data, can_send);
	sock->m_send_buffer.length += can_send;
	epollout(sock.get(), true);
	return can_send;
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_SOCKET_CLOSED
 * Exception::ERR_CODE_INVALID_OPERATION
 * SyscallException
 */

ssize_t NetworkManager::Socket_send(Socket & sock, const void * data, size_t len, const sockaddr_in & addr)
{
	if (sock == NULL  || data == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	if (!sock->m_udp)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);
	if (sock->closed())
		throw Exception(Exception::ERR_CODE_SOCKET_CLOSED);
	if (len == 0)
		return 0;
	ssize_t ret = sendto(sock->m_socket, data, len, 0, (const sockaddr*)&addr, sizeof(sockaddr_in));
	if (ret == -1)
		throw SyscallException(errno);
	return ret;
}

/*
 * Exception::ERR_CODE_NULL_REF
 * SyscallException
 */

size_t NetworkManager::Socket_recv(Socket & sock, char * data, size_t len, bool & is_closed, sockaddr_in * from)
{
	if (sock == NULL || data == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	if (len == 0)
		return 0;
	is_closed = false;
	if (sock->m_udp)
	{
		socklen_t s = sizeof(sockaddr_in);
		ssize_t ret = recvfrom(sock->m_socket, data, len, 0, (sockaddr*)from, &s);
		if (ret == -1)
			throw SyscallException(errno);
		return ret;
	}
	size_t total_received = 0;
	while(len != total_received)
	{
		ssize_t received = recv(sock->m_socket, data + total_received, len - total_received, 0);
		if (received == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return total_received;
			if (errno == ECONNREFUSED)
			{
				is_closed = true;
				return total_received;
			}
			else
				throw SyscallException();
		}
		if (received == 0)
		{
			sock->close_();
			is_closed = true;
			return total_received;
		}
		total_received += received;
		sock->m_rx += received;
	}
	return total_received;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

bool NetworkManager::Socket_closed(Socket & sock)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	return sock->closed();
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_close(Socket & sock)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	sock->close_();
}

void NetworkManager::Socket_delete(Socket & sock)
{
	if (sock == NULL)
		return;
	std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
	m_sockets.erase(sock);
	m_timeout_sockets.erase(sock);
	m_connected_after_resolving_sockets.erase(sock);
	m_unresolved_sockets.erase(sock);
	sock->close_();
	sock->m_need2delete = true;
	sock.reset();
	return;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_set_event_interface(Socket & sock, const SocketEventInterfacePtr & assoc)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	sock->m_assoc = assoc;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_get_event_interface(Socket & sock, SocketEventInterfacePtr & assoc)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	assoc =  sock->m_assoc;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

int NetworkManager::Socket_get_rx_speed(Socket & sock)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	return sock->m_rx / (get_time() - sock->m_rx_last_time);
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

int NetworkManager::Socket_get_tx_speed(Socket & sock)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	return sock->m_tx / (get_time()- sock->m_tx_last_time);
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_get_addr(Socket & sock, sockaddr_in & addr)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	memcpy(&addr, &sock->m_peer, sizeof(sockaddr_in));
}

/*
 * SyscallException
 */

void NetworkManager::epoll_mod(socket_ * sock)
{
	struct epoll_event event;
	event.data.ptr = (void*)sock;
	event.events = sock->m_epoll_events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, sock->m_socket, &event) == -1)
		throw SyscallException();
}

/*
 * SyscallException
 */

void NetworkManager::epoll_mod(socket_ * sock, uint32_t events)
{
	sock->m_epoll_events = events;
	epoll_mod(sock);
}

/*
 * SyscallException
 */

void NetworkManager::epollin(socket_ * sock, bool on)
{
	if (on == true)
	{
		if ((sock->m_epoll_events & EPOLLIN) != 0)
			return;
		sock->m_epoll_events |= EPOLLIN;
		epoll_mod(sock);
	}
	if (on == false)
	{
		if ((sock->m_epoll_events & EPOLLIN) == 0)
			return;
		sock->m_epoll_events ^= EPOLLIN;
		epoll_mod(sock);
	}
}

/*
 * SyscallException
 */

void NetworkManager::epollout(socket_ * sock, bool on)
{
	if (on == true)
	{
		if ((sock->m_epoll_events & EPOLLOUT) != 0)
			return;
		sock->m_epoll_events |= EPOLLOUT;
		epoll_mod(sock);
	}
	if (on == false)
	{
		if ((sock->m_epoll_events & EPOLLOUT) == 0)
			return;
		sock->m_epoll_events ^= EPOLLOUT;
		epoll_mod(sock);
	}
}

/*
 * SyscallException
 */

void NetworkManager::ConnectResolvedSocket(Socket & sock)
{
	struct epoll_event event;
	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
		throw SyscallException();

	event.data.ptr = (void*)sock.get();
	event.events = EPOLLOUT;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		int err = errno;
		close(sock->m_socket);
		throw SyscallException(err);
	}
	if (connect(sock->m_socket, (const struct sockaddr *)&sock->m_peer, sizeof(struct sockaddr_in)) == -1)
	{
		if (errno == EINPROGRESS)
		{
			sock->m_state = SOCKET_STATE_CONNECTION;
			goto end;
		}
		int err = errno;
		close(sock->m_socket);
		throw SyscallException(err);
	}
	sock->m_state = SOCKET_STATE_TRANSMISSION;
	epollin(sock.get(), true);
	epollout(sock.get(), buffer_remain(sock->m_send_buffer) != 0);
	m_connected_after_resolving_sockets.insert(sock);
end:
	sock->m_timer = time(NULL);
}

/*
 * SyscallException
 */

void NetworkManager::timeout_thread()
{
	std::list<Socket> sock2resolve;
	while(!m_thread_stop)
	{
		//logger::LOGGER() << "timeout_thread loop";
		{
			std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
			for(socket_set_iter iter = m_sockets.begin(); iter != m_sockets.end(); ++iter)
			{
				Socket sock = *iter;

				if (sock->m_timer != 0 &&
					time(NULL) - sock->m_timer >= TIMEOUT &&
					(sock->m_state == SOCKET_STATE_CONNECTION || (sock->m_epoll_events & EPOLLIN) != 0) &&
					!sock->closed())
				{
					m_timeout_sockets.insert(sock);
					sock->m_timer = time(NULL);
				}
				if (sock->m_need2resolved)
					sock2resolve.push_back(sock);
			}
		}

		//for(std::list<Socket>::iterator iter = sock2resolve.begin(); iter != sock2resolve.end(); ++iter)
		while(!sock2resolve.empty())
		{
			std::string domain_name;
			sockaddr_in addr;
			Socket sock;

			{
				std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
				sock = sock2resolve.front();
				sock2resolve.pop_front();
				sock->m_need2resolved = false;
				domain_name = sock->m_domain;
				addr = sock->m_peer;
			}


			try
			{
				DomainNameResolver dnr(sock->m_domain, &addr);
				std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
				sock->m_peer = addr;
				if (sock->m_need2delete)
					continue;
				try
				{
					ConnectResolvedSocket(sock);
				}
				catch(SyscallException & e)
				{
					throw SyscallException(e);
				}
			}
			catch (...)
			{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "DomainNameResolver is can not resolve "<< sock->m_domain.c_str();
#endif
				std::lock_guard<std::recursive_mutex> lock(m_mutex_sockets);
				if (!sock->m_need2delete)
					m_unresolved_sockets.insert(sock);
			}
		}

		usleep(5000);
	}
}

}
}
