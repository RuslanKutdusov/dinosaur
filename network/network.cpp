/*
 * network.cpp
 *
 *  Created on: 21.01.2012
 *      Author: ruslan
 */
#include "network.h"
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
	m_thread = 0;
	m_thread_stop = false;
}

int NetworkManager::Init()
{
	m_epoll_fd = epoll_create(10);
	if (m_epoll_fd == -1)
		return ERR_SYSCALL_ERROR;
	m_thread_stop = false;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutexattr_t   mta;
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex_sockets, &mta);
	pthread_mutex_unlock(&m_mutex_sockets);
	//pthread_mutex_init(&m_mutex_timeout_sockets, NULL);
	if (pthread_create(&m_thread, &attr, NetworkManager::timeout_thread, (void *)this))
		return ERR_SYSCALL_ERROR;
	pthread_attr_destroy(&attr);
	return ERR_NO_ERROR;
}

NetworkManager::~NetworkManager()
{
	//pthread_mutex_destroy(&m_mutex_sockets);
	printf("NetworkManager destructor\n");
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
		//std::cout<<"a"<<std::endl;
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
	std::cout<<"NetworkManager destroyed\n";
}

int NetworkManager::Socket_add(std::string & ip, uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock)
{
	return Socket_add(ip.c_str(), port, assoc, sock);
}

int NetworkManager::Socket_add(const char * ip, uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock)
{
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (port);
	if (inet_pton(AF_INET, ip, &sockaddr.sin_addr) == 0)
		return ERR_BAD_ARG;
	return Socket_add(&sockaddr, assoc, sock);
}

int NetworkManager::Socket_add_domain(const char *domain_name, uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock)
{
	sock.reset(new socket_());
	if (sock == NULL || domain_name == NULL)
		return ERR_INTERNAL;;
	sock->m_closed = false;
	sock->m_assoc = assoc;
	sock->m_connected = false;
	sock->m_peer.sin_port = htons (port);
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	sock->m_need2resolved = true;
	sock->m_domain = domain_name;
	sock->m_timer = 0;
	//if (lock_mutex)
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert(sock);
	//if (lock_mutex)
	pthread_mutex_unlock(&m_mutex_sockets);
	return ERR_NO_ERROR;
}

int NetworkManager::Socket_add_domain(std::string & domain_name, uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock)
{
	return Socket_add_domain(domain_name.c_str(), port, assoc, sock);
}

int NetworkManager::Socket_add(struct sockaddr_in * addr, const SocketAssociation::ptr & assoc, Socket & sock)
{
	sock.reset(new socket_());
	if (sock == NULL)
		return ERR_INTERNAL;
	struct epoll_event event;
	sock->m_closed = false;
	sock->m_assoc = assoc;
	sock->m_connected = false;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	{
		sock.reset();
		return ERR_INTERNAL;
	}
	event.data.ptr=(void*)sock.get();
	event.events = EPOLLOUT;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		close(sock->m_socket);
		sock.reset();
		return ERR_INTERNAL;
	}
	memcpy((void *)&sock->m_peer, (void *)addr, sizeof(struct sockaddr_in));
	if (connect(sock->m_socket, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1)
	{
		if (errno == EINPROGRESS)
		{
			sock->m_state = STATE_CONNECTION;
			goto end;
		}
		close(sock->m_socket);
		sock.reset();
		return ERR_INTERNAL;
	}
	sock->m_state = STATE_TRANSMISSION;
	m_connected_sockets.insert(sock);
	sock->m_connected = true;
end:
	sock->m_timer = time(NULL);
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert(sock);
	pthread_mutex_unlock(&m_mutex_sockets);
	return ERR_NO_ERROR;
}

