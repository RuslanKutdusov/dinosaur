/*
 * metafile.h
 *
 *  Created on: 13.07.2012
 *      Author: ruslan
 */

#ifndef METAFILE_H_
#define METAFILE_H_

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../utils/bencode.h"
#include "../utils/dir_tree.h"
#include "../consts.h"
#include "../err/err_code.h"
#include "../utils/sha1.h"
#include "../exceptions/exceptions.h"
#include "../utils/utils.h"
#include "../types.h"
#include "torrent_types.h"

namespace dinosaur {
namespace torrent {

class Metafile {
private:
	bencode::be_node * 			m_metafile;
	uint64_t 					m_metafile_len;
	void init(const char * metafile_path);
	void get_announces();
	void get_additional_info();
	void get_files_info(bencode::be_node * info);
	void get_main_info( uint64_t metafile_len);
	void calculate_info_hash(bencode::be_node * info, uint64_t metafile_len);
public:
	std::vector<std::string> 		announces;
	std::string 					comment;
	std::string 					created_by;
	uint64_t 						creation_date;
	uint64_t 						private_;
	uint64_t 						length;
	info::files_stat				files;
	dir_tree::DirTree				dir_tree;
	std::string 					name;
	uint64_t 						piece_length;
	uint32_t 						piece_count;
	char * 							pieces;
	dinosaur::SHA1_HASH 			info_hash_bin;
	dinosaur::SHA1_HASH_HEX 		info_hash_hex;
	Metafile();
	Metafile(const Metafile & metafile);
	Metafile & operator = (const Metafile & metafile);
	Metafile(const std::string & metafile_path);
	Metafile(const char * metafile_path);
	virtual ~Metafile();
	void save2file(const std::string & filepath);
	void save2file(const char * filepath);
	void dump();
};
typedef boost::shared_ptr<Metafile> MetafilePtr;

} /* namespace torrent */
}
#endif /* METAFILE_H_ */
