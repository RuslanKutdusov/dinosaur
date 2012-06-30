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
	pthread_mutex_init(&m_mutex_sockets, NULL);
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
	for(socket_set_iter iter = m_sockets.begin(); iter != m_sockets.end(); ++iter)
	{
		socket_* sock = (socket_*)(*iter);
		delete sock;
	}
	std::cout<<"DONE\n";
	std::cout<<"NetworkManager destroyed\n";
}

socket_ * NetworkManager::Socket_add(std::string & ip, uint16_t port, void * assoc, bool lock_mutex)
{
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (port);
	inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);
	return Socket_add(&sockaddr, assoc, lock_mutex);
}

socket_ * NetworkManager::Socket_add(const char * ip, uint16_t port, void * assoc, bool lock_mutex)
{
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (port);
	if (inet_pton(AF_INET, ip, &sockaddr.sin_addr) <= 0)
		return NULL;
	return Socket_add(&sockaddr, assoc, lock_mutex);
}

socket_ * NetworkManager::Socket_add_domain(const char *domain_name, uint16_t port, void * assoc, bool lock_mutex )
{
	socket_ * sock = new socket_();
	if (sock == NULL || domain_name == NULL)
		return NULL;
	sock->m_closed = false;
	sock->m_assoc = assoc;
	sock->m_connected = false;
	sock->m_peer.sin_port = htons (port);
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	sock->m_need2resolved = true;
	sock->m_domain = domain_name;
	sock->m_timer = 0;
	if (lock_mutex)
		pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert((int)sock);
	if (lock_mutex)
		pthread_mutex_unlock(&m_mutex_sockets);
	return sock;
}

socket_ * NetworkManager::Socket_add_domain(std::string & domain_name, uint16_t port, void * assoc, bool lock_mutex )
{
	return Socket_add_domain(domain_name.c_str(), port, assoc, lock_mutex);
}

socket_ * NetworkManager::Socket_add(struct sockaddr_in * addr, void * assoc, bool lock_mutex)//
{
	//struct socket_ * sock = (struct socket_ *)malloc(sizeof(struct socket_));
	socket_ * sock = new socket_();
	if (sock == NULL)
		return NULL;
	struct epoll_event event;
	sock->m_closed = false;
	sock->m_assoc = assoc;
	sock->m_connected = false;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	{
		free(sock);
		return NULL;
	}
	event.data.ptr=(void*)sock;
	event.events = EPOLLOUT;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		close(sock->m_socket);
		free(sock);
		return NULL;
	}
	memcpy((void *)&sock->m_peer, (void *)addr, sizeof(struct sockaddr_in));
	if (connect(sock->m_socket, (const struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1)
	{
		if (errno == EINPROGRESS)
		{
			sock->m_state = STATE_CONNECTION;
			//m_sockets[sock->m_socket] = sock;
			//return sock->m_socket;
			goto end;
		}
		close(sock->m_socket);
		free(sock);
		return NULL;
	}
	sock->m_state = STATE_TRANSMISSION;
	m_connected_sockets.insert((int)sock);
	sock->m_connected = true;
end:
	sock->m_timer = time(NULL);
	if (lock_mutex)
		pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert((int)sock);
	if (lock_mutex)
		pthread_mutex_unlock(&m_mutex_sockets);
	return sock;
}

socket_ * NetworkManager::Socket_add(int sock_fd, void * assoc, struct sockaddr_in * addr, bool lock_mutex)
{
	//struct socket_ * sock = (struct socket_ *)malloc(sizeof(struct socket_));
	socket_ * sock = new socket_();
	if (sock == NULL)
		return NULL;
	struct epoll_event event;
	sock->m_closed = false;
	sock->m_assoc = assoc;
	sock->m_connected = true;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	sock->m_socket = sock_fd;
	if (addr)
		memcpy((void *)&sock->m_peer, addr, sizeof(struct sockaddr_in));
	event.data.ptr=(void*)sock;
	event.events = EPOLLIN;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		close(sock->m_socket);
		free(sock);
		return NULL;
	}
	sock->m_state = STATE_TRANSMISSION;

	sock->m_timer = time(NULL);
	if (lock_mutex)
		pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert((int)sock);
	if (lock_mutex)
		pthread_mutex_unlock(&m_mutex_sockets);
	return sock;
}

