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
#include <netdb.h>
#include <map>
#include <list>
#include <string>
#include <set>
#include <time.h>
#include <pthread.h>
#include <pcre.h>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "../exceptions/exceptions.h"
#include "../err/err_code.h"

namespace network
{

double get_time();

#define BUFFER_SIZE 32768
#define MAX_EPOLL_EVENT 200
#define TIMEOUT 10
#define POW10_9 1000000000

enum
{
	STATE_CONNECTION,
	STATE_TRANSMISSION,
	STATE_LISTEN
};

enum
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
typedef boost::shared_ptr<socket_> Socket;
typedef std::set<Socket> 		 socket_set;
typedef socket_set::iterator socket_set_iter;

class DomainNameResolver
{
private:
	std::string m_domain;
	struct sockaddr_in * m_addr;
	int addrinfo();
	int resolve();
	int split_domain_and_port();
public:
	DomainNameResolver(std::string & domain, struct sockaddr_in * addr);
	DomainNameResolver(const char * domain, struct sockaddr_in * addr);
	~DomainNameResolver();
};

class SocketAssociation : public boost::enable_shared_from_this<SocketAssociation>
{
public:
	typedef boost::shared_ptr<SocketAssociation> ptr;
	SocketAssociation()	{}
	virtual ~SocketAssociation(){}
	ptr get_ptr()
	{
		return shared_from_this();
	}
	virtual int event_sock_ready2read(Socket sock) = 0;
	virtual int event_sock_closed(Socket  sock) = 0;
	virtual int event_sock_sended(Socket  sock) = 0;
	virtual int event_sock_connected(Socket  sock) = 0;
	virtual int event_sock_accepted(Socket  sock, Socket accepted_sock) = 0;
	virtual int event_sock_timeout(Socket  sock) = 0;
	virtual int event_sock_unresolved(Socket  sock) = 0;

};

class socket_ : public boost::enable_shared_from_this<socket_>
{
private:
	struct buffer m_send_buffer;
	struct buffer m_recv_buffer;
	uint32_t m_epoll_events;
	int m_state;
	int m_socket;
	bool m_closed;
	SocketAssociation::ptr m_assoc;
	socket_set m_accepted_sockets;
	bool m_connected;
	int m_errno;
	time_t m_timer;
	double m_rx_last_time;
	double m_tx_last_time;
	uint32_t m_rx;
	uint32_t m_tx;
	bool m_need2resolved;
	bool m_need2delete;
	std::string m_domain;
public:
	struct sockaddr_in m_peer;
	socket_()
	:m_epoll_events(0),m_state(0), m_socket(-1), m_closed(true), m_connected(false), m_errno(0), m_timer(0), m_rx_last_time(get_time()),m_tx_last_time(get_time()),
	 m_rx(0.0f), m_tx(0.0f), m_need2resolved(false), m_need2delete(false)
	{
#ifdef BITTORRENT_DEBUG
	printf("Socket constructor\n");
#endif
	}
	~socket_()
	{
#ifdef BITTORRENT_DEBUG
		std::cout<<"Socket destructed "<<m_socket<<" "<<m_domain<<" "<<inet_ntoa(m_peer.sin_addr)<<std::endl;
#endif
		close(m_socket);
	}
	int get_fd()
	{
		return m_socket;
	}
#ifdef BITTORRENT_DEBUG
	void print_ip()
	{
		printf("%s\n", inet_ntoa(m_peer.sin_addr));
	}
#endif
	friend class NetworkManager;
};

class NetworkManager
{
private:
	socket_set m_ready2read_sockets;
	socket_set m_sended_sockets;
	socket_set m_closed_sockets;
	socket_set m_connected_sockets;
	socket_set m_timeout_sockets;
	socket_set m_unresolved_sockets;
	socket_set m_listening_sockets;
	socket_set m_sockets;
	int m_epoll_fd;
	int handle_event(socket_ * sock, uint32_t events);
	int handler_connection(socket_ * sock);
	int handler_transmission(socket_ * sock, int transmission_type);
	int _send(socket_ * sock, const void * data,size_t len, bool full);
	int _recv(socket_ * sock, void * data, size_t len);
	int _close(socket_ * sock);
	int epoll_mod(socket_ * sock);
	int epoll_mod(socket_ * sock, uint32_t events);
	int epollin(socket_ * sock, bool on_off);
	int epollout(socket_ * sock, bool on_off);
private:
	pthread_t m_thread;
	pthread_mutex_t m_mutex_sockets;
	bool m_thread_stop;
	static void * timeout_thread(void * arg);
	int ConnectResolvedSocket(Socket & sock);
public:
	NetworkManager();
	int Init();
	~NetworkManager();
	int Socket_add(std::string & ip, uint16_t port, const SocketAssociation::ptr & assoc, Socket & socket);
	int Socket_add(const char * ip, uint16_t port, const SocketAssociation::ptr & assoc, Socket & socket);
	int Socket_add(sockaddr_in *addr, const SocketAssociation::ptr & assoc, Socket & socket);
	int Socket_add(int sock_fd, struct sockaddr_in * addr, const SocketAssociation::ptr & assoc, Socket & sock);
	int Socket_add_domain(const char *domain_name, uint16_t port, const SocketAssociation::ptr & assoc, Socket & socket);
	int Socket_add_domain(std::string & domain_name, uint16_t port, const SocketAssociation::ptr & assoc, Socket & socket);
	int ListenSocket_add(uint16_t port, const SocketAssociation::ptr & assoc, Socket & sock);
	int Socket_send(Socket & sock, const void * data, size_t len, bool full = true);//full == 1 => надо отправить все сразу
	int Socket_recv(Socket & sock, void * data, size_t len);
	ssize_t Socket_datalen(Socket & sock);
	int Socket_closed(Socket & sock, bool * closed);
	int Socket_close(Socket & sock);
	int Socket_delete(Socket & sock);
	int Socket_set_assoc(Socket & sock, const SocketAssociation::ptr & assoc);
	int Socket_get_assoc(Socket & sock, SocketAssociation::ptr & assoc);
	double Socket_get_rx_speed(Socket & sock);
	double Socket_get_tx_speed(Socket & sock);
	int get_sock_errno(socket_ * sock)
	{
		if (sock == NULL)
			return ERR_BAD_ARG;
		return sock->m_errno;
	}
	const char * get_sock_errno_str(socket_ * sock)
	{
		if (sock == NULL)
			return NULL;
		return sys_errlist[sock->m_errno];
	}
	ssize_t Socket_send_buf_len(socket_ * sock)
	{
		if (sock == NULL)
			return -1;
		return sock->m_send_buffer.length;
	}
#ifdef BITTORRENT_DEBUG
	int test_view_socks()
	{
		for(socket_set_iter iter = m_sockets.begin(); iter != m_sockets.end(); ++iter)
		{
			Socket sock= *iter;
			std::cout<<"Socket fd = "<<sock->m_socket<<std::endl;
			std::cout<<"       cl = "<<sock->m_closed<<std::endl;
			std::cout<<"       cn = "<<sock->m_connected<<std::endl;
			std::cout<<"       er = "<<sock->m_errno<<" "<<get_sock_errno_str(sock.get())<<std::endl;
			char * ip = inet_ntoa (sock->m_peer.sin_addr ) ;
			std::cout<<"       IP:port ="<<ip<<":"<<ntohs(sock->m_peer.sin_port)<<std::endl;
			std::cout<<"       Recvbuffer"<<std::endl;
			std::cout<<"                   length = "<<sock->m_recv_buffer.length<<std::endl;
			std::cout<<"                   pos    = "<<sock->m_recv_buffer.pos<<std::endl;
			std::cout<<"       Sendbuffer"<<std::endl;
			std::cout<<"                   length = "<<sock->m_send_buffer.length<<std::endl;
			std::cout<<"                   pos    = "<<sock->m_send_buffer.pos<<std::endl;
			std::cout<<"       Accepted socks: ";
			for(socket_set_iter iter2 = sock->m_accepted_sockets.begin(); iter2 != sock->m_accepted_sockets.end(); ++iter2)
			{
				std::cout<<*iter2<<" ";
			}
			std::cout<<std::endl;
		}
		return 0;
	}
#endif
	int clock();
	void notify();
};

}
