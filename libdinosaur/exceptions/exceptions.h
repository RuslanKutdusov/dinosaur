#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/archive/archive_exception.hpp>
#include "../consts.h"


namespace dinosaur {

class SyscallException
{
public:

private:
	int m_errno;
public:
	SyscallException()
	{
		m_errno = errno;
	}
	SyscallException(const SyscallException & a)
	{
		m_errno = a.m_errno;
	}
	SyscallException(int _errno)
	{
		m_errno = _errno;
	}
	int get_errno()
	{
		return m_errno;
	}
	const char * get_errno_str()
	{
		return sys_errlist[m_errno];
	}
};

class Exception
{
public:
	enum ERR_CODES
	{
		NO_ERROR,
		ERR_CODE_NULL_REF,
		ERR_CODE_UNDEF,
		ERR_CODE_INVALID_FORMAT,
		ERR_CODE_NO_MEMORY_AVAILABLE,
		ERR_CODE_CAN_NOT_CREATE_THREAD,

		ERR_CODE_SOCKET_CLOSED,
		ERR_CODE_SOCKET_CAN_NOT_SEND,
		ERR_CODE_CAN_NOT_RESOLVE,

		ERR_CODE_INVALID_METAFILE,
		ERR_CODE_INVALID_ANNOUNCE,
		ERR_CODE_LEECHER_REJECTED,
		ERR_CODE_SEED_REJECTED,
		ERR_CODE_INVALID_FILE_INDEX,
		ERR_CODE_FAIL_SET_FILE_PRIORITY,
		ERR_CODE_BLOCK_TOO_BIG,

		ERR_CODE_CAN_NOT_INIT_FILES,
		ERR_CODE_FILE_NOT_EXISTS,
		ERR_CODE_FILE_NOT_OPENED,
		ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW,

		ERR_CODE_DINOSAUR_INIT_ERROR,
		ERR_CODE_TORRENT_EXISTS,
		ERR_CODE_TORRENT_NOT_EXISTS,
		ERR_CODE_STATE_FILE_ERROR,

		ERR_CODE_INVALID_OPERATION,

		ERR_CODE_LRU_CACHE_NE,
		ERR_CODE_CACHE_FULL,

		ERR_CODE_BENCODE_PARSE_ERROR,
		ERR_CODE_BENCODE_ENCODE_ERROR,

		ERR_CODE_CAN_NOT_SAVE_CONFIG,
		ERR_CODE_DIR_NOT_EXISTS,
		ERR_CODE_INVALID_PORT,
		ERR_CODE_INVALID_W_CACHE_SIZE,
		ERR_CODE_INVALID_R_CACHE_SIZE,
		ERR_CODE_INVALID_TRACKER_NUM_WANT,
		ERR_CODE_INVALID_TRACKER_DEF_INTERVAL,
		ERR_CODE_INVALID_MAX_ACTIVE_SEEDS,
		ERR_CODE_INVALID_MAX_ACTIVE_LEECHS,
		ERR_CODE_INVALID_LISTEN_ON,
		ERR_CODE_INVALID_MAX_ACTIVE_TORRENTS,
		ERR_CODE_INVALID_FIN_RATIO,

		ERR_CODE_NODE_NOT_EXISTS
	};
private:
	ERR_CODES 	m_err_code;
	Exception(){}
public:
	Exception(ERR_CODES err_code)
	{
		m_err_code = err_code;
	}
	ERR_CODES get_errcode()
	{
		return m_err_code;
	}
};

struct ERR_CODES_STR
{
		std::string NO_ERROR;
		std::string ERR_CODE_NULL_REF;
		std::string ERR_CODE_UNDEF;
		std::string ERR_CODE_INVALID_FORMAT;
		std::string ERR_CODE_NO_MEMORY_AVAILABLE;
		std::string ERR_CODE_CAN_NOT_CREATE_THREAD;

		std::string ERR_CODE_SOCKET_CLOSED;
		std::string ERR_CODE_SOCKET_CAN_NOT_SEND;
		std::string ERR_CODE_CAN_NOT_RESOLVE;

		std::string ERR_CODE_INVALID_METAFILE;
		std::string ERR_CODE_INVALID_ANNOUNCE;
		std::string ERR_CODE_LEECHER_REJECTED;
		std::string ERR_CODE_SEED_REJECTED;
		std::string ERR_CODE_INVALID_FILE_INDEX;
		std::string ERR_CODE_FAIL_SET_FILE_PRIORITY;
		std::string ERR_CODE_BLOCK_TOO_BIG;

		std::string ERR_CODE_CAN_NOT_INIT_FILES;
		std::string ERR_CODE_FILE_NOT_EXISTS;
		std::string ERR_CODE_FILE_NOT_OPENED;
		std::string ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW;