int NetworkManager::Socket_add(int sock_fd, struct sockaddr_in * addr, const SocketAssociation::ptr & assoc, Socket & sock)
{
	sock.reset(new socket_());
	if (sock == NULL)
		return ERR_INTERNAL;
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
		close(sock->m_socket);
		sock.reset();
		return ERR_INTERNAL;
	}
	sock->m_state = STATE_TRANSMISSION;

	sock->m_timer = time(NULL);
	//if (lock_mutex)
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert(sock);
	//if (lock_mutex)
	pthread_mutex_unlock(&m_mutex_sockets);
	return ERR_NO_ERROR;
}

int NetworkManager::ListenSocket_add(uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock)
{
	struct epoll_event event;
	struct sockaddr_in addr;
	const unsigned int yes = 1;
	//struct socket_ * sock = (struct socket_ *)malloc(sizeof(struct socket_));
	sock.reset(new socket_());
	if (sock == NULL)
		return ERR_INTERNAL;

	sock->m_closed = false;
	sock->m_assoc = assoc;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));

	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	{
		sock.reset();
		return ERR_INTERNAL;
	}

	if (setsockopt (sock->m_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) == -1)
	{
		close(sock->m_socket);
		sock.reset();
		return ERR_INTERNAL;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	inet_pton(AF_INET,"0.0.0.0",&addr.sin_addr);
	memcpy((void *)&sock->m_peer, (void *)&addr, sizeof(struct sockaddr_in));
	if (bind (sock->m_socket, (const struct sockaddr *) &addr, sizeof (addr)))
	{
		close(sock->m_socket);
		sock.reset();
		return ERR_INTERNAL;
	}

	if (listen (sock->m_socket, SOMAXCONN) == -1)
	{
		close(sock->m_socket);
		sock.reset();
		return ERR_INTERNAL;
	}

	event.data.ptr = (void*)sock.get();
	event.events = EPOLLIN;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		close(sock->m_socket);
		sock.reset();
		return ERR_INTERNAL;
	}

	sock->m_state = STATE_LISTEN;
	sock->m_timer = time(NULL);
	//if (lock_mutex)
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert(sock);
	m_listening_sockets.insert(sock);
	//if (lock_mutex)
	pthread_mutex_unlock(&m_mutex_sockets);
	return ERR_NO_ERROR;
}

int NetworkManager::clock()
{
	struct epoll_event events[MAX_EPOLL_EVENT];
	int nfds = epoll_wait(m_epoll_fd, events, MAX_EPOLL_EVENT, 80);
	if (nfds == -1)
	{
		return ERR_SYSCALL_ERROR;
	}
	pthread_mutex_lock(&m_mutex_sockets);
	for(int i = 0; i < nfds; i++)
	{
		Socket  sock = ((socket_ *)events[i].data.ptr)->shared_from_this();
		sock->m_timer = time(NULL);
		bool need2send = buffer_remain(&sock->m_send_buffer) > 0;
		bool connected = sock->m_connected;

		if (handle_event(sock.get(), events[i].events) == ERR_SYSCALL_ERROR)
			sock->m_errno = errno;

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

	pthread_mutex_lock(&m_mutex_sockets);
	socket_set temp_ready2read_sockets = m_ready2read_sockets;
	socket_set temp_sended_sockets = m_sended_sockets;
	socket_set temp_closed_sockets = m_closed_sockets;
	socket_set temp_connected_sockets = m_connected_sockets;
	socket_set temp_timeout_sockets = m_timeout_sockets;
	socket_set temp_unresolved_sockets = m_unresolved_sockets;
	pthread_mutex_unlock(&m_mutex_sockets);

	for(socket_set_iter iter = temp_ready2read_sockets.begin(); iter != temp_ready2read_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
			(*iter)->m_assoc->event_sock_ready2read(*iter);
	}

	for(socket_set_iter iter = temp_sended_sockets.begin(); iter != temp_sended_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_sended(*iter);
			m_sended_sockets.erase(*iter);
		}
	}

	for(socket_set_iter iter = temp_connected_sockets.begin(); iter != temp_connected_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_connected(*iter);
			m_connected_sockets.erase(*iter);
		}
	}

	for(socket_set_iter iter = temp_timeout_sockets.begin(); iter != temp_timeout_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_timeout(*iter);
			m_timeout_sockets.erase(*iter);
		}
	}

	for(socket_set_iter iter = temp_unresolved_sockets.begin(); iter != temp_unresolved_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_unresolved(*iter);
			m_unresolved_sockets.erase(*iter);
		}
	}

	for(socket_set_iter iter = temp_closed_sockets.begin(); iter != temp_closed_sockets.end(); ++iter)
	{
		if ((*iter)->m_assoc != NULL)
		{
			(*iter)->m_assoc->event_sock_closed(*iter);
			m_closed_sockets.erase(*iter);
		}
	}
}

