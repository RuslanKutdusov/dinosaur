//============================================================================
// Name        : bittorrent.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "network/network.h"
#include "utils/bencode.h"
#include "torrent/torrent.h"
#include "cfg/glob_cfg.h"
#include "bittorrent/Bittorrent.h"
#include <sys/epoll.h>
#include <set>
#include <map>
#include <vector>
#include <queue>
using namespace std;
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pcre.h>
#include <time.h>
#include <math.h>
#include <queue>
#include "fs/fs.h"
#include <boost/bimap.hpp>
#include "block_cache/Block_cache.h"
#include <assert.h>
#include "gui/gui.h"
#include "lru_cache/lru_cache.h"
#include "utils/dir_tree.h"
#include "fs/fs_tests.h"

void network_test1()
{
	network::NetworkManager nm;
	int sock1_assoc = 321;
	network::socket_ * sock = nm.Socket_add("127.0.0.1",18025,&sock1_assoc);
	char data[] = "Hello dude!\n";

	while(1)
	{
		printf("loop\n");
		nm.Wait();
		nm.Socket_send(sock, data, strlen(data));
	}
}

void network_test2()
{
	std::string domain;
		cin>>domain;
	network::NetworkManager nm;
	nm.Init();
	//network::socket_* serv = nm.ListenSocket_add(54642,NULL);
	//nm.ListenSocket_add(54643,NULL);
	nm.Socket_add_domain(domain, 80, NULL, false);
	nm.Wait();
	while(1)
	{
		cout<<"================================================================================\n";
		nm.test_view_socks();
		nm.Wait();
		network::socket_ * sock = NULL;
		while(nm.event_ready2read_sock(&sock))
		{
			char buf[1000];
			ssize_t len=nm.Socket_datalen(sock);
			nm.Socket_recv(sock,buf,len);
			cout<<"RECV: "<<sock->get_fd()<<"LENGTH="<<len<<" DATA="<<buf<<endl;
			if (buf[0] == '1')
				return;
			nm.Socket_send(sock,buf,len);
		}
		while(nm.event_closed_sock(&sock))
		{
			cout<<"CLOSED: "<<sock<<endl;
			nm.Socket_delete(sock);
			//return;
		}
		/*while(nm.event_accepted_sock(serv,&sock))
		{
			cout<<"Accepted socket "<<sock->get_fd()<<endl;
		}*/
		while(nm.event_sended_socks(&sock))
		{
			cout<<"Sended socket "<<sock->get_fd()<<endl;
			nm.Socket_close(sock);
		}
		while(nm.event_timeout_on(&sock))
		{
			cout<<"Timeout on socket "<<sock->get_fd()<<endl;
			nm.Socket_close(sock);
		}
		while(nm.event_connected_sock(&sock))
		{
			cout<<"event_connected_sock socket "<<sock->get_fd()<<endl;
			nm.Socket_close(sock);
		}
		while(nm.event_unresolved_sock(&sock))
		{
			cout<<"event_unresolved_sock socket "<<sock->get_fd()<<endl;
			nm.Socket_close(sock);
		}
		//nm.clear_events();
		sleep(1);
	}
}


void char_set()
{
	unsigned char byte = '\xff';
	while(1)
	{
		int n;
		cin>>n;
		int bit = (int)pow(2.0f, 7 - n);
		//byte |= bit;
		char mask;
		memset(&mask, 255, 1);
		mask ^= bit;
		byte &= mask;
		cout<<(int)byte<<endl;
	}
}

void fm_test()
{
	//cfg::Glob_cfg g_cfg;
	//fs::FileManager fm(&g_cfg);
	//int a = 10;
	//int f = fm.File_add("/home/ruslan/.test_file",100, false, NULL);
	/*char * buf = "dkjfdjl;kdjg;kldfjg;lksdjg;kldjg;lkdsfjg;lkdsfjgkldjfg;lkdfjg;lkdsfjg;lkdjg;lkdjg;lsdfjgs";
	fm.File_write(f, buf, strlen(buf), 10, 234);

	while(1)
	{
		fs::event_on eo;
		memset(&eo, 0, sizeof(fs::event_on));
		if (fm.get_write_event(&eo))
		{
			printf("write event id=%lld add_info=%d assoc=%d\n", eo.id, eo.add_info, *(int*)eo.assoc);
		}
		else
			printf("No event\n");
		sleep(1);
	}*/
}