		std::string ERR_CODE_DINOSAUR_INIT_ERROR;
		std::string ERR_CODE_TORRENT_EXISTS;
		std::string ERR_CODE_TORRENT_NOT_EXISTS;
		std::string ERR_CODE_STATE_FILE_ERROR;

		std::string ERR_CODE_INVALID_OPERATION;

		std::string ERR_CODE_LRU_CACHE_NE;
		std::string ERR_CODE_CACHE_FULL;

		std::string ERR_CODE_BENCODE_PARSE_ERROR;
		std::string ERR_CODE_BENCODE_ENCODE_ERROR;

		std::string ERR_CODE_CAN_NOT_SAVE_CONFIG;
		std::string ERR_CODE_DIR_NOT_EXISTS;
		std::string ERR_CODE_INVALID_PORT;
		std::string ERR_CODE_INVALID_W_CACHE_SIZE;
		std::string ERR_CODE_INVALID_R_CACHE_SIZE;
		std::string ERR_CODE_INVALID_TRACKER_NUM_WANT;
		std::string ERR_CODE_INVALID_TRACKER_DEF_INTERVAL;
		std::string ERR_CODE_INVALID_MAX_ACTIVE_SEEDS;
		std::string ERR_CODE_INVALID_MAX_ACTIVE_LEECHS;
		std::string ERR_CODE_INVALID_LISTEN_ON;
		std::string ERR_CODE_INVALID_MAX_ACTIVE_TORRENTS;
		std::string ERR_CODE_INVALID_FIN_RATIO;
		std::string ERR_CODE_NODE_NOT_EXISTS;
		friend class boost::serialization::access;
private:
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & BOOST_SERIALIZATION_NVP(NO_ERROR);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_NULL_REF);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_UNDEF);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_FORMAT);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_NO_MEMORY_AVAILABLE);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_CAN_NOT_CREATE_THREAD);

			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_SOCKET_CLOSED);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_SOCKET_CAN_NOT_SEND);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_CAN_NOT_RESOLVE);

			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_METAFILE);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_ANNOUNCE);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_LEECHER_REJECTED);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_SEED_REJECTED);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_FILE_INDEX);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_FAIL_SET_FILE_PRIORITY);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_BLOCK_TOO_BIG);

			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_CAN_NOT_INIT_FILES);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_FILE_NOT_EXISTS);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_FILE_NOT_OPENED);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW);

			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_DINOSAUR_INIT_ERROR);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_TORRENT_EXISTS);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_TORRENT_NOT_EXISTS);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_STATE_FILE_ERROR);

			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_OPERATION);

			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_LRU_CACHE_NE);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_CACHE_FULL);

			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_BENCODE_PARSE_ERROR);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_BENCODE_ENCODE_ERROR);

			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_CAN_NOT_SAVE_CONFIG);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_DIR_NOT_EXISTS);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_PORT);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_W_CACHE_SIZE);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_R_CACHE_SIZE);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_TRACKER_NUM_WANT);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_TRACKER_DEF_INTERVAL);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_MAX_ACTIVE_SEEDS);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_MAX_ACTIVE_LEECHS);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_LISTEN_ON);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_MAX_ACTIVE_TORRENTS);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_INVALID_FIN_RATIO);
			ar & BOOST_SERIALIZATION_NVP(ERR_CODE_NODE_NOT_EXISTS);
		}
