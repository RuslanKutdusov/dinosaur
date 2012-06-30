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
	~DomainNameResolver(){}
};

class SocketAssociation : public boost::enable_shared_from_this<SocketAssociation>
{
public:
	SocketAssociation(){}
	virtual ~SocketAssociation(){}
	virtual int event_sock_ready2read(Socket sock) = 0;
	virtual int event_sock_closed(Socket  sock) = 0;
	virtual int event_sock_sended(Socket  sock) = 0;
	virtual int event_sock_connected(Socket  sock) = 0;
	virtual int event_sock_accepted(Socket  sock) = 0;
	virtual int event_sock_timeout(Socket  sock) = 0;
	virtual int event_sock_unresolved(Socket  sock) = 0;
};

typedef boost::shared_ptr<SocketAssociation> SocketAssociationPtr;

class socket_
{
private:
	struct buffer m_send_buffer;
	struct buffer m_recv_buffer;
	uint32_t m_epoll_events;
	int m_state;
	int m_socket;
	bool m_closed;
	SocketAssociationPtr m_assoc;
	socket_set m_accepted_sockets;
	bool m_connected;
	int m_errno;
	time_t m_timer;
	double m_rx_last_time;
	double m_tx_last_time;
	uint32_t m_rx;
	uint32_t m_tx;
	bool m_need2resolved;
	std::string m_domain;
	bool m_need2delete;
public:
	struct sockaddr_in m_peer;
	socket_()
	:m_state(0), m_socket(-1), m_closed(true), m_connected(false), m_errno(0), m_timer(0), m_rx_last_time(get_time()),m_tx_last_time(get_time()),
	 m_rx(0.0f), m_tx(0.0f), m_need2resolved(false), m_need2delete(false)
	{	}
	~socket_()
	{
		close(m_socket);
	}
	int get_fd()
	{
		return m_socket;
	}
	void print_ip()
	{
		printf("%s\n", inet_ntoa(m_peer.sin_addr));
	}
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
	socket_ * get_socket(int id);
private:
	pthread_t m_thread;
	pthread_mutex_t m_mutex_sockets;
	//pthread_mutex_t m_mutex_timeout_sockets;
	bool m_thread_stop;
	static void * timeout_thread(void * arg);
	int ConnectResolvedSocket(socket_ * sock);
public:
	NetworkManager();
	int Init();
	~NetworkManager();
	int Socket_add(std::string & ip, uint16_t port, const SocketAssociationPtr & assoc, Socket & socket);
	int Socket_add(const char * ip, uint16_t port, const SocketAssociationPtr & assoc, Socket & socket);
	int Socket_add(sock_addr_in *addr, const SocketAssociationPtr & assoc, Socket & socket);
	socket_ * Socket_add(std::string & ip, uint16_t port, void * assoc, bool lock_mutex = true);//
	socket_ * Socket_add(const char * ip, uint16_t port, void * assoc, bool lock_mutex = true);
	socket_ * Socket_add_domain(const char *domain_name, uint16_t port, void * assoc, bool lock_mutex = true);
	socket_ * Socket_add_domain(std::string & domain_name, uint16_t port, void * assoc, bool lock_mutex = true);
	socket_ * Socket_add(struct sockaddr_in * addr, void * assoc, bool lock_mutex = true);//
	socket_ * Socket_add(int sock_fd, void * assoc, struct sockaddr_in * addr = NULL, bool lock_mutex = true);
	socket_ * ListenSocket_add(uint16_t port, void * assoc, bool lock_mutex = true);//
	int Socket_send(socket_ * sock, const void * data, size_t len, bool full = true);//full == 1 => надо отправить все сразу
	int Socket_recv(socket_ * sock, void * data, size_t len);
	ssize_t Socket_datalen(socket_ * sock);
	int Socket_closed(socket_ * sock, bool * closed);
	int Socket_close(socket_ * sock);
	int Socket_delete(socket_ * sock);
	int Socket_set_assoc(socket_ * sock, void * assoc);
	void * Socket_get_assoc(socket_ * sock);
	double Socket_get_rx_speed(socket_ *sock);
	double Socket_get_tx_speed(socket_ *sock);
	void lock_mutex();
	void unlock_mutex();
	bool event_ready2read_sock(network::socket_** sock);
	bool event_closed_sock(network::socket_** sock);
	bool event_sended_socks(network::socket_** sock);
	bool event_timeout_on(network::socket_** sock);
	bool event_unresolved_sock(network::socket_** sock);
	bool event_accepted_sock(network::socket_* serv, network::socket_** sock);
	bool event_connected_sock(network::socket_** sock);

	void clear_events()
	{
		m_sended_sockets.clear();
		m_closed_sockets.clear();
		m_connected_sockets.clear();
		m_timeout_sockets.clear();
	}
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
	int test_view_socks()
	{
		for(socket_set_iter iter = m_sockets.begin(); iter != m_sockets.end(); ++iter)
		{
			socket_ * sock= (socket_*)(*iter);
			std::cout<<"Socket fd = "<<sock->m_socket<<std::endl;
			std::cout<<"       cl = "<<sock->m_closed<<std::endl;
			std::cout<<"       cn = "<<sock->m_connected<<std::endl;
			std::cout<<"       er = "<<sock->m_errno<<" "<<get_sock_errno_str(sock)<<std::endl;
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
	int Wait();
};

}
