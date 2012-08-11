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

ssize_t buffer_remain(struct buffer * buf)
{
	return buf->length - buf->pos;
}

double get_time()
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / (double)POW10_9;
}

NetworkManager::NetworkManager()
{
#ifdef BITTORRENT_DEBUG
	printf("NetworkManager default constructor\n");
#endif
	m_thread = 0;
	m_thread_stop = false;
}

/*
 * SyscallException
 */

void NetworkManager::Init() throw (SyscallException)
{
	m_epoll_fd = epoll_create(10);
	if (m_epoll_fd == -1)
		throw SyscallException();
	m_thread_stop = false;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutexattr_t   mta;
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex_sockets, &mta);
	pthread_mutex_unlock(&m_mutex_sockets);
	int ret = pthread_create(&m_thread, &attr, NetworkManager::timeout_thread, (void *)this);
	if (ret)
		throw Exception(Exception::ERR_CODE_CAN_NOT_CREATE_THREAD);
	pthread_attr_destroy(&attr);
}

NetworkManager::~NetworkManager()
{
#ifdef BITTORRENT_DEBUG
	printf("NetworkManager destructor\n");
#endif
	if (m_thread != 0)
	{
		m_thread_stop = true;
		void * status;
		pthread_join(m_thread, &status);
		pthread_mutex_destroy(&m_mutex_sockets);
	}
	socket_set temp_sockets = m_sockets;
	for(socket_set_iter iter = temp_sockets.begin(); iter != temp_sockets.end(); ++iter)
	{
		(*iter)->m_assoc.reset();
	}
	m_sockets.clear();
	temp_sockets.clear();
	m_listening_sockets.clear();
	m_ready2read_sockets.clear();
	m_sended_sockets.clear();
	m_closed_sockets.clear();
	m_connected_sockets.clear();
	m_timeout_sockets.clear();
	m_unresolved_sockets.clear();
#ifdef BITTORRENT_DEBUG
	printf("NetworkManager destroyed\n");
#endif
}

/*
 * SyscallException
 * Exception::ERR_CODE_INVALID_FORMAT
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_add(std::string & ip, uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock) throw (Exception, SyscallException)
{
	Socket_add(ip.c_str(), port, assoc, sock);
}

/*
 * SyscallException
 * Exception::ERR_CODE_INVALID_FORMAT
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_add(const char * ip, uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock) throw (Exception, SyscallException)
{
	if(ip == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (port);
	int ret = inet_pton(AF_INET, ip, &sockaddr.sin_addr);
	if (ret == 0)
		throw Exception(Exception::ERR_CODE_INVALID_FORMAT);
	if (ret == -1)
		throw Exception(Exception::ERR_CODE_UNDEF);
	Socket_add(&sockaddr, assoc, sock);
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_add_domain(const char *domain_name, uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock) throw (Exception)
{
#ifdef BITTORRENT_DEBUG
	printf("Adding socket %s\n", domain_name);
#endif
	sock.reset(new socket_());
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	if(domain_name == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	sock->m_closed = false;
	sock->m_assoc = assoc;
	sock->m_connected = false;
	sock->m_peer.sin_port = htons (port);
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	sock->m_need2resolved = true;
	sock->m_domain = domain_name;
	sock->m_timer = 0;
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert(sock);
	pthread_mutex_unlock(&m_mutex_sockets);
#ifdef BITTORRENT_DEBUG
	printf("Socket is added %s\n", domain_name);
#endif
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_add_domain(std::string & domain_name, uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock) throw (Exception)
{
	Socket_add_domain(domain_name.c_str(), port, assoc, sock);
}

/*
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 * SyscallException
 */