socket_ * NetworkManager::ListenSocket_add(uint16_t port, void * assoc, bool lock_mutex)
{
	struct epoll_event event;
	struct sockaddr_in addr;
	static const unsigned int yes = 1;
	//struct socket_ * sock = (struct socket_ *)malloc(sizeof(struct socket_));
	socket_ * sock = new socket_();
	if (sock == NULL)
		return NULL;

	sock->m_closed = false;
	sock->m_assoc = assoc;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));

	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
	{
		free(sock);
		return NULL;
	}

	if (setsockopt (sock->m_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) == -1)
	{
		close(sock->m_socket);
		free(sock);
		return NULL;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	inet_pton(AF_INET,"0.0.0.0",&addr.sin_addr);
	memcpy((void *)&sock->m_peer, (void *)&addr, sizeof(struct sockaddr_in));
	if (bind (sock->m_socket, (const struct sockaddr *) &addr, sizeof (addr)))
	{
		close(sock->m_socket);
		free(sock);
		return NULL;
	}

	if (listen (sock->m_socket, SOMAXCONN) == -1)
	{
		close(sock->m_socket);
		free(sock);
		return NULL;
	}

	event.data.ptr = (void*)sock;
	event.events = EPOLLIN;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		close(sock->m_socket);
		free(sock);
		return NULL;
	}

	sock->m_state = STATE_LISTEN;
	sock->m_timer = time(NULL);
	if (lock_mutex)
		pthread_mutex_lock(&m_mutex_sockets);
	m_sockets.insert((int)sock);
	if (lock_mutex)
		pthread_mutex_unlock(&m_mutex_sockets);
	return sock;
}

int NetworkManager::Wait()
{
	struct epoll_event events[MAX_EPOLL_EVENT];
	int nfds = epoll_wait(m_epoll_fd, events, MAX_EPOLL_EVENT, 80);
	if (nfds == -1)
	{
		return ERR_SYSCALL_ERROR;
	}
	pthread_mutex_lock(&m_mutex_sockets);
	for(int i=0; i<nfds; i++)
	{
		socket_ * sock = (socket_ *)events[i].data.ptr;
		sock->m_timer = time(NULL);
		bool need2send = buffer_remain(&sock->m_send_buffer) > 0;
		bool connected = sock->m_connected;

		if (handle_event(sock, events[i].events) == ERR_SYSCALL_ERROR)
			sock->m_errno = errno;

		if (buffer_remain(&sock->m_recv_buffer) > 0)
			m_ready2read_sockets.insert((int)sock);

		if (need2send && buffer_remain(&sock->m_send_buffer) == 0)
			m_sended_sockets.insert((int)sock);

		if (!connected && sock->m_connected)
			m_connected_sockets.insert((int)sock);

		if (sock->m_closed)
			m_closed_sockets.insert((int)sock);
	}
	for(socket_map_iter iter = m_sockets.begin(), iter_;
								iter != m_sockets.end();
								iter = iter_)
	{
		task_iter_ = task_iter;
							++task_iter_;
		socket_ * sock = *iter;
		if (sock->m_time != 0 &&
			time(NULL) - sock->m_timer >= TIMEOUT &&
			(sock->m_state == STATE_CONNECTION || (sock->m_epoll_events & EPOLLIN) != 0) &&
			!sock->m_closed)
		{
			nm->m_timeout_sockets.insert(sock);
		}
		if (sock->m_need2delete)
		{
			m_ready2read_sockets.erase(sock);
			m_sended_sockets.erase(sock);
			m_closed_sockets.erase(sock);
			m_timeout_sockets.erase(sock);
			m_unresolved_sockets.erase(sock);
			m_connected_sockets.erase(sock);
		}
	}

	pthread_mutex_unlock(&m_mutex_sockets);
	return 0;
}

int NetworkManager::Socket_send(socket_ * sock, const void * data, size_t len, bool full)
{
	if (sock == NULL)
		return ERR_BAD_ARG;
	if (sock->m_closed)
	{
		m_closed_sockets.insert((int)sock);
		//std::cout<<"SOCKET CLOSED\n";
		return ERR_BAD_ARG;
	}
	return _send(sock,data,len, full);
}

int NetworkManager::Socket_recv(socket_ * sock, void * data, size_t len)
{
	if (sock == NULL || data == NULL)
		return ERR_BAD_ARG;
	int ret =_recv(sock, data, len);
	if (ret < ERR_NO_ERROR)
	{
		//m_ready2read_sockets.erase(id);
		return ERR_INTERNAL;
	}
	if (buffer_remain(&sock->m_recv_buffer) > 0)
		m_ready2read_sockets.insert((int)sock);
	else
		m_ready2read_sockets.erase((int)sock);
	if (sock->m_closed)
		m_closed_sockets.insert((int)sock);
	return ret;
}

