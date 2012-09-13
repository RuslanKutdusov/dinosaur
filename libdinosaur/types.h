/*
 * types.h
 *
 *  Created on: 09.08.2012
 *      Author: ruslan
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <list>
#include <vector>
#include <stdint.h>
#include "consts.h"
#include "exceptions/exceptions.h"

namespace dinosaur
{

//typedef unsigned char 		SHA1_HASH[SHA1_LENGTH];
typedef std::string			SHA1_HASH_HEX;
typedef unsigned char * 	BITFIELD;
typedef uint64_t			FILE_INDEX;
typedef uint32_t 			BLOCK_INDEX;
typedef uint32_t 			PIECE_INDEX;
typedef uint32_t 			BLOCK_OFFSET;
typedef uint64_t			FILE_OFFSET;
typedef char				IP_CHAR[IP_CHAR_LENGHT];
typedef char				PEER_ID[PEER_ID_LENGTH];

class SHA1_HASH
{
protected:
	unsigned char m_data[SHA1_LENGTH];
	void clear();
public:
	SHA1_HASH();
	SHA1_HASH(const SHA1_HASH & hash);
	SHA1_HASH(const char * hash);
	SHA1_HASH(const unsigned char * hash);
	virtual ~SHA1_HASH();
	SHA1_HASH & operator=(const SHA1_HASH & hash);
	SHA1_HASH & operator=(const char * hash);
	SHA1_HASH & operator=(const unsigned char * hash);
	unsigned char & operator[](size_t i);
	const unsigned char & operator[](size_t i) const;
	bool operator<(const SHA1_HASH & hash) const;
	bool operator==(const SHA1_HASH & hash) const;
	bool operator!=(const SHA1_HASH & hash) const;
	void copy2(char * dst) const;
	void copy2(unsigned char * dst) const;
	void print() const;
	void to_hex(SHA1_HASH_HEX & hash) const ;
};

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
	TORRENT_WORK_FAILURE,
	TORRENT_WORK_RELEASING,
	TORRENT_WORK_DONE
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
	TORRENT_FAILURE_NONE,
	TORRENT_FAILURE_INIT_TORRENT,
	TORRENT_FAILURE_WRITE_FILE,
	TORRENT_FAILURE_READ_FILE,
	TORRENT_FAILURE_GET_BLOCK_CACHE,
	TORRENT_FAILURE_PUT_BLOCK_CACHE,
	TORRENT_FAILURE_DOWNLOADING
};

struct torrent_failure
{
	Exception::ERR_CODES	exception_errcode;
	int						errno_;
	TORRENT_FAILURE			where;
	std::string				description;
};

typedef std::list<torrent_failure> torrent_failures;

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
		time_t				start_time;
		uint64_t			files_count;
	};

	struct torrent_dyn
	{
		uint64_t 			downloaded;
		uint64_t 			uploaded;
		int 				rx_speed;
		int 				tx_speed;
		uint32_t			seeders;
		uint32_t			total_seeders;
		uint32_t			leechers;
		int 				progress;
		TORRENT_WORK		work;
		time_t				remain_time;
		float				ratio;
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

	typedef std::vector<file_stat> files_stat;

	struct peer
	{
		char 		ip[22];
		uint64_t 	downloaded;
		uint64_t 	uploaded;
		int 		downSpeed;
		int 		upSpeed;
		double 		available;
		uint32_t	blocks2request;
		uint32_t	requested_blocks;
		bool 		may_request;
	};
	typedef std::list<peer> peers;

	struct downloadable_piece
	{
		PIECE_INDEX 		index;
		uint32_t			block2download;
		uint32_t			downloaded_blocks;
		DOWNLOAD_PRIORITY 	priority;
	};
	typedef std::list<downloadable_piece> downloadable_pieces;
}

}


#endif /* TYPES_H_ */