int NetworkManager::Socket_send(Socket & sock, const void * data, size_t len, bool full)
{
	if (sock == NULL)
		return ERR_BAD_ARG;
	if (sock->m_closed)
	{
		m_closed_sockets.insert(sock);
		return ERR_BAD_ARG;
	}
	return _send(sock.get(),data,len, full);
}

int NetworkManager::Socket_recv(Socket & sock, void * data, size_t len)
{
	if (sock == NULL || data == NULL)
		return ERR_BAD_ARG;
	int ret =_recv(sock.get(), data, len);
	if (ret < ERR_NO_ERROR)
	{
		//m_ready2read_sockets.erase(id);
		return ERR_INTERNAL;
	}
	if (buffer_remain(&sock->m_recv_buffer) > 0)
		m_ready2read_sockets.insert(sock);
	else
		m_ready2read_sockets.erase(sock);
	if (sock->m_closed)
		m_closed_sockets.insert(sock);
	return ret;
}

ssize_t NetworkManager::Socket_datalen(Socket & sock)
{
	if (sock == NULL)
		return ERR_BAD_ARG;
	int len = buffer_remain(&sock->m_recv_buffer);
	if (len > 0)
		m_ready2read_sockets.insert(sock);
	else
		m_ready2read_sockets.erase(sock);
	if (sock->m_closed)
		m_closed_sockets.insert(sock);
	return len;
}

int NetworkManager::Socket_closed(Socket & sock, bool * closed)
{
	if (sock == NULL)
	{
		*closed = true;
		return ERR_BAD_ARG;
	}
	*closed = sock->m_closed;
	return ERR_NO_ERROR;
}

int NetworkManager::Socket_close(Socket & sock)
{
	if (sock == NULL)
		return ERR_BAD_ARG;
	_close(sock.get());
	if (sock->m_closed)
		m_closed_sockets.insert(sock);
	return ERR_NO_ERROR;
}

int NetworkManager::Socket_delete(Socket & sock)
{
	if (sock == NULL)
		return ERR_BAD_ARG;
	pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.erase(sock);
	m_ready2read_sockets.erase(sock);
	m_sended_sockets.erase(sock);
	m_closed_sockets.erase(sock);
	m_connected_sockets.erase(sock);
	m_timeout_sockets.erase(sock);
	m_unresolved_sockets.erase(sock);
	m_listening_sockets.erase(sock);
	_close(sock.get());
	sock->m_need2delete = true;
	sock.reset();
	pthread_mutex_unlock(&m_mutex_sockets);
	return ERR_NO_ERROR;
}

int NetworkManager::Socket_set_assoc(Socket & sock, const SocketAssociation::ptr & assoc)
{
	if (sock == NULL)
		return ERR_BAD_ARG;
	sock->m_assoc = assoc;
	return ERR_NO_ERROR;
}

int NetworkManager::Socket_get_assoc(Socket & sock, SocketAssociation::ptr & assoc)
{
	if (sock == NULL)
		return ERR_BAD_ARG;
	assoc =  sock->m_assoc;
	return ERR_NO_ERROR;
}

double NetworkManager::Socket_get_rx_speed(Socket & sock)
{
	if (sock == NULL)
		return 0.0f;
	double time = get_time();
	double speed = (double)sock->m_rx / (time - sock->m_rx_last_time);
	return speed;
}

