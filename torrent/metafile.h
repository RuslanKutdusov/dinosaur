/*
 * metafile.h
 *
 *  Created on: 13.07.2012
 *      Author: ruslan
 */

#ifndef METAFILE_H_
#define METAFILE_H_

#include "../utils/bencode.h"
#include "../utils/dir_tree.h"
#include "../consts.h"
#include "../err/err_code.h"
#include "../utils/sha1.h"
#include "torrent_types.h"

namespace torrent {

class Metafile {
private:
	bencode::be_node * m_metafile;
	uint64_t m_metafile_len;
	void init(const char * metafile_path);
	int get_announces();
	void get_additional_info();
	int get_files_info(bencode::be_node * info);
	int get_main_info( uint64_t metafile_len);
	int calculate_info_hash(bencode::be_node * info, uint64_t metafile_len);
public:
	std::vector<std::string> m_announces;
	std::string m_comment;
	std::string m_created_by;
	uint64_t m_creation_date;
	uint64_t m_private;
	uint64_t m_length;
	std::vector<file_info> m_files;
	dir_tree::DirTree * m_dir_tree;//почему ссылка, а не просто член класса?????????????????????????????????????????????????///
	std::string m_name;
	uint64_t m_piece_length;
	uint32_t m_piece_count;
	char * m_pieces;
	SHA1_HASH m_info_hash_bin;
	SHA1_HASH_HEX m_info_hash_hex;
	Metafile(const std::string & metafile_path);
	Metafile(const char * metafile_path);
	virtual ~Metafile();
	int save_meta2file(const std::string & filepath);
	int save_meta2file(const char * filepath);
};

} /* namespace torrent */
#endif /* METAFILE_H_ */
