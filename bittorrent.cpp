//============================================================================
// Name        : bittorrent.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "gui/gui.h"
#include "libdinosaur/dht/dht.h"
#include "libdinosaur/network/network.h"
#include "libdinosaur/log/log.h"
#include <limits>
#include <boost/shared_ptr.hpp>

/*
8F385C2AECB03BFB32AF3C54EC18DB5C021AFE43
FBFAAA3AFB29D1E6053C7C9475D8BE6189F95CBB
A8990F95B1EBF1B305EFF700E9A13AE5CA0BCBD0
F8A41B13B5CA4EE8983238E0794D3D34BC5F4E77
FACB6C05AC86212BAA1A55A2BE70B5733B045CD3
8A2954489A0ABCD50E18A844AC5BF38E4CD72D9B
9BBCE9E525CF08F5E9E25E5360AAD2B2D085FA54
D835E8D466826498D9A8877565705A8A3F628029
*/

dinosaur::network::NetworkManager nm;
dinosaur::dht::dhtPtr dht;
bool work = true;

static void * thread(void * arg)
{
	while(work)
	{
		try
		{
			nm.clock();
		}
		catch(...)
		{

		}
		//dht->clock();
	}
	return arg;
}

class ClassA : public dinosaur::network::SocketAssociation
{
private:
	void event_sock_ready2read(dinosaur::network::Socket sock)
	{
		printf("ready2read\n");
		char b[1024];
		memset(b, 0, 1024);
		bool closed;
		int r = nm.Socket_recv(sock, b, 1024, closed);
		printf("%u %s closed=%d\n", r, b, closed);
		if (closed)
			nm.Socket_delete(sock);
	}
	void event_sock_error(dinosaur::network::Socket  sock, int errno_)
	{
		printf("closed\n");
	}
	void event_sock_sended(dinosaur::network::Socket  sock)
	{
		printf("sended\n");
	}
	void event_sock_connected(dinosaur::network::Socket  sock)
	{
		printf("connected\n");
	}
	void event_sock_accepted(dinosaur::network::Socket  sock, dinosaur::network::Socket accepted_sock)
	{
		printf("accepted\n");
	}
	void event_sock_timeout(dinosaur::network::Socket  sock)
	{
		printf("timeout\n");
	}
	void event_sock_unresolved(dinosaur::network::Socket  sock)
	{
		printf("unresolved\n");
	}
};

int main(int argc,char* argv[])
{
	/*dinosaur::ERR_CODES_STR::save_defaults();
	nm.Init();
	boost::shared_ptr<ClassA> ClassA_PTR(new ClassA());
	dinosaur::network::Socket sock;
	//nm.Socket_add("127.0.0.1", 1234, ClassA_PTR, sock);
	nm.Socket_add_domain("vk.ru", 80, ClassA_PTR, sock);
	//uint32_t addr = 0;
	//dinosaur::dht::dht::CreateDHT((const in_addr &)addr, 1234, &nm, "/home/ruslan/", dht);
	//sockaddr_in saddr;
	//saddr.sin_family = AF_INET;
	//inet_aton("127.0.0.1", &saddr.sin_addr);
	//saddr.sin_port = htons(54723);
	//dht->add_node(saddr);
	pthread_t m_thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&m_thread, &attr, thread, NULL);
	int i;
	std::cin>>i;
	work = false;
	void * status;
	pthread_join(m_thread, &status);
	//dht->prepare2release();
	//dht.reset();
	return 0;*/


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