double NetworkManager::Socket_get_tx_speed(Socket & sock)
{
	if (sock == NULL)
		return 0.0f;
	double time = get_time();
	double speed = (double)sock->m_tx / (time - sock->m_tx_last_time);
	return speed;
}

int NetworkManager::handle_event(socket_ * sock, uint32_t events)
{
	int ret;
	if ((events & EPOLLOUT) != 0)
	{
		if (sock->m_state == STATE_CONNECTION)
		{
			ret = handler_connection(sock);
			if (ret != ERR_NO_ERROR)
			{
				close(sock->m_socket);
				sock->m_closed = true;
				sock->m_connected = false;
				return ret;
			}
			uint32_t new_events = EPOLLIN;
			if (buffer_remain(&sock->m_send_buffer) != 0)
				new_events |= EPOLLOUT;
			ret = epoll_mod(sock, new_events);
			if (ret != ERR_NO_ERROR)
			{
				close(sock->m_socket);
				sock->m_closed = true;
				sock->m_connected = false;
				return ret;
			}
			//return ERR_NO_ERROR;//?????????????????????????????????///
		}
		if (sock->m_state == STATE_TRANSMISSION)
		{
			ret = handler_transmission(sock, TRANSMISSION_OUT);
			if (ret != ERR_NO_ERROR)
			{
				close(sock->m_socket);
				sock->m_closed = true;
				sock->m_connected = false;
				return ret;
			}
		}
	}
	if ((events & EPOLLIN) != 0)
	{
		if (sock->m_state == STATE_TRANSMISSION)
		{
			ret = handler_transmission(sock, TRANSMISSION_IN);
			if (ret != ERR_NO_ERROR)
			{
				close(sock->m_socket);
				sock->m_closed = true;
				sock->m_connected = false;
				return ret;
			}
		}
		if (sock->m_state == STATE_LISTEN)
		{
			int client_sock;
			struct sockaddr_in client_addr;
			socklen_t client_addr_len = sizeof(client_addr);

			memset(&client_addr,'\0',sizeof(client_addr));
			client_sock = accept4(sock->m_socket, (struct sockaddr *)&client_addr, &client_addr_len, SOCK_NONBLOCK);
			if (client_sock == -1)
			{
				if (errno != EAGAIN && errno != EWOULDBLOCK)
					return ERR_NO_ERROR;
				return ERR_SYSCALL_ERROR;
			}
			SocketAssociation::ptr assoc;
			Socket new_sock;
			Socket_add(client_sock, &client_addr, assoc, new_sock);
			if (new_sock == NULL)
				return ERR_INTERNAL;
			sock->m_accepted_sockets.insert(new_sock);
			return ERR_NO_ERROR;
		}
	}
	return ERR_NO_ERROR;
}

int NetworkManager::handler_connection(socket_ * sock)
{
	int connect_result;
	socklen_t opt_len = sizeof(int);
	//ошибка в errno
	if (getsockopt (sock->m_socket, SOL_SOCKET, SO_ERROR, &connect_result, &opt_len) == -1)
		return ERR_SYSCALL_ERROR;

	if (connect_result != 0)
	{
		errno = connect_result;
		return ERR_SYSCALL_ERROR;
	}
	sock->m_connected = true;
	sock->m_state = STATE_TRANSMISSION;
	return ERR_NO_ERROR;
}

