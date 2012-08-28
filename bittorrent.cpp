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

class A : public dinosaur::network::SocketAssociation
{
public:
	int event_sock_ready2read(dinosaur::network::Socket sock){}
	int event_sock_closed(dinosaur::network::Socket sock){}
	int event_sock_sended(dinosaur::network::Socket sock){}
	int event_sock_connected(dinosaur::network::Socket sock){}
	int event_sock_accepted(dinosaur::network::Socket sock, dinosaur::network::Socket accepted_sock){}
	int event_sock_timeout(dinosaur::network::Socket sock){}
	int event_sock_unresolved(dinosaur::network::Socket sock){}
};

int main(int argc,char* argv[])
{
	/*dinosaur::network::NetworkManager nm;
	boost::shared_ptr<A> a(new A());
	dinosaur::network::Socket s;
	nm.Init();
	nm.Socket_add("127.0.0.1", 5555, a, s);
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