public:
		const std::string & str_code(Exception::ERR_CODES err_code)
		{
			std::ifstream ifs(EXCEPTIONS_DESCRIPTIONS_FILE);
		    boost::archive::xml_iarchive ia(ifs);

		    ia >> BOOST_SERIALIZATION_NVP(*this);
		    switch(err_code)
		    {
				case(Exception::NO_ERROR): 								return NO_ERROR;
				case(Exception::ERR_CODE_NULL_REF): 					return ERR_CODE_NULL_REF;
				case(Exception::ERR_CODE_UNDEF): 						return ERR_CODE_UNDEF;
				case(Exception::ERR_CODE_INVALID_FORMAT): 				return ERR_CODE_INVALID_FORMAT;
				case(Exception::ERR_CODE_NO_MEMORY_AVAILABLE): 			return ERR_CODE_NO_MEMORY_AVAILABLE;
				case(Exception::ERR_CODE_CAN_NOT_CREATE_THREAD): 		return ERR_CODE_CAN_NOT_CREATE_THREAD;

				case(Exception::ERR_CODE_SOCKET_CLOSED): 				return ERR_CODE_SOCKET_CLOSED;
				case(Exception::ERR_CODE_SOCKET_CAN_NOT_SEND): 			return ERR_CODE_SOCKET_CAN_NOT_SEND;
				case(Exception::ERR_CODE_CAN_NOT_RESOLVE): 				return ERR_CODE_CAN_NOT_RESOLVE;

				case(Exception::ERR_CODE_INVALID_METAFILE): 			return ERR_CODE_INVALID_METAFILE;
				case(Exception::ERR_CODE_INVALID_ANNOUNCE): 			return ERR_CODE_INVALID_ANNOUNCE;
				case(Exception::ERR_CODE_LEECHER_REJECTED): 			return ERR_CODE_LEECHER_REJECTED;
				case(Exception::ERR_CODE_SEED_REJECTED): 				return ERR_CODE_SEED_REJECTED;
				case(Exception::ERR_CODE_INVALID_FILE_INDEX): 			return ERR_CODE_INVALID_FILE_INDEX;
				case(Exception::ERR_CODE_FAIL_SET_FILE_PRIORITY): 		return ERR_CODE_FAIL_SET_FILE_PRIORITY;
				case(Exception::ERR_CODE_BLOCK_TOO_BIG): 				return ERR_CODE_BLOCK_TOO_BIG;

				case(Exception::ERR_CODE_CAN_NOT_INIT_FILES): 			return ERR_CODE_CAN_NOT_INIT_FILES;
				case(Exception::ERR_CODE_FILE_NOT_EXISTS): 				return ERR_CODE_FILE_NOT_EXISTS;
				case(Exception::ERR_CODE_FILE_NOT_OPENED): 				return ERR_CODE_FILE_NOT_OPENED;
				case(Exception::ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW): 	return ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW;

				case(Exception::ERR_CODE_DINOSAUR_INIT_ERROR): 			return ERR_CODE_DINOSAUR_INIT_ERROR;
				case(Exception::ERR_CODE_TORRENT_EXISTS): 				return ERR_CODE_TORRENT_EXISTS;
				case(Exception::ERR_CODE_TORRENT_NOT_EXISTS): 			return ERR_CODE_TORRENT_NOT_EXISTS;
				case(Exception::ERR_CODE_STATE_FILE_ERROR): 			return ERR_CODE_STATE_FILE_ERROR;

				case(Exception::ERR_CODE_INVALID_OPERATION): 			return ERR_CODE_INVALID_OPERATION;

				case(Exception::ERR_CODE_LRU_CACHE_NE): 				return ERR_CODE_LRU_CACHE_NE;
				case(Exception::ERR_CODE_CACHE_FULL): 					return ERR_CODE_CACHE_FULL;

				case(Exception::ERR_CODE_BENCODE_PARSE_ERROR): 			return ERR_CODE_BENCODE_PARSE_ERROR;
				case(Exception::ERR_CODE_BENCODE_ENCODE_ERROR): 		return ERR_CODE_BENCODE_ENCODE_ERROR;

				case(Exception::ERR_CODE_CAN_NOT_SAVE_CONFIG): 			return ERR_CODE_CAN_NOT_SAVE_CONFIG;
				case(Exception::ERR_CODE_DIR_NOT_EXISTS): 				return ERR_CODE_DIR_NOT_EXISTS;
				case(Exception::ERR_CODE_INVALID_PORT): 				return ERR_CODE_INVALID_PORT;
				case(Exception::ERR_CODE_INVALID_W_CACHE_SIZE): 		return ERR_CODE_INVALID_W_CACHE_SIZE;
				case(Exception::ERR_CODE_INVALID_R_CACHE_SIZE): 		return ERR_CODE_INVALID_R_CACHE_SIZE;
				case(Exception::ERR_CODE_INVALID_TRACKER_NUM_WANT): 	return ERR_CODE_INVALID_TRACKER_NUM_WANT;
				case(Exception::ERR_CODE_INVALID_TRACKER_DEF_INTERVAL): return ERR_CODE_INVALID_TRACKER_DEF_INTERVAL;
				case(Exception::ERR_CODE_INVALID_MAX_ACTIVE_SEEDS): 	return ERR_CODE_INVALID_MAX_ACTIVE_SEEDS;
				case(Exception::ERR_CODE_INVALID_MAX_ACTIVE_LEECHS): 	return ERR_CODE_INVALID_MAX_ACTIVE_LEECHS;
				case(Exception::ERR_CODE_INVALID_LISTEN_ON): 			return ERR_CODE_INVALID_LISTEN_ON;
				case(Exception::ERR_CODE_INVALID_MAX_ACTIVE_TORRENTS): 	return ERR_CODE_INVALID_MAX_ACTIVE_TORRENTS;
				case(Exception::ERR_CODE_INVALID_FIN_RATIO): 			return ERR_CODE_INVALID_FIN_RATIO;
				case(Exception::ERR_CODE_NODE_NOT_EXISTS): 				return ERR_CODE_NODE_NOT_EXISTS;
		    }
		}
		static void save_defaults()
		{
			std::ofstream ofs(EXCEPTIONS_DESCRIPTIONS_FILE);
			boost::archive::xml_oarchive oa(ofs);

			struct ERR_CODES_STR ecs;
			ecs.NO_ERROR										= "No error";
			ecs.ERR_CODE_NULL_REF								= "Null reference";
			ecs.ERR_CODE_UNDEF 									= "Undefined error";
			ecs.ERR_CODE_INVALID_FORMAT							= "Invalid format";
			ecs.ERR_CODE_NO_MEMORY_AVAILABLE					= "No memory available";
			ecs.ERR_CODE_CAN_NOT_CREATE_THREAD					= "Can not create thread";

			ecs.ERR_CODE_SOCKET_CLOSED							= "Socket closed";
			ecs.ERR_CODE_SOCKET_CAN_NOT_SEND					= "Can not send";
			ecs.ERR_CODE_CAN_NOT_RESOLVE						= "Can not resolve domain name";

			ecs.ERR_CODE_INVALID_METAFILE						= "Invalid metafile";
			ecs.ERR_CODE_INVALID_ANNOUNCE						= "Invalid(unsupported) announce";
			ecs.ERR_CODE_LEECHER_REJECTED						= "Leecher rejected";
			ecs.ERR_CODE_SEED_REJECTED							= "Seed rejected";
			ecs.ERR_CODE_INVALID_FILE_INDEX						= "Invalid file index";
			ecs.ERR_CODE_FAIL_SET_FILE_PRIORITY					= "Can not set file priority";
			ecs.ERR_CODE_BLOCK_TOO_BIG							= "Block too big";

			ecs.ERR_CODE_CAN_NOT_INIT_FILES						= "Can not init files";
			ecs.ERR_CODE_FILE_NOT_EXISTS						= "File does not exists";
			ecs.ERR_CODE_FILE_NOT_OPENED						= "File does not opened";
			ecs.ERR_CODE_FILE_WRITE_OFFSET_OVERFLOW 			= "File write offset overflow";

			ecs.ERR_CODE_DINOSAUR_INIT_ERROR					= "Can not init dinosaur";
			ecs.ERR_CODE_TORRENT_EXISTS							= "Torrent already exists";
			ecs.ERR_CODE_TORRENT_NOT_EXISTS						= "Torrent does not exists";
			ecs.ERR_CODE_STATE_FILE_ERROR						= "State file operation error";

			ecs.ERR_CODE_INVALID_OPERATION 						= "Invalid operation";

			ecs.ERR_CODE_LRU_CACHE_NE							= "Element does not exists in LRU Cache";
			ecs.ERR_CODE_CACHE_FULL								= "Cache full";

			ecs.ERR_CODE_BENCODE_PARSE_ERROR 					= "Bencode parse error";
			ecs.ERR_CODE_BENCODE_ENCODE_ERROR					= "Bencode encoding error";

			ecs.ERR_CODE_CAN_NOT_SAVE_CONFIG 					= "Can not save config";
			ecs.ERR_CODE_DIR_NOT_EXISTS 						= "Directory does not exists";
			ecs.ERR_CODE_INVALID_PORT							= "Invalid port";
			ecs.ERR_CODE_INVALID_W_CACHE_SIZE 					= "Invalid write cache size";
			ecs.ERR_CODE_INVALID_R_CACHE_SIZE 					= "Invalid read cache size";
			ecs.ERR_CODE_INVALID_TRACKER_NUM_WANT				= "Ivalid tracker numwant";
			ecs.ERR_CODE_INVALID_TRACKER_DEF_INTERVAL 			= "Invalid default tracker interval";
			ecs.ERR_CODE_INVALID_MAX_ACTIVE_SEEDS 				= "Invalid max active seeders";
			ecs.ERR_CODE_INVALID_MAX_ACTIVE_LEECHS 				= "Invalid max active leechers";
			ecs.ERR_CODE_INVALID_LISTEN_ON 						= "Invalid listen on IP";
			ecs.ERR_CODE_INVALID_MAX_ACTIVE_TORRENTS 			= "Invalid max active torrents";
			ecs.ERR_CODE_INVALID_FIN_RATIO						= "Invalid final ratio";
			ecs.ERR_CODE_NODE_NOT_EXISTS						= "Such node does not exists";

			oa << BOOST_SERIALIZATION_NVP(ecs);
			ofs.close();
		}
};

const std::string exception_errcode2str(Exception::ERR_CODES err);
const std::string exception_errcode2str(Exception & e);
}