int NetworkManager::handler_transmission(socket_ * sock, int transmission_type)
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
				return epollout(sock, false);
			}
			ssize_t sended = send(sock->m_socket, sock->m_send_buffer.data + sock->m_send_buffer.pos, ret, MSG_NOSIGNAL);
			if (sended == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
				break;
			if (sended == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK))
				return ERR_SYSCALL_ERROR;
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
				return ERR_NO_ERROR;
			ssize_t received = recv(sock->m_socket, temp_buffer, free_in_buffer, 0);
			if (received == -1 && !(errno == EAGAIN || errno==EWOULDBLOCK))
				return ERR_SYSCALL_ERROR;
			if (received == -1 && (errno == EAGAIN || errno==EWOULDBLOCK))
				return ERR_NO_ERROR;
			if (received == 0)
			{
				close(sock->m_socket);
				sock->m_closed = true;
				sock->m_connected = false;
				return ERR_NO_ERROR;
			}
			memcpy(sock->m_recv_buffer.data + sock->m_recv_buffer.length, temp_buffer, received);
			sock->m_recv_buffer.length += received;
			//замер скорости
			sock->m_rx += received;
		}
	}
	return ERR_NO_ERROR;
}

int NetworkManager::_send(socket_ * sock, const void * data,size_t len, bool full)
{
	if (sock == NULL || data == NULL || len <= 0 || sock->m_closed)
		return ERR_BAD_ARG;
	size_t can_send = BUFFER_SIZE - sock->m_send_buffer.length;
	if (full && can_send < len)
		return ERR_INTERNAL;
	can_send = can_send < len ? can_send : len;
	memcpy(sock->m_send_buffer.data + sock->m_send_buffer.length, data, can_send);
	sock->m_send_buffer.length += can_send;
	if (epollout(sock, true) != ERR_NO_ERROR)
		return ERR_INTERNAL;
	return can_send;
}

int NetworkManager::_recv(socket_ * sock, void * data, size_t len)
{
	if (sock == NULL || len <= 0 || data == NULL)
		return ERR_BAD_ARG;
	size_t l = sock->m_recv_buffer.length - sock->m_recv_buffer.pos;
	if (l > len)
		l = len;
	//if (sock->m_recv_buffer.length - sock->m_recv_buffer.pos < len)
		//return ERR_INTERNAL;
	memcpy(data, sock->m_recv_buffer.data + sock->m_recv_buffer.pos, l);
	sock->m_recv_buffer.pos += l;
	if (buffer_remain(&sock->m_recv_buffer) == 0)
	{
		sock->m_recv_buffer.length = 0;
		sock->m_recv_buffer.pos = 0;
	}
	return l;
}

int NetworkManager::_close(socket_ * sock)
{
	close(sock->m_socket);
	sock->m_closed = true;
	sock->m_connected = false;
	return ERR_NO_ERROR;
}

int NetworkManager::epoll_mod(socket_ * sock)
{
	struct epoll_event event;
	event.data.ptr = (void*)sock;
	event.events = sock->m_epoll_events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, sock->m_socket, &event) == -1)
		return ERR_SYSCALL_ERROR;
	return ERR_NO_ERROR;
}

int NetworkManager::epoll_mod(socket_ * sock, uint32_t events)
{
	struct epoll_event event;
	event.data.ptr = (void*)sock;
	event.events = events;
	sock->m_epoll_events = events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, sock->m_socket, &event) == -1)
	{
		return ERR_SYSCALL_ERROR;
	}
	return ERR_NO_ERROR;
}

int NetworkManager::epollin(socket_ * sock, bool on_off)
{
	if (on_off == true)
	{
		if ((sock->m_epoll_events & EPOLLIN) != 0)
			return ERR_NO_ERROR;
		sock->m_epoll_events |= EPOLLIN;
		return epoll_mod(sock);
	}
	if (on_off == false)
	{
		if ((sock->m_epoll_events & EPOLLIN) == 0)
			return ERR_NO_ERROR;
		sock->m_epoll_events ^= EPOLLIN;
		return epoll_mod(sock);
	}
	return ERR_NO_ERROR;
}

int NetworkManager::epollout(socket_ * sock, bool on_off)
{
	if (on_off == true)
	{
		if ((sock->m_epoll_events & EPOLLOUT) != 0)
			return ERR_NO_ERROR;
		sock->m_epoll_events |= EPOLLOUT;
		return epoll_mod(sock);
	}
	if (on_off == false)
	{
		if ((sock->m_epoll_events & EPOLLOUT) == 0)
			return ERR_NO_ERROR;
		sock->m_epoll_events ^= EPOLLOUT;
		return epoll_mod(sock);
	}
	return ERR_NO_ERROR;
}

