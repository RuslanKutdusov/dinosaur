/*
 * metafile.cpp
 *
 *  Created on: 13.07.2012
 *      Author: ruslan
 */

#include "metafile.h"

namespace dinosaur {
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

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */

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
		{
			bencode::_free(m_metafile);
			throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
		}

		if (bencode::get_str(info,"pieces",&str) == -1 || str->len % 20 != 0)
		{
			bencode::_free(m_metafile);
			throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
		}
		pieces	 = str->ptr;
	}
	memcpy(info_hash_bin, metafile.info_hash_bin, SHA1_LENGTH);
	memcpy(info_hash_hex, metafile.info_hash_hex, SHA1_HEX_LENGTH);
}

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */

Metafile & Metafile::operator = (const Metafile & metafile)
{
	if (this != &metafile)
	{
		if (m_metafile != NULL)
			bencode::_free(m_metafile);
		m_metafile = 		bencode::copy(metafile.m_metafile);
		m_metafile_len = 	metafile.m_metafile_len;
		announces = 		metafile.announces;
		comment = 			metafile.comment;
		created_by = 		metafile.created_by;
		creation_date = 	metafile.creation_date;
		private_ = 			metafile.private_;
		length = 			metafile.length;
		files = 			metafile.files;
		dir_tree = 			metafile.dir_tree;
		name = 				metafile.name;
		piece_length = 		metafile.piece_length;
		piece_count = 		metafile.piece_count;
		pieces = 			NULL;
		if (m_metafile != NULL)
		{
			bencode::be_node * info;
			bencode::be_str * str;
			if (bencode::get_node(m_metafile, "info", &info) == -1)
			{
				bencode::_free(m_metafile);
				throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
			}

			if (bencode::get_str(info,"pieces",&str) == -1 || str->len % 20 != 0)
			{
				bencode::_free(m_metafile);
				throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
			}
			pieces	 = str->ptr;
		}
		memcpy(info_hash_bin, metafile.info_hash_bin, SHA1_LENGTH);
		memcpy(info_hash_hex, metafile.info_hash_hex, SHA1_HEX_LENGTH);
	}
	return *this;
}

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */


Metafile::Metafile(const std::string & metafile_path)
{
	try
	{
		init(metafile_path.c_str());
	}
	catch(Exception & e)
	{
		if (m_metafile != NULL)
			bencode::_free(m_metafile);
		throw Exception(e);
	}
}

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */

Metafile::Metafile(const char * metafile_path)
{
	try
	{
		init(metafile_path);
	}
	catch(Exception & e)
	{
		if (m_metafile != NULL)
			bencode::_free(m_metafile);
		throw Exception(e);
	}
}

Metafile::~Metafile() {
	// TODO Auto-generated destructor stub
	//m_pieces осбождать не надо, осбождается в bencode::_free()
	if (m_metafile != NULL)
		bencode::_free(m_metafile);
}

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */

void Metafile::init(const char * metafile_path)
{
	char * buf = read_file(metafile_path, &m_metafile_len, MAX_TORRENT_FILE_LENGTH);
	if (buf == NULL)
		throw SyscallException();
	m_metafile = bencode::decode(buf, m_metafile_len, false);
#ifdef BITTORRENT_DEBUG
	bencode::dump(m_metafile);
#endif
	free(buf);
	if (!m_metafile)
		throw Exception(Exception::ERR_CODE_INVALID_METAFILE);

	get_announces();

	get_additional_info();

	get_main_info(m_metafile_len);
}

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */

