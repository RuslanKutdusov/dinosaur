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
#include <thread>
#include "network_test.h"

bool send_get_peers = false;
bool send_get_peers2 = false;
bool dump = false;
bool do_exit = false;

void thread_()
{
	while(!do_exit){
		int i;
		std::cin >> i;
		if (i == 1)
			send_get_peers = true;
		if (i == 2)
			send_get_peers2 = true;
		if (i == 3)
			dump = true;
		if (i == 4)
			do_exit = true;

	}
	printf("Done\n");
}

int main(int argc,char* argv[])
{
	network_test::network_test();
	return 0;
	std::thread t = std::thread(thread_);

	dinosaur::network::NetworkManager nm;
	nm.Init();
	dinosaur::dht::dhtPtr dht;
	in_addr addr;
	inet_aton("0.0.0.0", &addr);
	dinosaur::dht::dht::CreateDHT(addr, 8881, &nm, "/home/ruslan/", dht);

	sockaddr_in addr2;
	addr2.sin_family = AF_INET;
	inet_aton("127.0.0.1", &addr2.sin_addr);
	addr2.sin_port = htons(6881);
	dht->add_node(addr2);

	dinosaur::dht::node_id id("\xab\xf5\xac\xb4\xde\xd7\xd0\x8e\xc7\x49\x95\x37\xa7\x09\xbb\xe4\xd9\x7c\x48\x08");

	while(!do_exit){
		try{
			nm.clock();
			dht->clock();
		}
		catch(...){

		}
		if (send_get_peers){
			send_get_peers = false;
		 	dht->search(id);
		}
		if (send_get_peers2){
			send_get_peers2 = false;
			dht->get_peers(addr2, id);
		}
		if (dump){
			dump = false;
			dht->dump();
		}
	}
	t.join();
	return 0;
	dinosaur::ERR_CODES_STR::save_defaults();
    try
    {
        init_gui();
    }
    catch(dinosaur::Exception  &e)
    {
        printf("%s\n", dinosaur::exception_errcode2str(e).c_str());
    }

//    try
//    {
//        dinosaur::torrent_failures tfs;
//        dinosaur::Dinosaur::CreateDinosaur(bt, tfs);
//    }
//    catch(dinosaur::Exception & e)
//    {
//        std::cout<<dinosaur::exception_errcode2str(e);
//    }
//    catch(...)
//    {
//        std::cout<<"Undefined behavior\n";
//        return 0;
//    }

	return 0;
}