int NetworkManager::ConnectResolvedSocket(Socket & sock)
{
	if (sock == NULL)
		return ERR_NULL_REF;
	struct epoll_event event;
	sock->m_connected = false;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
		return ERR_SYSCALL_ERROR;

	event.data.ptr=(void*)sock.get();
	event.events = EPOLLOUT;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		close(sock->m_socket);
		return ERR_SYSCALL_ERROR;
	}
	if (connect(sock->m_socket, (const struct sockaddr *)&sock->m_peer, sizeof(struct sockaddr_in)) == -1)
	{
		if (errno == EINPROGRESS)
		{
			sock->m_state = STATE_CONNECTION;
			goto end;
		}
		close(sock->m_socket);
		return ERR_SYSCALL_ERROR;
	}
	sock->m_state = STATE_TRANSMISSION;
	m_connected_sockets.insert(sock);
	sock->m_connected = true;
end:
	sock->m_timer = time(NULL);
	return ERR_NO_ERROR;
}

DomainNameResolver::DomainNameResolver(std::string & domain, struct sockaddr_in * addr)
{
	if (addr == NULL || domain.length() == 0)
		throw Exception("Invalid argument");
	m_domain = domain;
	m_addr = addr;
	if (resolve() != ERR_NO_ERROR)
			throw Exception("Can not resolve");
}

DomainNameResolver::DomainNameResolver(const char * domain, struct sockaddr_in * addr)
{
	if (addr == NULL || domain == NULL)
		throw Exception("Invalid argument");
	m_domain = domain;
	m_addr = addr;
	if (resolve() != ERR_NO_ERROR)
		throw Exception("Can not resolve");
}

int DomainNameResolver::addrinfo()
{
	struct addrinfo * res = NULL;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	//резолвим ipшник хоста
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	if (getaddrinfo(m_domain.c_str(), NULL, &hints, &res) == 0)
	{
		uint16_t port = m_addr->sin_port;
		memcpy(m_addr, res->ai_addr, sizeof(struct sockaddr_in));
		m_addr->sin_port = port;
		freeaddrinfo(res);
		return ERR_NO_ERROR;
	}
	else
	{
		//freeaddrinfo(res);
		return ERR_INTERNAL;
	}
}

int DomainNameResolver::resolve()
{
	if (addrinfo() != ERR_NO_ERROR)
	{
		if (split_domain_and_port() != ERR_NO_ERROR)
			return ERR_INTERNAL;
		return addrinfo();
	}
	return ERR_NO_ERROR;
}

int DomainNameResolver::split_domain_and_port()
{
	pcre *re;
	const char * error;
	int erroffset;
	char pattern2[] = "^(.+?):(\\d+)$";
	re = pcre_compile(pattern2, 0, &error, &erroffset, NULL);
	if (!re)
		return ERR_INTERNAL;

	int ovector[1024];
	int count = pcre_exec(re, NULL, m_domain.c_str(), m_domain.length(), 0, 0, ovector, 1024);
	if (count <= 0)
	{
		pcre_free(re);
		return ERR_INTERNAL;
	}
	pcre_free(re);
	std::string port_str = m_domain.substr(ovector[4], ovector[5] - ovector[4]);
	m_domain = m_domain.substr(0, ovector[3] - ovector[2]);

	unsigned int p;
	sscanf(port_str.c_str(), "%u", &p);
	m_addr->sin_port = htons(p);
	return ERR_NO_ERROR;
}

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
				if (nm->ConnectResolvedSocket(sock) != ERR_NO_ERROR)
					nm->m_unresolved_sockets.insert(sock);
				pthread_mutex_unlock(&nm->m_mutex_sockets);
			}
			catch (Exception & e)
			{
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