int bittorrent_test()
{
	try
	{
		bittorrent::Bittorrent bt;
		char fn[256];
		cin>>fn;
		if (strncmp(fn,"exit", 4)==0)
			return 0;
		std::string hash;
		bt.AddTorrent(fn, &hash);
		while(1)
		{
			torrent::torrent_info info;
			bt.Torrent_info(hash, &info);
			system("clear");
			cout<<"Name "<<info.name<<endl;
			cout<<"Comment "<<info.comment<<endl;
			cout<<"Created by "<<info.created_by<<endl;
			cout<<"Creation date "<<info.creation_date<<endl;
			cout<<"Downloaded "<<info.downloaded<<endl;
			cout<<"Uploaded "<<info.uploaded<<endl;
			cout<<"Length "<<info.length<<endl;
			cout<<"Piece count "<<info.piece_count<<endl;
			cout<<"Piece length "<<info.piece_length<<endl;
			cout<<"Private "<<info.private_<<endl;
			cout<<"Download speed "<<info.rx_speed<<endl;
			cout<<"Uploade speed "<<info.tx_speed<<endl;
			cout<<"Trackers:"<<endl;
			for(torrent::torrent_info::tracker_info_iter iter = info.trackers.begin(); iter != info.trackers.end(); ++iter)
			{
				printf("  %s %s %u %llu %llu\n", (*iter).announce.c_str(), (*iter).status.c_str(), (uint32_t)(*iter).update_in, (*iter).seeders, (*iter).leechers);
			}
			cout<<"Peers:"<<endl;
			for(torrent::torrent_info::peer_info_iter iter= info.peers.begin(); iter != info.peers.end(); ++iter)
			{
				if ((*iter).downSpeed > 0 || (*iter).upSpeed > 0)
					printf("  %s %f %llu %f %llu %f\n", (*iter).ip, (*iter).available, (*iter).downloaded, (*iter).downSpeed, (*iter).uploaded, (*iter).upSpeed);
			}
			sleep(1);
		}
	}
	catch(Exception e)
	{
		cout<<"EXCEPTION\n";
	}
	return 0;
}

void bimap_test()
{
	typedef boost::bimap< int, char > bm_type;
	bm_type bm;
	//bm.insert(bm_type::value_type(1, "one"));
	//bm.insert(bm_type::value_type(2, "two"));
	for(int i = 65; i < 75; i++)
	{
		bm.insert(bm_type::value_type(rand(), (char)i));
	}
	bm_type::right_iterator it = bm.right.find('C');
	bm.right.replace_key(it, 'c');
	bm.right.erase('A');
	it = bm.right.find('B');
	if (it == bm.right.end() && bm.right.count('B') == 0)
		cout<<"not found\n";
	else
		bm.right.replace_data(it, -100);
	bm.right.erase(it);
	bm.insert(bm_type::value_type(-200, 'F'));
	for( bm_type::const_iterator iter = bm.begin(), iend = bm.end();
	        iter != iend; ++iter )
	{
		// iter->left  : data : int
		// iter->right : data : std::string

		std::cout << iter->left << " <--> " << iter->right << std::endl;
	}
	cout<<"==================================\n";
	typedef bm_type::left_map::const_iterator left_const_iterator;

	for( left_const_iterator left_iter = bm.left.begin(), iend = bm.left.end();
		 left_iter != iend; ++left_iter )
	{
		// left_iter->first  : key  : int
		// left_iter->second : data : std::string

		std::cout << left_iter->first << " --> " << left_iter->second << std::endl;
	}
	cout<<"==================================\n";
	typedef bm_type::right_map::const_iterator right_const_iterator;

	for( right_const_iterator right_iter = bm.right.begin(), iend = bm.right.end();
			right_iter != iend; ++right_iter )
	{
		// left_iter->first  : key  : int
		// left_iter->second : data : std::string

		std::cout << right_iter->first << " --> " << right_iter->second << std::endl;
	}

}

