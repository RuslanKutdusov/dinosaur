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

int main(int argc,char* argv[])
{
	//dinosaur::dht::routing_table rt;
	int i;
	std::cin>>i;
	return 0;
	dinosaur::network::NetworkManager nm;
	nm.Init();
	uint32_t addr = 0;
	dinosaur::dht::dhtPtr dht;
	dinosaur::dht::node_id id = dinosaur::dht::generate_random_node_id();
	dinosaur::dht::dht::CreateDHT((const in_addr &)addr, 1234, &nm, dht, id);
	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &saddr.sin_addr);
	saddr.sin_port = htons(54723);
	dinosaur::SHA1_HASH infohash;
	dinosaur::dht::TOKEN token;
	token.token = new char[5];
	memcpy(token.token, "tokeN", 5);
	token.length = 5;
	dht->announce_peer(saddr, infohash, 1235, token);
	while(1)
	{
		try
		{
		nm.clock();
		nm.notify();
		}
		catch(...)
		{

		}
	}
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