void NetworkManager::Socket_add(struct sockaddr_in * addr, const SocketAssociation::ptr & assoc, Socket & sock) throw (Exception, SyscallException)
{
#ifdef BITTORRENT_DEBUG
	printf("Adding socket %s\n",  inet_ntoa(addr->sin_addr));
#endif
	sock.reset(new socket_());
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	if (addr == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	struct epoll_event event;
	sock->m_closed = false;
	sock->m_assoc = assoc;
	sock->m_connected = false;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
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
	memcpy((void *)&sock->m_peer, (void *)addr, sizeof(struct sockaddr_in));
	if (connect(sock->m_socket, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1)
	{
		if (errno == EINPROGRESS)
		{
			sock->m_state = STATE_CONNECTION;
			goto end;
		}
		int err = errno;
		close(sock->m_socket);
		sock.reset();
		throw SyscallException(err);
	}
	sock->m_state = STATE_TRANSMISSION;
	m_connected_sockets.insert(sock);
	sock->m_connected = true;
end:
	sock->m_timer = time(NULL);
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert(sock);
	pthread_mutex_unlock(&m_mutex_sockets);
#ifdef BITTORRENT_DEBUG
	printf("Socket is added %d %s\n", sock->m_socket, inet_ntoa(sock->m_peer.sin_addr));
#endif
}

/*
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 * SyscallException
 */

void NetworkManager::Socket_add(int sock_fd, struct sockaddr_in * addr, const SocketAssociation::ptr & assoc, Socket & sock) throw (Exception, SyscallException)
{
	sock.reset(new socket_());
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	if (addr == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	struct epoll_event event;
	sock->m_closed = false;
	sock->m_assoc = assoc;
	sock->m_connected = true;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	sock->m_socket = sock_fd;
	if (addr)
		memcpy((void *)&sock->m_peer, addr, sizeof(struct sockaddr_in));
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
	sock->m_state = STATE_TRANSMISSION;

	sock->m_timer = time(NULL);
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert(sock);
	pthread_mutex_unlock(&m_mutex_sockets);
}

/*
 * SyscallException
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::ListenSocket_add(sockaddr_in * addr, const SocketAssociation::ptr & assoc, Socket & sock)  throw (Exception, SyscallException)
{
	struct epoll_event event;
	const unsigned int yes = 1;
	sock.reset(new socket_());
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);
	if (addr == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);

	sock->m_closed = false;
	sock->m_assoc = assoc;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));

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

	memcpy((void *)&sock->m_peer, (void *)addr, sizeof(struct sockaddr_in));
	if (bind (sock->m_socket, (const struct sockaddr *) addr, sizeof (struct sockaddr)))
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

	sock->m_state = STATE_LISTEN;
	sock->m_timer = time(NULL);
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert(sock);
	m_listening_sockets.insert(sock);
	pthread_mutex_unlock(&m_mutex_sockets);
}

/*
 * SyscallException
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::ListenSocket_add(uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock)  throw (Exception, SyscallException)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	if (inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr) == -1)
		throw Exception(Exception::ERR_CODE_UNDEF);
	ListenSocket_add(&addr, assoc, sock);
}

/*
 * SyscallException
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::ListenSocket_add(uint16_t port, in_addr * addr, const SocketAssociation::ptr & assoc, Socket & sock)  throw (Exception, SyscallException)
{
	if (addr == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (port);
	memcpy(&sockaddr.sin_addr, addr, sizeof(in_addr));
	ListenSocket_add(&sockaddr, assoc, sock);
}

/*
 * SyscallException
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_INVALID_FORMAT
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::ListenSocket_add(uint16_t port, const char * ip, const SocketAssociation::ptr & assoc, Socket & sock)  throw (Exception, SyscallException)
{
	if (ip == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	int ret = inet_pton(AF_INET, ip, &addr.sin_addr);
	if (ret == 0)
		throw Exception(Exception::ERR_CODE_INVALID_FORMAT);
	if (ret == -1)
		throw Exception(Exception::ERR_CODE_UNDEF);
	ListenSocket_add(&addr, assoc, sock);
}

/*
 * SyscallException
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_INVALID_FORMAT
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::ListenSocket_add(uint16_t port, const std::string & ip, const SocketAssociation::ptr & assoc, Socket & sock)  throw (Exception, SyscallException)
{
	ListenSocket_add(port, ip.c_str(), assoc, sock);
}

/*
 * SyscallException
 */

int NetworkManager::clock()  throw (SyscallException)
{
	struct epoll_event events[MAX_EPOLL_EVENT];
	int nfds = epoll_wait(m_epoll_fd, events, MAX_EPOLL_EVENT, 80);
	if (nfds == -1)
	{
		throw SyscallException();
	}
	pthread_mutex_lock(&m_mutex_sockets);
	for(int i = 0; i < nfds; i++)
	{
		Socket  sock = ((socket_ *)events[i].data.ptr)->shared_from_this();
		sock->m_timer = time(NULL);
		bool need2send = buffer_remain(&sock->m_send_buffer) > 0;
		bool connected = sock->m_connected;

		try
		{
			handle_event(sock.get(), events[i].events);
		}
		catch(SyscallException & e)
		{
			sock->m_errno = e.get_errno();
		}

		if (buffer_remain(&sock->m_recv_buffer) > 0)
			m_ready2read_sockets.insert(sock);

		if (need2send && buffer_remain(&sock->m_send_buffer) == 0)
			m_sended_sockets.insert(sock);

		if (!connected && sock->m_connected)
			m_connected_sockets.insert(sock);

		if (sock->m_closed)
			m_closed_sockets.insert(sock);
	}
	pthread_mutex_unlock(&m_mutex_sockets);
	return ERR_NO_ERROR;
}

void NetworkManager::notify()
{
	pthread_mutex_lock(&m_mutex_sockets);
	for(socket_set_iter listening_sockets_iter = m_listening_sockets.begin();
			listening_sockets_iter != m_listening_sockets.end(); ++listening_sockets_iter)
	{
		for(socket_set_iter accepted_sockets_iter = (*listening_sockets_iter)->m_accepted_sockets.begin();
				accepted_sockets_iter != (*listening_sockets_iter)->m_accepted_sockets.end(); ++accepted_sockets_iter)
			if ((*listening_sockets_iter)->m_assoc != NULL)
			{
				(*listening_sockets_iter)->m_assoc->event_sock_accepted(*listening_sockets_iter, *accepted_sockets_iter);
				(*listening_sockets_iter)->m_accepted_sockets.erase(accepted_sockets_iter);
			}
	}

	socket_set temp_ready2read_sockets = m_ready2read_sockets;
	for(socket_set_iter iter = temp_ready2read_sockets.begin(); iter != temp_ready2read_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
			(*iter)->m_assoc->event_sock_ready2read(*iter);
	}

	socket_set temp_sended_sockets = m_sended_sockets;
	for(socket_set_iter iter = temp_sended_sockets.begin(); iter != temp_sended_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_sended(*iter);
			m_sended_sockets.erase(*iter);
		}
	}

	socket_set temp_connected_sockets = m_connected_sockets;
	for(socket_set_iter iter = temp_connected_sockets.begin(); iter != temp_connected_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_connected(*iter);
			m_connected_sockets.erase(*iter);
		}
	}

	socket_set temp_timeout_sockets = m_timeout_sockets;
	for(socket_set_iter iter = temp_timeout_sockets.begin(); iter != temp_timeout_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_timeout(*iter);
			m_timeout_sockets.erase(*iter);
		}
	}

	socket_set temp_unresolved_sockets = m_unresolved_sockets;
	for(socket_set_iter iter = temp_unresolved_sockets.begin(); iter != temp_unresolved_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_unresolved(*iter);
			m_unresolved_sockets.erase(*iter);
		}
	}

	socket_set temp_closed_sockets = m_closed_sockets;
	for(socket_set_iter iter = temp_closed_sockets.begin(); iter != temp_closed_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_closed(*iter);
			m_closed_sockets.erase(*iter);
		}
	}
	pthread_mutex_unlock(&m_mutex_sockets);
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_SOCKET_CLOSED
 * Exception::ERR_CODE_SOCKET_CAN_NOT_SEND
 */