/*void block_cache_test()
{
	block_cache::Block_cache bc;
	uint16_t count = 5;
	uint16_t data_count = count + 2;
	bc.Init(count);
	bc.dump_elements();
	bc.dump_all();
	cout<<"||||||||||||||||||||||||||||||||||||\n";
	char ** data;
	data = new char*[data_count ];
	for(uint16_t i = 0; i < data_count; i++)
	{
		data[i] = new char[16384];
		sprintf(data[i], "Data index %u", i);
	}
	for(uint16_t i = 0; i < count; i++)
	{
		block_cache::cache_key key(data[i], i);
		bc.put(key, (block_cache::cache_element*)data[i]);
	}
	bc.dump_all();
	cout<<"||||||||||||||||||||||||||||||||||||\n";

	char r0[16384];
	block_cache::cache_key key(data[2], 2);
	assert(bc.get(key, (block_cache::cache_element*)r0) == ERR_NO_ERROR);
	cout<<r0<<endl;
	bc.dump_all();
	cout<<"||||||||||||||||||||||||||||||||||||\n";

	block_cache::cache_key key1(data[data_count - 2], data_count - 2);
	bc.put(key1, (block_cache::cache_element*)data[data_count -2]);
	block_cache::cache_key key2(data[data_count - 1], data_count - 1);
	bc.put(key2, (block_cache::cache_element*)data[data_count -1]);
	bc.dump_all();
	cout<<"||||||||||||||||||||||||||||||||||||\n";

	for(uint16_t i = 0; i < data_count; i++)
	{
		delete[] data[i];
	}

	delete[] data;
}

void test_lru_cache()
{
	lru_cache::LRU_Cache <int,int> lc;
	lc.Init(10);
	for(int i = 0; i < 10; i++)
	{
		int d = i + 10;
		lc.put(i, &d);
	}
	lc.dump_all();
	while(1)
	{
		int k,d;
		cin>>k;
		if (k == -1)
		{
			cin>>k;
			lc.get(k, &d);
			cout<<d<<endl;
			lc.dump_all();
			continue;
		}
		cin>>d;
		lc.put(k,&d);
		lc.dump_all();
	}
}*/

void bencode_test()
{
	string metafile;
	cin>>metafile;
	uint64_t len;
	char * buf = read_file(metafile.c_str(), &len);
	if (buf == NULL)
	{
		return;
	}
	bencode::be_node * node  = bencode::decode(buf, len, true);
	if (node == NULL)
		return;
	free(buf);
	bencode::dump(node);
	bencode::be_node * info;// = bencode::get_info_dict(m_metafile);
	if (bencode::get_node(node, "info", &info) == -1)
		return;
	buf = new char[len];
	uint32_t out_len = 0;
	int ret = bencode::encode(info, &buf, len, &out_len);
	cout<<"RET = "<<ret<<"out_len="<<out_len<<endl;
	char hash_hex1[41];
	char hash_hex2[41];
	memset(hash_hex1, 0, 41);
	memset(hash_hex2, 0, 41);

	CSHA1 csha1;
	csha1.Update((unsigned char*)buf,out_len);
	csha1.Final();
	csha1.ReportHash(hash_hex1,CSHA1::REPORT_HEX);
	csha1.Reset();
	delete[] buf;

	//uint64_t out_len64 = 0;
	buf = NULL;
	//bencode::encode_old(info,&buf,&out_len64);

	csha1.Update((unsigned char*)buf,out_len);
	csha1.Final();
	csha1.ReportHash(hash_hex2,CSHA1::REPORT_HEX);
	csha1.Reset();
	delete[] buf;

	cout<<hash_hex1<<endl;
	cout<<hash_hex2<<endl;
}

int main(int argc,char* argv[]) {
	//block_cache_test();
	//dir_tree::DirTreeTest drt;
	//drt.test_DirTree();
	//test_fd_cache_test();
	//return 0;

	try
	{
		init_gui();

	}
	catch(Exception e)
	{
		e.print();
	}
	//network_test2();
	return 0;
}