void Metafile::get_announces()
{
	bencode::be_node * node;

	if (bencode::get_node(m_metafile,"announce-list",&node) == 0 && bencode::is_list(node))
	{
		bencode::be_node * l;
		for(int i = 0; bencode::get_node(node, i, &l) == 0; i++)
		{
			if (!bencode::is_list(l))
				throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
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
}

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */

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

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */

void Metafile::get_files_info(bencode::be_node * info)
{
	dir_tree = dir_tree::DirTree(name);
	bencode::be_node * files_node;
	if (bencode::get_node(info,"files",&files_node) == -1 || !bencode::is_list(files_node))
	{
		if (bencode::get_int(info,"length",&length) == -1 || length <= 0)
			throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
		files.resize(1);
		files[0].length = length;
		files[0].path = name;
	}
	else
	{//определяем кол-во файлов
		int files_count = get_list_size(files_node);
		if (files_count <= 0)
			throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
		files.resize(files_count);
		length = 0;
		for(int i = 0; i < files_count; i++)
		{
			bencode::be_node * file_node;// = temp.val.l[i];
			if ( bencode::get_node(files_node, i, &file_node) == -1 || !bencode::is_dict(file_node))
				throw Exception(Exception::ERR_CODE_INVALID_METAFILE);

			bencode::be_node * path_node;
			if (bencode::get_node(file_node, "path", &path_node) == -1 || !bencode::is_list(path_node))
				throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
			if (bencode::get_int(file_node, "length", &files[i].length) == -1)
				throw Exception(Exception::ERR_CODE_INVALID_METAFILE);

			dir_tree.reset();
			int path_list_size = bencode::get_list_size(path_node);
			files[i].index = i;
			for(int j = 0; j < path_list_size; j++)
			{
				bencode::be_str * str;
				if (bencode::get_str(path_node, j, &str) == -1)
					throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
				char * name = bencode::str2c_str(str);
				files[i].path += name;
				if (j < path_list_size - 1)
				{
					try
					{
						dir_tree.put(name);
					}
					catch(Exception & e)
					{
						throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
					}
					files[i].path += '/';
				}
				delete[] name;

			}
			length += files[i].length;
		}
	}
}

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */

void Metafile::get_main_info( uint64_t metafile_len)
{
	bencode::be_node * info;// = bencode::get_info_dict(m_metafile);
	bencode::be_str * str;
	if (bencode::get_node(m_metafile, "info", &info) == -1)
		throw Exception(Exception::ERR_CODE_INVALID_METAFILE);

	if (bencode::get_str(info,"name",&str) == -1)
		throw Exception(Exception::ERR_CODE_INVALID_METAFILE);

	char * c_str = bencode::str2c_str(str);
	name        = c_str;
	delete[] c_str;

	get_files_info(info);

	if (bencode::get_int(info,"piece length",&piece_length) == -1 || piece_length == 0)
		throw Exception(Exception::ERR_CODE_INVALID_METAFILE);

	if (bencode::get_str(info,"pieces",&str) == -1 || str->len % 20 != 0)
		throw Exception(Exception::ERR_CODE_INVALID_METAFILE);

	pieces	 = str->ptr;
	piece_count = str->len / 20;

	if (bencode::get_int(m_metafile,"private",&private_) == -1)
		bencode::get_int(info,"private",&private_);

	calculate_info_hash(info, metafile_len);
}

/*
 * Exception::ERR_CODE_INVALID_METAFILE
 */

void Metafile::calculate_info_hash(bencode::be_node * info, uint64_t metafile_len)
{
	memset(info_hash_bin,0,20);
	memset(info_hash_hex,0,41);
	char * bencoded_info = new char[metafile_len];
	uint32_t bencoded_info_len = 0;
	if (bencode::encode(info, &bencoded_info, metafile_len, &bencoded_info_len) != ERR_NO_ERROR)
	{
		delete[] bencoded_info;
		throw Exception(Exception::ERR_CODE_INVALID_METAFILE);
	}
	CSHA1 csha1;
	csha1.Update((unsigned char*)bencoded_info,bencoded_info_len);
	csha1.Final();
	csha1.ReportHash(info_hash_hex,CSHA1::REPORT_HEX);
	csha1.GetHash(info_hash_bin);
	delete[] bencoded_info;
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_BENCODE_ENCODE_ERROR
 * SyscallException
 */

void Metafile::save2file(const std::string & filepath)
{
	return save2file(filepath.c_str());
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_BENCODE_ENCODE_ERROR
 * SyscallException
 */

void Metafile::save2file(const char * filepath)
{
	if (filepath == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);

	if (m_metafile == NULL)
		throw Exception(Exception::ERR_CODE_UNDEF);

	char * bencoded_metafile = new(std::nothrow) char[m_metafile_len];
	if (bencoded_metafile == NULL)
		throw Exception(Exception::ERR_CODE_NO_MEMORY_AVAILABLE);
	uint32_t bencoded_metafile_len = 0;
	if (bencode::encode(m_metafile, &bencoded_metafile, m_metafile_len, &bencoded_metafile_len) != ERR_NO_ERROR)
	{
		delete[] bencoded_metafile;
		throw Exception(Exception::ERR_CODE_BENCODE_ENCODE_ERROR);
	}
	int fd = open(filepath, O_RDWR | O_CREAT, S_IRWXU);
	if (fd == -1)
	{
		delete[] bencoded_metafile;
		throw SyscallException();
	}
	ssize_t ret = write(fd, bencoded_metafile, bencoded_metafile_len);
	if (ret == -1)
	{
		int err = errno;
		delete[] bencoded_metafile;
		close(fd);
		throw SyscallException(err);
	}
	close(fd);
	delete[] bencoded_metafile;
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
	for(std::vector<info::file_stat>::iterator iter = files.begin(); iter != files.end(); ++iter)
	{
		std::cout<<"	"<<iter->path<<"	"<<iter->length<<std::endl;
	}
	std::cout<<"PIECE LENGHT: "<<piece_length<<std::endl;
	std::cout<<"PIECE COUNT: "<<piece_count<<std::endl;
	std::cout<<"INFOHASH: "<<info_hash_hex<<std::endl;
}

} /* namespace torrent */
}
