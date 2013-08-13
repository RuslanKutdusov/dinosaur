/*
 * network_test.h
 *
 *  Created on: 10 авг. 2013 г.
 *      Author: ruslan
 */

#ifndef NETWORK_TEST_H_
#define NETWORK_TEST_H_
#include "libdinosaur/network/network.h"
#include <set>

namespace network_test{

class A : public dinosaur::network::SocketEventInterface{
private:
	std::set<std::shared_ptr<A>> clients;
	void event_sock_ready2read(dinosaur::network::Socket sock){
		printf("\n%s read ", desc.c_str());
		char buf[1024];
		bool is_closed;
		size_t readed = m_nm->Socket_recv(sock, buf, 1024, is_closed);
		if (is_closed)
			printf("(closed), ");
		else
			m_nm->Socket_send(sock, buf, readed);
		printf("readed %zu\n", readed);
	}
	void event_sock_error(dinosaur::network::Socket sock, int errno_){
		printf("\n%s error %s\n", desc.c_str(), sys_errlist[errno_]);
	}
	void event_sock_sended(dinosaur::network::Socket sock){
		printf("\n%s sended\n", desc.c_str());
	}
	void event_sock_connected(dinosaur::network::Socket sock){
		printf("\n%s connected\n", desc.c_str());
	}
	void event_sock_accepted(dinosaur::network::Socket sock, dinosaur::network::Socket accepted_sock){
		const sockaddr_in & peer = accepted_sock->get_addr();
		printf("\n%s accepted %s:%u\n", desc.c_str(), inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
		char buf[1024];
		sprintf(buf, "%s:%u", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
		auto ptr = std::make_shared<A>(buf, m_nm);
		ptr->sock = accepted_sock;
		m_nm->Socket_set_event_interface(accepted_sock, ptr);
		clients.insert(ptr);
	}
	void event_sock_timeout(dinosaur::network::Socket sock){
		printf("\n%s timeout\n", desc.c_str());
	}
	void event_sock_unresolved(dinosaur::network::Socket sock){
		printf("\n%s unresolved\n", desc.c_str());
	}
	std::string desc;
	dinosaur::network::NetworkManager* m_nm;
public:
	dinosaur::network::Socket sock;
	A(const std::string & desc_, dinosaur::network::NetworkManager * nm)
		: desc(desc_), m_nm(nm)
	{
	}
	virtual ~A(){
		std::cout << "\n" << desc << " destroyed\n";
	}
};

bool stop = false;
bool kill_this_shit = false;

static void network_test_thread(dinosaur::network::NetworkManager & nm, dinosaur::network::Socket sock){
	while(1){
		std::string str;
		std::cin >> str;
		if (str == "stop"){
			stop = true;
			break;
		}
		if (str == "kill")
			kill_this_shit = true;
		try{
			nm.Socket_send(sock, str.c_str(), str.length());
		}
		catch (dinosaur::Exception & e){
			std::cout << dinosaur::exception_errcode2str(e) << std::endl;
		}
	}
}


static void network_test(){
	using namespace dinosaur;
	using namespace dinosaur::network;
	ERR_CODES_STR::save_defaults();
	NetworkManager nm;
	std::shared_ptr<A> localhost = std::make_shared<A>("localhost", &nm);
	std::shared_ptr<A> ructf = std::make_shared<A>("ructf", &nm);
	std::shared_ptr<A> server = std::make_shared<A>("server", &nm);
	nm.Socket_add("127.0.0.1", 8000, localhost, localhost->sock);
	nm.Socket_add_domain("ructf.org", 80, ructf, ructf->sock);
	nm.ListenSocket_add(9999, server, server->sock);
	std::thread thread = std::thread(network_test_thread, std::ref(nm), localhost->sock);
	while(!stop){
		try{
			nm.clock();
		}
		catch(SyscallException & e){
			printf("Exception: %s\n", e.get_errno_str());
		}
		if (kill_this_shit){
			localhost.reset();
			kill_this_shit = false;
		}
	}
	thread.join();
}

}

#endif /* NETWORK_TEST_H_ */
