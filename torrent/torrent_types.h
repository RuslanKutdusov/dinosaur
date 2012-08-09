/*
 * torrent_types.h
 *
 *  Created on: 13.07.2012
 *      Author: ruslan
 */

#ifndef TORRENT_TYPES_H_
#define TORRENT_TYPES_H_

#include "../consts.h"
#include "../fs/fs.h"

namespace torrent
{

typedef unsigned char 		SHA1_HASH[SHA1_LENGTH];
typedef char 				SHA1_HASH_HEX[SHA1_HEX_LENGTH];
typedef unsigned char * 	BITFIELD;
typedef uint32_t			FILE_INDEX;
typedef uint32_t 			BLOCK_INDEX;
typedef uint32_t 			PIECE_INDEX;
typedef uint32_t 			BLOCK_OFFSET;
typedef uint64_t			FILE_OFFSET;


enum DOWNLOAD_PRIORITY
{
	DOWNLOAD_PRIORITY_LOW,
	DOWNLOAD_PRIORITY_NORMAL,
	DOWNLOAD_PRIORITY_HIGH
};

enum PIECE_STATE
{
	PIECE_STATE_NOT_FIN,
	PIECE_STATE_FIN_HASH_OK,
	PIECE_STATE_FIN_HASH_BAD
};

enum PEER_ADD
{
	PEER_ADD_TRACKER,
	PEER_ADD_INCOMING
};

enum TORRENT_WORK
{
	TORRENT_DOWNLOADING,
	TORRENT_UPLOADING,
	TORRENT_CHECKING,
	TORRENT_PAUSED,
	TORRENT_FAILURE
};

/*struct piece_info
{
	int 		file_index;//индекс файла, в котором начинается кусок
	uint32_t 	length;//его длина
	uint32_t 	remain;
	uint64_t 	offset;//смещение внутри файла до начала куска
	uint32_t 	block_count;//кол-во блоков в куске
};*/

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
		std::string 		status;
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


#endif /* TORRENT_TYPES_H_ */
