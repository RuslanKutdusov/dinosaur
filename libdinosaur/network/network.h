#pragma once
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <errno.h>
#include <map>
#include <list>
#include <string>
#include <set>
#include <time.h>
#include <sys/time.h>
#include <thread>
#include <mutex>
#include <memory>
#include "../exceptions/exceptions.h"
#include "../err/err_code.h"
#include "../log/log.h"

namespace dinosaur {
namespace network
{

double get_time();

#define BUFFER_SIZE 32768
#define MAX_EPOLL_EVENT 200
#define TIMEOUT 10
#define POW10_9 1000000000
#define POW10_6 1000000

enum SOCKET_STATE
{
	SOCKET_STATE_CONNECTION,
	SOCKET_STATE_TRANSMISSION,
	SOCKET_STATE_LISTEN
};

enum TRANSMISSION_TYPE
{
	TRANSMISSION_OUT,
	TRANSMISSION_IN
};


struct buffer
{
	char data[BUFFER_SIZE];
	size_t length;
	size_t pos;
};

class NetworkManager;
class socket_;
typedef std::shared_ptr<socket_> Socket;
typedef std::set<Socket> 		 socket_set;
typedef socket_set::iterator socket_set_iter;

class DomainNameResolver
{
private:
	std::string m_domain;
	sockaddr_in * m_addr;
	void addrinfo();
	void resolve();
	void split_domain_and_port();
public:
	DomainNameResolver(const std::string & domain, sockaddr_in * addr);
	DomainNameResolver(const char * domain, sockaddr_in * addr);
	~DomainNameResolver();
};

class SocketEventInterface : public std::enable_shared_from_this<SocketEventInterface>
{
public:
	SocketEventInterface()	{}
	virtual ~SocketEventInterface(){}
private:
	virtual void event_sock_ready2read(Socket sock) = 0;
	virtual void event_sock_error(Socket  sock, int errno_) = 0;
	virtual void event_sock_sended(Socket  sock) = 0;
	virtual void event_sock_connected(Socket  sock) = 0;
	virtual void event_sock_accepted(Socket  sock, Socket accepted_sock) = 0;
	virtual void event_sock_timeout(Socket  sock) = 0;
	virtual void event_sock_unresolved(Socket  sock) = 0;
	friend class socket_;
};

typedef std::weak_ptr<SocketEventInterface> SocketEventInterfacePtr;

class socket_ : public std::enable_shared_from_this<socket_>
{
private:
	buffer 						m_send_buffer;
	uint32_t 					m_epoll_events;
	SOCKET_STATE 				m_state;
	int 						m_socket;
	SocketEventInterfacePtr 	m_assoc;
	int 						m_errno;
	time_t 						m_timer;
	double 						m_rx_last_time;
	double 						m_tx_last_time;
	uint64_t 					m_rx;
	uint64_t 					m_tx;
	bool 						m_need2resolved;
	bool 						m_need2delete;
	std::string 				m_domain;
	bool 						m_udp;
	sockaddr_in 				m_peer;
	void close_()
	{
		close(m_socket);
		m_socket = -1;
	}
	bool closed()
	{
		return m_socket == -1;
	}
	void event_sock_ready2read(){
		if (!m_assoc.expired())
			m_assoc.lock()->event_sock_ready2read(shared_from_this());
	}
	void event_sock_error(int errno_){
		if (!m_assoc.expired())
			m_assoc.lock()->event_sock_error(shared_from_this(), errno_);
	}
	void event_sock_sended(){
		if (!m_assoc.expired())
			m_assoc.lock()->event_sock_sended(shared_from_this());
	}
	void event_sock_connected(){
		if (!m_assoc.expired())
			m_assoc.lock()->event_sock_connected(shared_from_this());
	}
	void event_sock_accepted(Socket accepted_sock){
		if (!m_assoc.expired())
			m_assoc.lock()->event_sock_accepted(shared_from_this(), accepted_sock);
	}
	void event_sock_timeout(){
		if (!m_assoc.expired())
			m_assoc.lock()->event_sock_timeout(shared_from_this());
	}
	void event_sock_unresolved(){
		if (!m_assoc.expired())
			m_assoc.lock()->event_sock_unresolved(shared_from_this());
	}
public:
	socket_()
		: m_epoll_events(0), m_socket(-1),  m_errno(0), m_timer(0), m_rx_last_time(get_time()),m_tx_last_time(get_time()),
		 m_rx(0.0f), m_tx(0.0f), m_need2resolved(false), m_need2delete(false), m_udp(false)
	{
		memset(&m_peer, 0, sizeof(sockaddr_in));
		m_send_buffer.length = 0;
		m_send_buffer.pos = 0;
	}
	virtual ~socket_()
	{
		close(m_socket);
#ifdef BITTORRENT_DEBUG
		logger::LOGGER() << "socket " << m_socket << " destroyed";
#endif
	}
	const sockaddr_in & get_addr(){
		return m_peer;
	}
#ifdef BITTORRENT_DEBUG
	int get_fd()
	{
		return m_socket;
	}
	void print_ip()
	{
		logger::LOGGER() << inet_ntoa(m_peer.sin_addr);
	}
#endif
	int get_errno()
	{
		return m_errno;
	}
	friend class NetworkManager;
};

class NetworkManager
{
private:
	socket_set 	m_timeout_sockets;
	socket_set 	m_unresolved_sockets;
	socket_set 	m_connected_after_resolving_sockets;
	socket_set 	m_sockets;
	int 		m_epoll_fd;
	void handle_event(Socket & sock, uint32_t events);
	void handler_connection(Socket & sock);
	void socket_state_transmission_out_handler(Socket & sock);
	void socket_state_transmission_in_handler(Socket & sock);
	void epoll_mod(socket_ * sock);
	void epoll_mod(socket_ * sock, uint32_t events);
	void epollin(socket_ * sock, bool on_off);
	void epollout(socket_ * sock, bool on_off);
private:
	std::recursive_mutex 	m_mutex_sockets;
	bool 		m_thread_stop;
	std::thread m_thread;
	void timeout_thread();
	void ConnectResolvedSocket(Socket & sock);
public:
	NetworkManager();
	void Init();
	~NetworkManager();
	void Socket_add(const std::string & ip, uint16_t port, const SocketEventInterfacePtr & assoc, Socket & socket);
	void Socket_add(const sockaddr_in & addr, const SocketEventInterfacePtr & assoc, Socket & socket);
	void Socket_add(int sock_fd, const sockaddr_in & addr, const SocketEventInterfacePtr & assoc, Socket & sock);
	void Socket_add_domain(const std::string & domain_name, uint16_t port, const SocketEventInterfacePtr & assoc, Socket & socket);
	void Socket_add_UDP(const SocketEventInterfacePtr & assoc, Socket & socket, const sockaddr_in * bind_ = nullptr);
	void ListenSocket_add(const sockaddr_in & addr, const SocketEventInterfacePtr & assoc, Socket & sock) ;
	void ListenSocket_add(uint16_t port, const SocketEventInterfacePtr & assoc, Socket & sock) ;
	void ListenSocket_add(uint16_t port, const in_addr & addr, const SocketEventInterfacePtr & assoc, Socket & sock);
	void ListenSocket_add(uint16_t port, const std::string & ip, const SocketEventInterfacePtr & assoc, Socket & sock);
	size_t Socket_send(Socket & sock, const void * data, size_t len, bool full = true);//full == 1 => надо отправить все сразу
	ssize_t Socket_send(Socket & sock, const void * data, size_t len, const sockaddr_in & addr);
	size_t Socket_recv(Socket & sock, char * data, size_t len, bool & is_closed, sockaddr_in * from = nullptr);
	bool Socket_closed(Socket & sock);
	void Socket_close(Socket & sock);
	void Socket_delete(Socket & sock);
	void Socket_set_event_interface(Socket & sock, const SocketEventInterfacePtr & assoc);
	void Socket_get_event_interface(Socket & sock, SocketEventInterfacePtr & assoc);
	int Socket_get_rx_speed(Socket & sock);
	int Socket_get_tx_speed(Socket & sock);
	void Socket_get_addr(Socket & sock, sockaddr_in & addr);
	ssize_t Socket_sendbuf_remain(Socket & sock)
	{
		if (sock == NULL)
			return 0;
		return sock->m_send_buffer.length - sock->m_send_buffer.pos;
	}
	int clock() ;
};

}
}