ssize_t NetworkManager::Socket_datalen(socket_ * sock)
{
	if (sock == NULL)
		return ERR_BAD_ARG;
	int len = buffer_remain(&sock->m_recv_buffer);
	if (len > 0)
		m_ready2read_sockets.insert((int)sock);
	else
		m_ready2read_sockets.erase((int)sock);
	if (sock->m_closed)
		m_closed_sockets.insert((int)sock);
	return len;
}

int NetworkManager::Socket_closed(socket_ * sock, bool * closed)
{
	if (sock == NULL)
	{
		*closed = true;
		return ERR_BAD_ARG;
	}
	*closed = sock->m_closed;
	return ERR_NO_ERROR;
}

int NetworkManager::Socket_close(socket_ * sock)
{
	if (sock == NULL)
	{
		return ERR_BAD_ARG;
	}
	_close(sock);
	if (sock->m_closed)
		m_closed_sockets.insert((int)sock);
	return ERR_NO_ERROR;
}

int NetworkManager::Socket_delete(socket_ * sock)
{
	pthread_mutex_lock(&m_mutex_sockets);
	//socket_ * sock = get_socket(id);//m_sockets[id];
	if (sock == NULL)
	{
		pthread_mutex_unlock(&m_mutex_sockets);
		return ERR_BAD_ARG;
	}
	sock->m_need2delete = true;
	_close(sock);
	//m_sockets.erase((int)sock);
	//delete sock;
	pthread_mutex_unlock(&m_mutex_sockets);
	return ERR_NO_ERROR;
}

int NetworkManager::Socket_set_assoc(socket_ * sock, void * assoc)
{
	if (sock == NULL)
		return ERR_BAD_ARG;
	sock->m_assoc = assoc;
	return ERR_NO_ERROR;
}

void * NetworkManager::Socket_get_assoc(socket_ * sock)
{
	if (sock == NULL)
		return NULL;
	return sock->m_assoc;
}

double NetworkManager::Socket_get_rx_speed(socket_ *sock)
{
	if (sock == NULL)
		return 0.0f;
	double time = get_time();
	double speed = (double)sock->m_rx / (time - sock->m_rx_last_time);
	//sock->m_rx_last_time = time;
	//sock->m_rx = 0;
	return speed;
}

