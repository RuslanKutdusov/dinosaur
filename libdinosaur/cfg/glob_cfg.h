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
		ar & listen_on;
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
	/*
	 * Exception::ERR_CODE_CAN_NOT_SAVE_CONFIG
	 */
	int serialize()
	{
		std::ofstream ofs(m_cfg_file_name.c_str());
		if (ofs.fail())
			throw Exception(Exception::ERR_CODE_CAN_NOT_SAVE_CONFIG);
		try
		{
			boost::archive::text_oarchive oa(ofs);
			oa << cfg;
		}
		catch(boost::archive::archive_exception & e)
		{
			ofs.close();
			throw Exception(Exception::ERR_CODE_CAN_NOT_SAVE_CONFIG);
		}
		catch(...)
		{
			ofs.close();
			throw Exception(Exception::ERR_CODE_CAN_NOT_SAVE_CONFIG);
		}
		ofs.close();
		return 0;
	}
	int deserialize()
	{
		std::ifstream ifs(m_cfg_file_name.c_str());
		if (ifs.fail())
		{
			ifs.close();
			return -1;
		}
		try
		{
			boost::archive::text_iarchive ia(ifs);
			ia >> cfg;
		}
		catch(...)
		{
			ifs.close();
			return -1;
		}
		ifs.close();
		return 0;
	}
	/*
	 * SyscallException
	 */
	int init_default_config()
	{
		//дефолтная папка для загрузок - пользовательская директория
		errno = 0;
		passwd *pw = getpwuid(getuid());
		if (pw == NULL)
			throw SyscallException();
		cfg.download_directory = pw->pw_dir;

		if (cfg.download_directory[cfg.download_directory.length() - 1] != '/')
			cfg.download_directory.append("/");

		cfg.port = 6881;
		cfg.write_cache_size = 512;
		cfg.read_cache_size = 512;
		cfg.tracker_default_interval = 600;
		cfg.tracker_numwant = 300;
		cfg.max_active_seeders = 20;
		cfg.max_active_leechers = 10;
		cfg.send_have = true;
		inet_pton(AF_INET, "0.0.0.0", &cfg.listen_on);
		cfg.max_active_torrents = 5;
		return 0;
	}
public:
	Glob_cfg(){}
	/*
	 * SyscallException
	 * Exception::ERR_CODE_CAN_NOT_SAVE_CONFIG
	 */
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
	/*
	* Exception::ERR_CODE_CAN_NOT_SAVE_CONFIG
	*/
	void save()
	{
		serialize();
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
	uint16_t get_max_active_torrents()
	{
		return cfg.max_active_torrents;
	}


	/*
	 * Exception::ERR_CODE_DIR_NOT_EXISTS
	 * SyscallException
	 */
	void set_download_directory(const char * dir)
	{
		struct stat st;
		if(stat(dir, &st) == 0)
		{
			if (S_ISDIR(st.st_mode))
				cfg.download_directory = dir;
			else
				throw Exception(Exception::ERR_CODE_DIR_NOT_EXISTS);
		}
		else
			throw SyscallException();
	}
	/*
	 * Exception::ERR_CODE_DIR_NOT_EXISTS
	 * SyscallException
	 */
	void set_download_directory(const std::string & dir)
	{
		set_download_directory(dir.c_str());
	}
	void set_port(uint16_t port)
	{
		cfg.port = port;
	}
	/*
	 * Exception::ERR_CODE_INVALID_CONFIG_VALUE
	 */
	void set_write_cache_size(uint16_t s)
	{
		if (s == 0)
			throw Exception(Exception::ERR_CODE_INVALID_W_CACHE_SIZE);
		cfg.write_cache_size = s;
	}
	/*
	 * Exception::ERR_CODE_INVALID_CONFIG_VALUE
	 */
	void set_read_cache_size(uint16_t s)
	{
		if (s == 0)
			throw Exception(Exception::ERR_CODE_INVALID_R_CACHE_SIZE);
		cfg.read_cache_size = s;
	}
	void set_tracker_numwant(uint32_t v)
	{
		cfg.tracker_numwant = v;
	}
	/*
	 * Exception::ERR_CODE_INVALID_CONFIG_VALUE
	 */
	void set_tracker_default_interval(uint64_t v)
	{
		if (v == 0)
			throw Exception(Exception::ERR_CODE_INVALID_TRACKER_DEF_INTERVAL);
		cfg.tracker_default_interval = v;
	}
	/*
	 * Exception::ERR_CODE_INVALID_CONFIG_VALUE
	 */
	void set_max_active_seeders(uint32_t v)
	{
		if (v == 0)
			throw Exception(Exception::ERR_CODE_INVALID_MAX_ACTIVE_SEEDS);
		cfg.max_active_seeders = v;
	}
	/*
	 * Exception::ERR_CODE_INVALID_CONFIG_VALUE
	 */
	void set_max_active_leechers(uint32_t v)
	{
		if (v == 0)
			throw Exception(Exception::ERR_CODE_INVALID_MAX_ACTIVE_LEECHS);
		cfg.max_active_leechers = v;
	}
	void set_send_have(bool v)
	{
		cfg.send_have = v;
	}
	/*
	 * Exception::ERR_CODE_INVALID_CONFIG_VALUE
	 */
	void set_listen_on(IP_CHAR ip)
	{
		if (inet_pton(AF_INET, ip, &cfg.listen_on) <= 0)
			throw Exception(Exception::ERR_CODE_INVALID_LISTEN_ON);
	}
	void set_listen_on(in_addr * addr)
	{
		if (addr == NULL)
			throw Exception(Exception::ERR_CODE_NULL_REF);
		memcpy(&cfg.listen_on, addr, sizeof(in_addr));
	}
	/*
	 * Exception::ERR_CODE_INVALID_CONFIG_VALUE
	 */
	void set_max_active_torrents(uint16_t v)
	{
		if (v == 0)
			throw Exception(Exception::ERR_CODE_INVALID_MAX_ACTIVE_TORRENTS);
		cfg.max_active_torrents = v;
	}
	~Glob_cfg(){}
};

} /* namespace cfg */
}