size_t NetworkManager::Socket_send(Socket & sock, const void * data, size_t len, bool full)  throw (Exception)
{
	if (sock == NULL  || data == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	if (sock->m_closed)
	{
		m_closed_sockets.insert(sock);
		throw Exception(Exception::ERR_CODE_SOCKET_CLOSED);
	}
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
 */

size_t NetworkManager::Socket_recv(Socket & sock, void * data, size_t len) throw (Exception)
{
	if (sock == NULL || data == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);

	if (len == 0)
		return 0;
	size_t l = sock->m_recv_buffer.length - sock->m_recv_buffer.pos;
	if (l > len)
		l = len;
	memcpy(data, sock->m_recv_buffer.data + sock->m_recv_buffer.pos, l);
	sock->m_recv_buffer.pos += l;
	if (buffer_remain(&sock->m_recv_buffer) == 0)
	{
		sock->m_recv_buffer.length = 0;
		sock->m_recv_buffer.pos = 0;
	}
	if (buffer_remain(&sock->m_recv_buffer) > 0)
		m_ready2read_sockets.insert(sock);
	else
		m_ready2read_sockets.erase(sock);
	if (sock->m_closed)
		m_closed_sockets.insert(sock);
	return l;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

ssize_t NetworkManager::Socket_datalen(Socket & sock) throw (Exception)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	int len = buffer_remain(&sock->m_recv_buffer);
	if (len > 0)
		m_ready2read_sockets.insert(sock);
	else
		m_ready2read_sockets.erase(sock);
	if (sock->m_closed)
		m_closed_sockets.insert(sock);
	return len;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_closed(Socket & sock, bool * closed) throw (Exception)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	*closed = sock->m_closed;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_close(Socket & sock) throw (Exception)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	close(sock->m_socket);
	sock->m_closed = true;
	sock->m_connected = false;
	m_closed_sockets.insert(sock);
}

void NetworkManager::Socket_delete(Socket & sock)
{
	if (sock == NULL)
		return;
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.erase(sock);
	m_ready2read_sockets.erase(sock);
	m_sended_sockets.erase(sock);
	m_closed_sockets.erase(sock);
	m_connected_sockets.erase(sock);
	m_timeout_sockets.erase(sock);
	m_unresolved_sockets.erase(sock);
	m_listening_sockets.erase(sock);
	close(sock->m_socket);
	sock->m_need2delete = true;
	sock.reset();
	pthread_mutex_unlock(&m_mutex_sockets);
	return;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_set_assoc(Socket & sock, const SocketAssociation::ptr & assoc) throw (Exception)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	sock->m_assoc = assoc;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_get_assoc(Socket & sock, SocketAssociation::ptr & assoc) throw (Exception)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	assoc =  sock->m_assoc;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

double NetworkManager::Socket_get_rx_speed(Socket & sock) throw (Exception)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	double time = get_time();
	double speed = (double)sock->m_rx / (time - sock->m_rx_last_time);
	return speed;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

double NetworkManager::Socket_get_tx_speed(Socket & sock) throw (Exception)
{
	if (sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	double time = get_time();
	double speed = (double)sock->m_tx / (time - sock->m_tx_last_time);
	return speed;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

void NetworkManager::Socket_get_addr(Socket & sock, sockaddr_in * addr)  throw (Exception)
{
	if (addr == NULL || sock == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	memcpy(addr, &sock->m_peer, sizeof(sockaddr_in));
}

/*
 * SyscallException
 */

void NetworkManager::handle_event(socket_ * sock, uint32_t events) throw (SyscallException)
{
	if ((events & EPOLLOUT) != 0)
	{
		if (sock->m_state == STATE_CONNECTION)
		{
			try
			{
				handler_connection(sock);

				uint32_t new_events = EPOLLIN;
				if (buffer_remain(&sock->m_send_buffer) != 0)
					new_events |= EPOLLOUT;
				epoll_mod(sock, new_events);
			}
			catch(SyscallException & e)
			{
				close(sock->m_socket);
				sock->m_closed = true;
				sock->m_connected = false;
				throw SyscallException(e);
			}
		}
		if (sock->m_state == STATE_TRANSMISSION)
		{
			try
			{
				handler_transmission(sock, TRANSMISSION_OUT);
			}
			catch(SyscallException & e)
			{
				close(sock->m_socket);
				sock->m_closed = true;
				sock->m_connected = false;
				throw SyscallException(e);
			}
		}
	}
	if ((events & EPOLLIN) != 0)
	{
		if (sock->m_state == STATE_TRANSMISSION)
		{
			try
			{
				handler_transmission(sock, TRANSMISSION_IN);
			}
			catch(SyscallException & e)
			{
				close(sock->m_socket);
				sock->m_closed = true;
				sock->m_connected = false;
				throw SyscallException(e);
			}
		}
		if (sock->m_state == STATE_LISTEN)
		{
			int client_sock;
			struct sockaddr_in client_addr;
			socklen_t client_addr_len = sizeof(client_addr);

			memset(&client_addr, '\0', sizeof(client_addr));
			client_sock = accept4(sock->m_socket, (struct sockaddr *)&client_addr, &client_addr_len, SOCK_NONBLOCK);
			if (client_sock == -1)
			{
				if (errno != EAGAIN && errno != EWOULDBLOCK)
					return;
				throw SyscallException();
			}
			SocketAssociation::ptr assoc;
			Socket new_sock;
			Socket_add(client_sock, &client_addr, assoc, new_sock);
			sock->m_accepted_sockets.insert(new_sock);
		}
	}
}

/*
 * SyscallException
 */

void NetworkManager::handler_connection(socket_ * sock) throw (SyscallException)
{
	int connect_result;
	socklen_t opt_len = sizeof(int);
	//ошибка в errno
	if (getsockopt (sock->m_socket, SOL_SOCKET, SO_ERROR, &connect_result, &opt_len) == -1)
		throw SyscallException();

	if (connect_result != 0)
		throw SyscallException(connect_result);

	sock->m_connected = true;
	sock->m_state = STATE_TRANSMISSION;
}

/*
 * SyscallException
 */

void NetworkManager::handler_transmission(socket_ * sock, int transmission_type) throw (SyscallException)
{
	if (transmission_type == TRANSMISSION_OUT)
	{
		while(1)
		{
			ssize_t ret = sock->m_send_buffer.length - sock->m_send_buffer.pos;
			if (ret == 0)
			{
				sock->m_send_buffer.length = 0;
				sock->m_send_buffer.pos = 0;
				epollout(sock, false);
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
	if (transmission_type == TRANSMISSION_IN)
	{
		while(1)
		{
			char temp_buffer[BUFFER_SIZE];
			ssize_t free_in_buffer = BUFFER_SIZE - sock->m_recv_buffer.length;
			if (free_in_buffer == 0)
				return;
			ssize_t received = recv(sock->m_socket, temp_buffer, free_in_buffer, 0);
			if (received == -1 && !(errno == EAGAIN || errno==EWOULDBLOCK))
				throw SyscallException();
			if (received == -1 && (errno == EAGAIN || errno==EWOULDBLOCK))
				return;
			if (received == 0)
			{
				close(sock->m_socket);
				sock->m_closed = true;
				sock->m_connected = false;
				return;
			}
			memcpy(sock->m_recv_buffer.data + sock->m_recv_buffer.length, temp_buffer, received);
			sock->m_recv_buffer.length += received;
			//замер скорости
			sock->m_rx += received;
		}
	}
}

/*
 * SyscallException
 */

void NetworkManager::epoll_mod(socket_ * sock) throw (SyscallException)
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

void NetworkManager::epoll_mod(socket_ * sock, uint32_t events) throw (SyscallException)
{
	struct epoll_event event;
	event.data.ptr = (void*)sock;
	event.events = events;
	sock->m_epoll_events = events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, sock->m_socket, &event) == -1)
		throw SyscallException();
}

/*
 * SyscallException
 */

void NetworkManager::epollin(socket_ * sock, bool on_off) throw (SyscallException)
{
	if (on_off == true)
	{
		if ((sock->m_epoll_events & EPOLLIN) != 0)
			return;
		sock->m_epoll_events |= EPOLLIN;
		epoll_mod(sock);
	}
	if (on_off == false)
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

void NetworkManager::epollout(socket_ * sock, bool on_off) throw (SyscallException)
{
	if (on_off == true)
	{
		if ((sock->m_epoll_events & EPOLLOUT) != 0)
			return;
		sock->m_epoll_events |= EPOLLOUT;
		epoll_mod(sock);
	}
	if (on_off == false)
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

void NetworkManager::ConnectResolvedSocket(Socket & sock) throw (SyscallException)
{
	struct epoll_event event;
	sock->m_connected = false;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
		throw SyscallException();

	event.data.ptr=(void*)sock.get();
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
			sock->m_state = STATE_CONNECTION;
			goto end;
		}
		close(sock->m_socket);
		int err = errno;
		close(sock->m_socket);
		throw SyscallException(err);
	}
	sock->m_state = STATE_TRANSMISSION;
	m_connected_sockets.insert(sock);
	sock->m_connected = true;
end:
	sock->m_timer = time(NULL);
}

/*
 * SyscallException
 */

void * NetworkManager::timeout_thread(void * arg)
{
	int ret = 1;
	NetworkManager * nm = (NetworkManager*)arg;
	std::list<Socket> sock2resolve;
	while(!nm->m_thread_stop)
	{
		//printf("timeout_thread loop\n");
		pthread_mutex_lock(&nm->m_mutex_sockets);
		for(socket_set_iter iter = nm->m_sockets.begin(); iter != nm->m_sockets.end(); ++iter)
		{
			Socket sock = *iter;

			if (sock->m_timer != 0 &&
				time(NULL) - sock->m_timer >= 10 &&
				(sock->m_state == STATE_CONNECTION || (sock->m_epoll_events & EPOLLIN) != 0) &&
				!sock->m_closed)
			{
				nm->m_timeout_sockets.insert(sock);
			}
			if (sock->m_need2resolved)
				sock2resolve.push_back(sock);
		}
		pthread_mutex_unlock(&nm->m_mutex_sockets);

		//for(std::list<Socket>::iterator iter = sock2resolve.begin(); iter != sock2resolve.end(); ++iter)
		while(!sock2resolve.empty())
		{
			Socket sock = sock2resolve.front();
			sock2resolve.pop_front();
			sock->m_need2resolved = false;
			struct sockaddr_in addr;
			memcpy(&addr, &sock->m_peer, sizeof(addr));
			try
			{
				DomainNameResolver dnr(sock->m_domain, &addr);
				memcpy(&sock->m_peer, &addr, sizeof(addr));
				pthread_mutex_lock(&nm->m_mutex_sockets);
				if (sock->m_need2delete)
				{
					pthread_mutex_unlock(&nm->m_mutex_sockets);
					continue;
				}
				try
				{
					nm->ConnectResolvedSocket(sock);
				}
				catch(SyscallException & e)
				{
					pthread_mutex_unlock(&nm->m_mutex_sockets);
					throw SyscallException(e);
				}
				pthread_mutex_unlock(&nm->m_mutex_sockets);
			}
			catch (...)
			{
#ifdef BITTORRENT_DEBUG
	printf("DomainNameResolver is can not resolve %s\n", sock->m_domain.c_str());
#endif
				pthread_mutex_lock(&nm->m_mutex_sockets);
				if (!sock->m_need2delete)
					nm->m_unresolved_sockets.insert(sock);
				pthread_mutex_unlock(&nm->m_mutex_sockets);
			}
		}

		usleep(500);
	}
	return (void*)ret;
}

}
}
