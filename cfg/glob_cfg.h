/*
 * GlobCfg.h
 *
 *  Created on: 17.02.2012
 *      Author: ruslan
 */

#pragma once
#include <iostream>
#include <string>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "../exceptions/exceptions.h"
#include "../err/err_code.h"
#include "../consts.h"
#include "../types.h"
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/serialization/version.hpp>

namespace dinosaur {
namespace cfg {

struct config
{
	std::string 		download_directory;
	uint16_t 			port;
	uint16_t 			write_cache_size;
	uint16_t 			read_cache_size;
	uint64_t 			tracker_default_interval;
	uint32_t			tracker_numwant;
	uint32_t 			max_active_seeders;
	uint32_t 			max_active_leechers;
	bool 				send_have;
	uint32_t 			listen_on;
	uint16_t			max_active_torrents;
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & download_directory;
		ar & port;
		ar & write_cache_size;
		ar & read_cache_size;
		ar & tracker_default_interval;
		ar & tracker_numwant;
		ar & max_active_seeders;
		ar & max_active_leechers;
		ar & send_have;
		ar & max_active_torrents;
	}
};
//BOOST_CLASS_VERSION(config, 1)

class Glob_cfg {
private:
	std::string				m_work_directory;
	std::string 			m_cfg_file_name;
	config 					cfg;
	dinosaur::PEER_ID		m_peer_id;
	int serialize()
	{
		std::ofstream ofs(m_cfg_file_name.c_str());
		if (ofs.fail())
			return -1;
		try
		{
			boost::archive::text_oarchive oa(ofs);
			oa << cfg;
		}
		catch(boost::archive::archive_exception & e)
		{
			return -1;
		}
		catch(...)
		{
			return -1;
		}
		return 0;
	}
	int deserialize()
	{
		std::ifstream ifs(m_cfg_file_name.c_str());
		if (ifs.fail())
			return -1;
		try
		{
			boost::archive::text_iarchive ia(ifs);
			ia >> cfg;
		}
		catch(boost::archive::archive_exception & e)
		{
			return -1;
		}
		catch(...)
		{
			return -1;
		}
		return 0;
	}
public:
	Glob_cfg(){}
	Glob_cfg(std::string work_directory)
	{
		m_work_directory = work_directory;
		//формируем путь к файлу вида $HOME/.dinosaur/config
		m_cfg_file_name = m_work_directory;
		m_cfg_file_name.append("config");
		if (deserialize() == -1)
		{
			init_default_config();
			serialize();
		}
		strncpy(m_peer_id, CLIENT_ID, PEER_ID_LENGTH);
	}
	int init_default_config()
	{
		//дефолтная папка для загрузок - пользовательская директория
		passwd *pw = getpwuid(getuid());
		if (pw == NULL)
			return ERR_SYSCALL_ERROR;
		cfg.download_directory = pw->pw_dir;

		if (cfg.download_directory[cfg.download_directory.length() - 1] != '/')
			cfg.download_directory.append("/");

		cfg.port = 23412;
		cfg.write_cache_size = 512;
		cfg.read_cache_size = 512;
		cfg.tracker_default_interval = 600;
		cfg.tracker_numwant = 300;
		cfg.max_active_seeders = 20;
		cfg.max_active_leechers = 10;
		cfg.send_have = true;
		inet_pton(AF_INET, "0.0.0.0", &cfg.listen_on);
		cfg.max_active_torrents = 5;
		return ERR_NO_ERROR;
	}
	const std::string & get_download_directory()
	{
		return cfg.download_directory;
	}
	void get_peer_id(dinosaur::PEER_ID c)
	{
		strncpy(c, m_peer_id, PEER_ID_LENGTH);
	}
	uint16_t get_port()
	{
		return cfg.port;
	}
	uint16_t get_write_cache_size()
	{
		return cfg.write_cache_size;
	}
	uint16_t get_read_cache_size()
	{
		return cfg.read_cache_size;
	}
	uint32_t get_tracker_numwant()
	{
		return cfg.tracker_numwant;
	}
	uint64_t get_tracker_default_interval()
	{
		return cfg.tracker_default_interval;
	}
	uint32_t get_max_active_seeders()
	{
		return cfg.max_active_seeders;
	}
	uint32_t get_max_active_leechers()
	{
		return cfg.max_active_leechers;
	}
	bool get_send_have()
	{
		return cfg.send_have;
	}
	void get_listen_on(IP_CHAR ip)
	{
		inet_ntop(AF_INET,  &cfg.listen_on, ip, INET_ADDRSTRLEN);
	}
	void get_listen_on(in_addr * addr)
	{
		memcpy(addr, &cfg.listen_on, sizeof(in_addr));
	}
	~Glob_cfg(){}
};

} /* namespace cfg */
}
