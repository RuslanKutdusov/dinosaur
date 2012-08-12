/*
 * types.h
 *
 *  Created on: 09.08.2012
 *      Author: ruslan
 */

#ifndef TYPES_H_
#define TYPES_H_

#include "consts.h"
#include <list>

namespace dinosaur
{

typedef unsigned char 		SHA1_HASH[SHA1_LENGTH];
typedef char 				SHA1_HASH_HEX[SHA1_HEX_LENGTH];
typedef unsigned char * 	BITFIELD;
typedef uint32_t			FILE_INDEX;
typedef uint32_t 			BLOCK_INDEX;
typedef uint32_t 			PIECE_INDEX;
typedef uint32_t 			BLOCK_OFFSET;
typedef uint64_t			FILE_OFFSET;
typedef char				IP_CHAR[IP_CHAR_LENGHT];
typedef char				PEER_ID[PEER_ID_LENGTH];

enum DOWNLOAD_PRIORITY
{
	DOWNLOAD_PRIORITY_LOW,
	DOWNLOAD_PRIORITY_NORMAL,
	DOWNLOAD_PRIORITY_HIGH
};

enum TORRENT_WORK
{
	TORRENT_WORK_DOWNLOADING,
	TORRENT_WORK_UPLOADING,
	TORRENT_WORK_CHECKING,
	TORRENT_WORK_PAUSED,
	TORRENT_WORK_FAILURE
};

struct socket_status
{
	bool 					listen;
	int 					errno_;
	Exception::ERR_CODES 	exception_errcode;
};

enum TRACKER_STATUS
{
	TRACKER_STATUS_OK,
	TRACKER_STATUS_UPDATING,
	TRACKER_STATUS_ERROR,
	TRACKER_STATUS_TIMEOUT,
	TRACKER_STATUS_UNRESOLVED,
	TRACKER_STATUS_INVALID_ANNOUNCE,
	TRACKER_STATUS_CONNECTING,
	TRACKER_STATUS_SEE_FAILURE_MESSAGE
};

enum TORRENT_FAILURE
{
	TORRENT_FAILURE_INITIALIZATION,
	TORRENT_FAILURE_WRITE_FILE,
	TORRENT_FAILURE_READ_FILE,
	TORRENT_FAILURE_GET_BLOCK_CACHE,
	TORRENT_FAILURE_PUT_BLOCK_CACHE
};

struct torrent_failure
{
	Exception::ERR_CODES	exception_errcode;
	int						errno_;
	TORRENT_FAILURE			where;
	std::string				description;
};

namespace info
{

	struct torrent_stat
	{
		std::string 		name;
		std::string 		comment;
		std::string 		created_by;
		std::string 		download_directory;
		uint64_t 			creation_date;
		uint64_t 			private_;
		uint64_t 			length;
		uint64_t 			piece_length;
		uint32_t 			piece_count;
		SHA1_HASH_HEX 		info_hash_hex;
	};

	struct torrent_dyn
	{
		uint64_t 			downloaded;
		uint64_t 			uploaded;
		double 				rx_speed;
		double 				tx_speed;
		uint32_t			seeders;
		uint32_t			total_seeders;
		uint32_t			leechers;
		int 				progress;
		TORRENT_WORK		work;
	};

	struct tracker
	{
		std::string 		announce;
		TRACKER_STATUS 		status;
		std::string			failure_mes;
		uint64_t 			seeders;
		uint64_t 			leechers;
		time_t 				update_in;
	};

	typedef std::list<tracker> trackers;

	struct file_stat
	{
		std::string 		path;
		uint64_t 			length;
		FILE_INDEX			index;
	};

	struct file_dyn
	{
		DOWNLOAD_PRIORITY	priority;
		uint64_t			downloaded;
	};

	typedef std::list<file_stat> files;

	struct peer
	{
		char 		ip[22];
		uint64_t 	downloaded;
		uint64_t 	uploaded;
		double 		downSpeed;
		double 		upSpeed;
		double 		available;
	};

	typedef std::list<peer> peers;
}

}


#endif /* TYPES_H_ */