double NetworkManager::Socket_get_tx_speed(socket_ *sock)
{
	if (sock == NULL)
		return 0.0f;
	double time = get_time();
	double speed = (double)sock->m_tx / (time - sock->m_tx_last_time);
	//sock->m_tx_last_time = time;
	//sock->m_tx = 0;
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
			socket_ * new_sock = Socket_add(client_sock, NULL, &client_addr, false);
			if (new_sock == NULL)
				return ERR_INTERNAL;
			sock->m_accepted_sockets.insert((int)new_sock);
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
	/*if (BUFFER_SIZE - sock->m_send_buffer.length < len)
	{
		return ERR_INTERNAL;
	}
	memcpy(sock->m_send_buffer.data + sock->m_send_buffer.length, data, len);
	sock->m_send_buffer.length += len;
	return epollout(sock, true);*/
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

bool NetworkManager::event_ready2read_sock(network::socket_** sock)
{
	if (m_ready2read_sockets.empty())
		return false;
	socket_set_iter iter = m_ready2read_sockets.begin();
	*sock = (socket_*)(*iter);
	m_ready2read_sockets.erase(iter);
	return true;
}

bool NetworkManager::event_closed_sock(network::socket_** sock)
{
	if (m_closed_sockets.empty())
		return false;
	socket_set_iter iter = m_closed_sockets.begin();
	*sock = (socket_*)(*iter);
	m_closed_sockets.erase(iter);
	return true;
}

bool NetworkManager::event_sended_socks(network::socket_** sock)
{
	if (m_sended_sockets.empty())
		return false;
	socket_set_iter iter = m_sended_sockets.begin();
	*sock = (socket_*)(*iter);
	m_sended_sockets.erase(iter);
	return true;
}

bool NetworkManager::event_timeout_on(network::socket_** sock)
{
	//pthread_mutex_lock(&m_mutex_sockets);
	if (m_timeout_sockets.empty())
	{
		//pthread_mutex_unlock(&m_mutex_sockets);
		return false;
	}
	socket_set_iter iter = m_timeout_sockets.begin();
	*sock = (socket_*)(*iter);
	m_timeout_sockets.erase(iter);
	//pthread_mutex_unlock(&m_mutex_sockets);
	return true;
}

bool NetworkManager::event_unresolved_sock(network::socket_** sock)
{
	//pthread_mutex_lock(&m_mutex_sockets);
	if (m_unresolved_sockets.empty())
	{
		//pthread_mutex_unlock(&m_mutex_sockets);
		return false;
	}
	socket_set_iter iter = m_unresolved_sockets.begin();
	*sock = (socket_*)(*iter);
	m_unresolved_sockets.erase(iter);
	//pthread_mutex_unlock(&m_mutex_sockets);
	return true;
}

bool NetworkManager::event_accepted_sock(network::socket_* serv, network::socket_ ** sock)
{
	//pthread_mutex_lock(&m_mutex_sockets);
	if (serv->m_accepted_sockets.empty())
	{
		//pthread_mutex_unlock(&m_mutex_sockets);
		return false;
	}
	socket_set_iter iter = serv->m_accepted_sockets.begin();
	*sock = (socket_*)(*iter);
	serv->m_accepted_sockets.erase(iter);
	//pthread_mutex_unlock(&m_mutex_sockets);
	return true;
}

bool NetworkManager::event_connected_sock(network::socket_** sock)
{
	if (m_connected_sockets.empty())
		return false;
	socket_set_iter iter = m_connected_sockets.begin();
	*sock = (socket_*)(*iter);
	m_connected_sockets.erase(iter);
	return true;
}

int NetworkManager::ConnectResolvedSocket(socket_ * sock)
{
	if (sock == NULL)
		return ERR_NULL_REF;
	struct epoll_event event;
	sock->m_connected = false;
	memset(&sock->m_send_buffer,0,sizeof(struct buffer));
	memset(&sock->m_recv_buffer,0,sizeof(struct buffer));
	if ((sock->m_socket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
		return ERR_SYSCALL_ERROR;

	event.data.ptr=(void*)sock;
	event.events = EPOLLOUT;
	sock->m_epoll_events = event.events;
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock->m_socket, &event) == -1)
	{
		close(sock->m_socket);
		return ERR_SYSCALL_ERROR;
	}
	//memcpy((void *)&sock->m_peer, (void *)addr, sizeof(struct sockaddr_in));
	if (connect(sock->m_socket, (const struct sockaddr *)&sock->m_peer, sizeof(struct sockaddr_in)) == -1)
	{
		if (errno == EINPROGRESS)
		{
			sock->m_state = STATE_CONNECTION;
			//m_sockets[sock->m_socket] = sock;
			//return sock->m_socket;
			goto end;
		}
		close(sock->m_socket);
		return ERR_SYSCALL_ERROR;
	}
	sock->m_state = STATE_TRANSMISSION;
	m_connected_sockets.insert((int)sock);
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
	//pthread_exit((void *)st);
	//std::cout<<"TIMEOUT_THREAD started\n";
	while(!nm->m_thread_stop)
	{
		pthread_mutex_lock(&nm->m_mutex_sockets);
		//pthread_mutex_lock(&m_mutex_timeout_sockets);
		for(socket_set_iter iter = nm->m_sockets.begin(); iter != nm->m_sockets.end(); ++iter)
		{
			socket_ * sock = (socket_*)(*iter);
			if (sock->m_need2resolved)
			{
				std::string domain_name = sock->m_domain;
				struct sockaddr_in addr;
				memcpy(&addr, &sock->m_peer, sizeof(addr));
				pthread_mutex_unlock(&nm->m_mutex_sockets);
				try
				{
					DomainNameResolver dnr(domain_name, &addr);
					pthread_mutex_lock(&nm->m_mutex_sockets);
					memcpy(&sock->m_peer, &addr, sizeof(addr));
					if (nm->ConnectResolvedSocket(sock) != ERR_NO_ERROR)
						nm->m_unresolved_sockets.insert((int)sock);
				}
				catch (Exception e)
				{
					pthread_mutex_lock(&nm->m_mutex_sockets);
					nm->m_unresolved_sockets.insert((int)sock);
				}
				sock->m_need2resolved = false;
				continue;
			}
		}
		pthread_mutex_unlock(&nm->m_mutex_sockets);
		//pthread_mutex_unlock(&m_mutex_timeout_sockets);
		//sleep(TIMEOUT);
		usleep(200);
	}
	//std::cout<<"TIMEOUT_THREAD stopped\n";
	return (void*)ret;
}

void NetworkManager::lock_mutex()
{
	pthread_mutex_lock(&m_mutex_sockets);
}

void NetworkManager::unlock_mutex()
{
	pthread_mutex_unlock(&m_mutex_sockets);
}

/*socket_ * NetworkManager::get_socket(int id)
{
	socket_map_iter iter = m_sockets.find(id);
	if (iter == m_sockets.end() && m_sockets.count(id) == 0)
		return NULL;
	return (*iter).second;
}*/

}
