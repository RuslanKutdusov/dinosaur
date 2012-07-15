/*
 * metafile.cpp
 *
 *  Created on: 13.07.2012
 *      Author: ruslan
 */

#include "metafile.h"

namespace torrent {

Metafile::Metafile()
: m_metafile(NULL),
  m_metafile_len(0),
  creation_date(0),
  private_(0),
  length(0),
  piece_length(0),
  piece_count(0),
  pieces(NULL)
{
	memset(info_hash_bin, 0, SHA1_LENGTH);
	memset(info_hash_hex, '0', SHA1_HEX_LENGTH);
	info_hash_hex[SHA1_HEX_LENGTH - 1] = '\0';
}

Metafile::Metafile(const Metafile & metafile)
: m_metafile(bencode::copy(metafile.m_metafile)),
  m_metafile_len(metafile.m_metafile_len),
  announces(metafile.announces),
  comment(metafile.comment),
  created_by(metafile.created_by),
  creation_date(metafile.creation_date),
  private_(metafile.private_),
  length(metafile.length),
  files(metafile.files),
  dir_tree(metafile.dir_tree),
  name(metafile.name),
  piece_length(metafile.piece_length),
  piece_count(metafile.piece_count),
  pieces(NULL)
{
	if (m_metafile != NULL)
	{
		bencode::be_node * info;// = bencode::get_info_dict(m_metafile);
		bencode::be_str * str;
		if (bencode::get_node(m_metafile, "info", &info) == -1)
			throw Exception("Invalid metafile, can not get info dictionary");

		if (bencode::get_str(info,"pieces",&str) == -1 || str->len % 20 != 0)
			throw Exception("Invalid metafile");
		pieces	 = str->ptr;
	}
	memcpy(info_hash_bin, metafile.info_hash_bin, SHA1_LENGTH);
	memcpy(info_hash_hex, metafile.info_hash_hex, SHA1_HEX_LENGTH);
}

Metafile & Metafile::operator = (const Metafile & metafile)
{
	if (this != &metafile)
	{
		if (m_metafile != NULL)
			bencode::_free(m_metafile);
		m_metafile = 		bencode::copy(metafile.m_metafile);
		m_metafile_len = 	metafile.m_metafile_len;
		announces = 		metafile.announces;
		comment = 		metafile.comment;
		created_by = 		metafile.created_by;
		creation_date = 	metafile.creation_date;
		private_ = 		metafile.private_;
		length = 			metafile.length;
		files = 			metafile.files;
		dir_tree = 		metafile.dir_tree;
		name = 			metafile.name;
		piece_length = 	metafile.piece_length;
		piece_count = 	metafile.piece_count;
		pieces = NULL;
		if (m_metafile != NULL)
		{
			bencode::be_node * info;// = bencode::get_info_dict(m_metafile);
			bencode::be_str * str;
			if (bencode::get_node(m_metafile, "info", &info) == -1)
				throw Exception("Invalid metafile, can not get info dictionary");

			if (bencode::get_str(info,"pieces",&str) == -1 || str->len % 20 != 0)
				throw Exception("Invalid metafile");
			pieces	 = str->ptr;
		}
		memcpy(info_hash_bin, metafile.info_hash_bin, SHA1_LENGTH);
		memcpy(info_hash_hex, metafile.info_hash_hex, SHA1_HEX_LENGTH);
	}
	return *this;
}


Metafile::Metafile(const std::string & metafile_path)
{
	init(metafile_path.c_str());
}

Metafile::Metafile(const char * metafile_path)
{
	init(metafile_path);
}

Metafile::~Metafile() {
	// TODO Auto-generated destructor stub
	//m_pieces осбождать не надо, осбождается в bencode::_free()
	if (m_metafile != NULL)
		bencode::_free(m_metafile);
}

void Metafile::init(const char * metafile_path)
{
	char * buf = read_file(metafile_path, &m_metafile_len, MAX_TORRENT_FILE_LENGTH);
	if (buf == NULL)
		throw FileException(errno);
	m_metafile = bencode::decode(buf, m_metafile_len, true);
#ifdef BITTORRENT_DEBUG
	bencode::dump(m_metafile);
#endif
	free(buf);
	if (!m_metafile)
		throw Exception("Invalid metafile, parse error");

	if (get_announces() != ERR_NO_ERROR)
		throw Exception("Invalid metafile, parse error");

	get_additional_info();

	if (get_main_info(m_metafile_len) != ERR_NO_ERROR)
		throw Exception("Invalid metafile, parse error");
}

int Metafile::get_announces()
{
	if (m_metafile == NULL)
		return ERR_NULL_REF;
	bencode::be_node * node;

	if (bencode::get_node(m_metafile,"announce-list",&node) == 0 && bencode::is_list(node))
	{
		bencode::be_node * l;
		for(int i = 0; bencode::get_node(node, i, &l) == 0; i++)
		{
			if (!bencode::is_list(l))
				return ERR_INTERNAL;
			bencode::be_str * str;
			for(int j = 0; bencode::get_str(l, j, &str) == 0; j++)
			{
				char *c_str = bencode::str2c_str(str);
				announces.push_back(c_str);
				delete[] c_str;
			}
		}
	}
	else
	{
		bencode::be_str * str;
		//отсутствие трекеров не ошибка
		if (bencode::get_str(m_metafile,"announce",&str) == ERR_NO_ERROR)
		{
			char *c_str = bencode::str2c_str(str);
			announces.push_back(c_str);
			delete[] c_str;
		}
	}
	return ERR_NO_ERROR;
}

void Metafile::get_additional_info()
{
	if (m_metafile == NULL)
		return;
	bencode::be_str * str;
	char * c_str;
	if (bencode::get_str(m_metafile,"comment",&str) == 0)
	{
		c_str = bencode::str2c_str(str);
		comment        = c_str;
		delete[] c_str;
	}

	if (bencode::get_str(m_metafile,"created by",&str) == 0)
	{
		c_str = bencode::str2c_str(str);
		created_by        = c_str;
		delete[] c_str;
	}

	bencode::get_int(m_metafile,"creation date",&creation_date);
}

int Metafile::get_files_info(bencode::be_node * info)
{
	if (m_metafile == NULL || info == NULL)
		return ERR_NULL_REF;
	dir_tree = dir_tree::DirTree(name);
	bencode::be_node * files_node;
	if (bencode::get_node(info,"files",&files_node) == -1 || !bencode::is_list(files_node))
	{
		if (bencode::get_int(info,"length",&length) == -1 || length <= 0)
			return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get key length");
		files.resize(1);
		files[0].length = length;
		files[0].name = name;
	}
	else
	{//определяем кол-во файлов
		int files_count = get_list_size(files_node);
		//std::cout<<m_files_count<<std::endl;
		if (files_count <= 0)
			return ERR_INTERNAL;
		files.resize(files_count);
		length = 0;
		for(int i = 0; i < files_count; i++)
		{
			bencode::be_node * file_node;// = temp.val.l[i];
			if ( bencode::get_node(files_node, i, &file_node) == -1 || !bencode::is_dict(file_node))
				return ERR_INTERNAL;

			bencode::be_node * path_node;
			if (bencode::get_node(file_node, "path", &path_node) == -1 || !bencode::is_list(path_node))
				return ERR_INTERNAL;
			if (bencode::get_int(file_node, "length", &files[i].length) == -1)
				return ERR_INTERNAL;

			dir_tree.reset();
			int path_list_size = bencode::get_list_size(path_node);
			//int substring_offset = 0;
			for(int j = 0; j < path_list_size; j++)
			{
				bencode::be_str * str;
				if (bencode::get_str(path_node, j, &str) == -1)
					return ERR_INTERNAL;
				//m_files[i].name.resize(m_files[i].name.size() + str->len + 1);
				//strncpy(&m_files[i].name[substring_offset], str->ptr, str->len);
				char * name = bencode::str2c_str(str);
				files[i].name += name;
				if (j < path_list_size - 1)
				{
					//m_files[i].name[substring_offset + str->len] = '\0';
					if (dir_tree.put(name) != ERR_NO_ERROR)
						return ERR_INTERNAL;
					files[i].name += '/';
				}
				delete[] name;

				//substring_offset += str->len;
				//m_files[i].name[substring_offset++] = '/';
			}
			//m_files[i].name[m_files[i].name.size() - 1] = '\0';
			length += files[i].length;
		}
	}
	return ERR_NO_ERROR;
}

int Metafile::get_main_info( uint64_t metafile_len)
{
	if (m_metafile == NULL)
		return ERR_NULL_REF;

	bencode::be_node * info;// = bencode::get_info_dict(m_metafile);
	bencode::be_str * str;
	if (bencode::get_node(m_metafile, "info", &info) == -1)
		return ERR_INTERNAL;

	if (bencode::get_str(info,"name",&str) == -1)
		return ERR_INTERNAL;
	char * c_str = bencode::str2c_str(str);
	name        = c_str;
	delete[] c_str;

	get_files_info(info);

	if (bencode::get_int(info,"piece length",&piece_length) == -1 || piece_length == 0)
		return ERR_INTERNAL;

	if (bencode::get_str(info,"pieces",&str) == -1 || str->len % 20 != 0)
		return ERR_INTERNAL;
	pieces	 = str->ptr;
	piece_count = str->len / 20;

	if (bencode::get_int(m_metafile,"private",&private_) == -1)
		bencode::get_int(info,"private",&private_);

	return calculate_info_hash(info, metafile_len);
}

int Metafile::calculate_info_hash(bencode::be_node * info, uint64_t metafile_len)
{
	if (info == NULL || metafile_len == 0)
		return ERR_BAD_ARG;
	memset(info_hash_bin,0,20);
	memset(info_hash_hex,0,41);
	char * bencoded_info = new char[metafile_len];
	uint32_t bencoded_info_len = 0;
	if (bencode::encode(info, &bencoded_info, metafile_len, &bencoded_info_len) != ERR_NO_ERROR)
	{
		delete[] bencoded_info;
		return ERR_INTERNAL;
	}
	bencode::dump(info);
	CSHA1 csha1;
	csha1.Update((unsigned char*)bencoded_info,bencoded_info_len);
	csha1.Final();
	csha1.ReportHash(info_hash_hex,CSHA1::REPORT_HEX);
	csha1.GetHash(info_hash_bin);
	delete[] bencoded_info;
	return ERR_NO_ERROR;
}

int Metafile::save2file(const std::string & filepath)
{
	return save2file(filepath.c_str());
}

int Metafile::save2file(const char * filepath)
{
	if (filepath == NULL)
		return ERR_BAD_ARG;

	if (m_metafile == NULL)
		return ERR_NULL_REF;

	char * bencoded_metafile = new(std::nothrow) char[m_metafile_len];
	if (bencoded_metafile == NULL)
		return ERR_INTERNAL;
	uint32_t bencoded_metafile_len = 0;
	if (bencode::encode(m_metafile, &bencoded_metafile, m_metafile_len, &bencoded_metafile_len) != ERR_NO_ERROR)
	{
		delete[] bencoded_metafile;
		return ERR_INTERNAL;
	}
	int fd = open(filepath, O_RDWR | O_CREAT, S_IRWXU);
	if (fd == -1)
	{
		delete[] bencoded_metafile;
		return ERR_SYSCALL_ERROR;
	}
	ssize_t ret = write(fd, bencoded_metafile, bencoded_metafile_len);
	if (ret == -1)
	{
		delete[] bencoded_metafile;
		return ERR_SYSCALL_ERROR;
		close(fd);
		return ERR_SYSCALL_ERROR;
	}
	close(fd);
	delete[] bencoded_metafile;
	return ERR_NO_ERROR;
}

void Metafile::dump()
{
	std::cout<<"ANNOUNCES:"<<std::endl;
	for(std::vector<std::string>::iterator iter = announces.begin(); iter != announces.end(); ++iter)
	{
		std::cout<<"	"<<*iter<<std::endl;
	}
	std::cout<<"COMMENT: "<<comment<<std::endl;
	std::cout<<"CREATED BY: "<<created_by<<std::endl;
	std::cout<<"CREATION DATA: "<<creation_date<<std::endl;
	std::cout<<"PRIVATE: "<<private_<<std::endl;
	std::cout<<"LENGHT: "<<length<<std::endl;
	std::cout<<"FILES:"<<std::endl;
	for(std::vector<file_info>::iterator iter = files.begin(); iter != files.end(); ++iter)
	{
		std::cout<<"	"<<iter->name<<"	"<<iter->length<<std::endl;
	}
	std::cout<<"PIECE LENGHT: "<<piece_length<<std::endl;
	std::cout<<"PIECE COUNT: "<<piece_count<<std::endl;
	std::cout<<"INFOHASH: "<<info_hash_hex<<std::endl;
}

} /* namespace torrent */
