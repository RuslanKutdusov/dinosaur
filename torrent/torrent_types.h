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

enum PEER_ADD
{
	PEER_ADD_TRACKER,
	PEER_ADD_INCOMING
};

struct peer_info
{
	char 		ip[22];
	uint64_t 	downloaded;
	uint64_t 	uploaded;
	double 		downSpeed;
	double 		upSpeed;
	double 		available;
};

class tracker_info
{
public:
	std::string 	announce;
	std::string 	status;
	uint64_t 		seeders;
	uint64_t 		leechers;
	time_t 			update_in;
};

struct file
{
	uint64_t 		length;
	std::string		name;
	bool 			download;
	fs::File 		file_;
};

struct base_file_info
{
	uint64_t 		length;
	std::string		name;
};

struct piece_info
{
	int 		file_index;//индекс файла, в котором начинается кусок
	uint32_t 	length;//его длина
	uint32_t 	remain;
	uint64_t 	offset;//смещение внутри файла до начала куска
	uint32_t 	block_count;//кол-во блоков в куске
};

class torrent_info
{
public:
	typedef std::list<peer_info> 				peer_info_list;
	typedef peer_info_list::iterator 			peer_info_iter;
	typedef std::list<tracker_info> 			tracker_info_list;
	typedef tracker_info_list::iterator 		tracker_info_iter;
	typedef std::pair<std::string, uint64_t> 	file;
	typedef std::list<file> 					file_list;
	typedef file_list::iterator 				file_list_iter;
public:
	std::string 		name;
	std::string 		comment;
	std::string 		created_by;
	std::string 		download_directory;
	uint64_t 			creation_date;
	uint64_t 			private_;
	uint64_t 			length;
	uint64_t 			piece_length;
	uint32_t 			piece_count;
	uint64_t 			downloaded;
	uint64_t 			uploaded;
	double 				rx_speed;
	double 				tx_speed;
	peer_info_list 		peers;
	tracker_info_list	trackers;
	file_list 			file_list_;
	SHA1_HASH_HEX 		info_hash_hex;
	int 				progress;
};

}


#endif /* TORRENT_TYPES_H_ */
