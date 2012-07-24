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

namespace cfg {

struct config_file
{
	int version;
	char download_directory[MAX_FILENAME_LENGTH];
	uint16_t port;
	uint16_t cache_size;
	uint64_t tracker_default_interval;
	uint32_t tracker_numwant;
	uint32_t max_active_seeders;
	uint32_t max_active_leechers;
};

class Glob_cfg {
private:
	std::string m_work_directory;
	std::string m_download_directory; //папка для торрентов
	char m_peer_id[20];
	uint16_t m_port;
	uint16_t m_cache_size;
	uint64_t m_tracker_default_interval;
	uint32_t m_tracker_numwant;
	uint32_t m_max_active_seeders;
	uint32_t m_max_active_leechers;
public:
	Glob_cfg(){}
	Glob_cfg(std::string work_directory)
	{
		m_work_directory = work_directory;

		struct stat st;
			//формируем путь к файлу вида $HOME/.dinosaur/config
		std::string cfg_file_name = m_work_directory;
		cfg_file_name.append("config");
		bool _new = false;
		//если файла нет или его длина не равна размеру структуры => косяк, делаем заново
		if (stat(cfg_file_name.c_str(), &st) == -1 || (uint64_t)st.st_size != sizeof(config_file))
			_new = true;
		int fd = open(cfg_file_name.c_str(), O_RDWR | O_CREAT, S_IRWXU);
		if (fd == -1)
			throw Exception();
		if (_new)
		{
			if (init_default_config() != ERR_NO_ERROR || save_config(fd) != ERR_NO_ERROR)
			{
				close(fd);
				throw Exception();
			}
		}
		else
		{
			if (read_config(fd) != ERR_NO_ERROR)
			{
				close(fd);
				throw Exception();
			}
		}
		close(fd);
		strncpy(m_peer_id, CLIENT_ID, 20);
	}
	int save_config(int fd)
	{
		config_file cf;
		cf.version = CONFIG_FILE_VERSION;
		if (m_download_directory.length() > MAX_FILENAME_LENGTH)
			return ERR_INTERNAL;
		strncpy(cf.download_directory, m_download_directory.c_str(), m_download_directory.length());
		cf.port = m_port;
		cf.cache_size = m_cache_size;
		cf.tracker_default_interval = m_tracker_default_interval;
		cf.tracker_numwant = m_tracker_numwant;
		cf.max_active_seeders = m_max_active_seeders;
		cf.max_active_leechers = m_max_active_leechers;
		ssize_t ret = write(fd, &cf, sizeof(config_file));
		if (ret == -1)
			return ERR_SYSCALL_ERROR;
		return ERR_NO_ERROR;
	}
	int read_config(int fd)
	{
		config_file cf;
		ssize_t ret = read(fd, &cf, sizeof(config_file));
		if (ret == -1)
		{
			if (init_default_config() != ERR_NO_ERROR || save_config(fd) != ERR_NO_ERROR)
					return ERR_INTERNAL;
			return ERR_NO_ERROR;
		}
		if (cf.version != CONFIG_FILE_VERSION)
		{
			if (init_default_config() != ERR_NO_ERROR || save_config(fd) != ERR_NO_ERROR)
				return ERR_INTERNAL;
			return ERR_NO_ERROR;
		}
		m_port = cf.port;
		m_cache_size = cf.cache_size;
		m_tracker_default_interval = cf.tracker_default_interval;
		m_tracker_numwant = cf.tracker_numwant;
		m_download_directory = cf.download_directory;
		m_max_active_seeders = cf.max_active_seeders;
		m_max_active_leechers = cf.max_active_leechers;
		return ERR_NO_ERROR;
	}
	int init_default_config()
	{
		//дефолтная папка для загрузок - пользовательская директория
		passwd *pw = getpwuid(getuid());
		if (pw == NULL)
			return ERR_SYSCALL_ERROR;
		m_download_directory = pw->pw_dir;
		if (m_download_directory.length() <= 0)
			throw Exception();
		if (m_download_directory[m_download_directory.length() - 1] != '/')
			m_download_directory.append("/");
		m_port = 23412;
		m_tracker_numwant = 300;
		m_cache_size = 512;
		m_tracker_default_interval = 600;
		m_max_active_seeders = 20;
		m_max_active_leechers = 10;
		return ERR_NO_ERROR;
	}
	const std::string & get_download_directory()
	{
		return m_download_directory;
	}
	void get_peer_id(char * c)
	{
		if (c == NULL)
			return;
		strncpy(c, m_peer_id, 20);
	}
	uint16_t get_port()
	{
		return m_port;
	}
	uint16_t get_cache_size()
	{
		return m_cache_size;
	}
	uint32_t get_tracker_numwant()
	{
		return m_tracker_numwant;
	}
	uint64_t get_tracker_default_interval()
	{
		return m_tracker_default_interval;
	}
	uint32_t get_max_active_seeders()
	{
		return m_max_active_seeders;
	}
	uint32_t get_max_active_leechers()
	{
		return m_max_active_leechers;
	}
	~Glob_cfg(){}
};

} /* namespace cfg */
