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
#include "dinosaur.h"
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
#include <execinfo.h>

//little change for git

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
}

void bencode_test()
{
	string metafile;
	cin>>metafile;
	uint64_t len;
	char * buf = read_file(metafile.c_str(), &len, 10000000);
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
}*/
void catchCrash(int signum)
{
	void *callstack[128];
    int frames = backtrace(callstack, 128);
    char **strs=backtrace_symbols(callstack, frames);
    // тут выводим бэктрейс в файлик crash_report.txt
    // можно так же вывести и иную полезную инфу - версию ОС, программы, etc
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

int main(int argc,char* argv[])
{
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
	//network_test2();
	return 0;
}
