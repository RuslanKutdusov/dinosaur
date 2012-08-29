//============================================================================
// Name        : bittorrent.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "gui/gui.h"
#include <execinfo.h>
#include <boost/shared_ptr.hpp>
#include "libdinosaur/network/network.h"

void catchCrash(int signum)
{
	void *callstack[128];
    int frames = backtrace(callstack, 128);
    char **strs=backtrace_symbols(callstack, frames);

    FILE *f = fopen("crash_report.txt", "w");
    if (f)
    {
        for(int i = 0; i < frames; ++i)
        {
            fprintf(f, "%s\n", strs[i]);
        }
        fclose(f);
    }
    free(strs);
    signal(signum, SIG_DFL);
	exit(3);
}

/*dinosaur::network::NetworkManager nm;
dinosaur::network::Socket s;
dinosaur::network::Socket s2;

class A : public dinosaur::network::SocketAssociation
{
public:
	int event_sock_ready2read(dinosaur::network::Socket sock)
	{
		printf("ready2read\n");
		char buf[256];
		sockaddr_in addr;
		try
		{
			size_t ret = nm.Socket_recv(s2, buf, 256, &addr);
			printf("Received %d bytes, data=%s\n", ret, buf);
		}catch (dinosaur::Exception & e) {
			printf("Exception: %s\n", dinosaur::exception_errcode2str(e).c_str());
		}
		catch(dinosaur::SyscallException &e)
		{
			printf("syscallException: %s\n", e.get_errno_str());
		}
	}
	int event_sock_closed(dinosaur::network::Socket sock){printf("event_sock_closed\n");}
	int event_sock_sended(dinosaur::network::Socket sock){printf("event_sock_sended\n");}
	int event_sock_connected(dinosaur::network::Socket sock){printf("event_sock_connected\n");}
	int event_sock_accepted(dinosaur::network::Socket sock, dinosaur::network::Socket accepted_sock){printf("event_sock_accepted\n");}
	int event_sock_timeout(dinosaur::network::Socket sock)
	{
		printf("event_sock_timeout\n");
		nm.Socket_delete(s2);
	}
	int event_sock_unresolved(dinosaur::network::Socket sock){printf("event_sock_unresolved\n");}
};

class B : public dinosaur::network::SocketAssociation
{
public:
	int event_sock_ready2read(dinosaur::network::Socket sock)
	{
		printf("ready2read_\n");
	}
	int event_sock_closed(dinosaur::network::Socket sock)
	{
		printf("event_sock_closed_\n");
		nm.Socket_delete(s);
	}
	int event_sock_sended(dinosaur::network::Socket sock){printf("event_sock_sended_\n");}
	int event_sock_connected(dinosaur::network::Socket sock){printf("event_sock_connected_\n");}
	int event_sock_accepted(dinosaur::network::Socket sock, dinosaur::network::Socket accepted_sock){printf("event_sock_accepted_\n");}
	int event_sock_timeout(dinosaur::network::Socket sock)
	{
		printf("event_sock_timeout_\n");
		nm.Socket_close(s);
	}
	int event_sock_unresolved(dinosaur::network::Socket sock){printf("event_sock_unresolved_\n");}
};*/

int main(int argc,char* argv[])
{
	/*boost::shared_ptr<A> a(new A());
	boost::shared_ptr<B> b(new B());
	nm.Init();
	nm.Socket_add_UDP(b, s);
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (6666);
	inet_pton(AF_INET, "0.0.0.0", &sockaddr.sin_addr);
	nm.Socket_add_UDP(a, s2, &sockaddr);
	try
	{
		char * buf = "hello udp!\n";
		struct sockaddr_in sockaddr;
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = htons (6666);
		inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr);
		nm.Socket_send(s, buf, strlen(buf), sockaddr);
	}
	catch (dinosaur::Exception & e) {
		printf("Exception: %s\n", dinosaur::exception_errcode2str(e).c_str());
	}
	catch(dinosaur::SyscallException &e)
	{
		printf("syscallException: %s\n", e.get_errno_str());
	}
	while(1)
	{
		try
		{
			nm.clock();
			nm.notify();
		}
		catch (dinosaur::SyscallException & e) {
			printf("syscallException: %s\n", e.get_errno_str());
		}
		catch (dinosaur::Exception & e) {
			printf("Exception: %s\n", dinosaur::exception_errcode2str(e).c_str());
		}

	}
	s.reset();
	s2.reset();
	/*nm.Socket_add("127.0.0.1", 5555, a, s);
	int fd = s->get_fd();

    int rcvbuf;
    socklen_t size;
	int err = getsockopt(fd, SOL_SOCKET, SO_RCVBUF,
		      &rcvbuf, &size);
	printf("%s %d %d\n", sys_errlist[err], rcvbuf, size);

	while(1)
	{
		std::cin>>rcvbuf;
		err = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, size);
		printf("%s ", sys_errlist[err]);
		int err = getsockopt(fd, SOL_SOCKET, SO_RCVBUF,
				      &rcvbuf, &size);
		printf("%s %d %d\n", sys_errlist[err], rcvbuf, size);
	}
	return 0;*/
	signal(SIGSEGV, catchCrash);
	signal(SIGABRT, catchCrash);
	dinosaur::ERR_CODES_STR::save_defaults();
	try
	{
		init_gui();
	}
	catch(dinosaur::Exception  &e)
	{
		printf("%s\n", dinosaur::exception_errcode2str(e).c_str());
	}
	return 0;
}
