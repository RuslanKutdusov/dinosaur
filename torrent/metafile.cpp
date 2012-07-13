/*
 * metafile.cpp
 *
 *  Created on: 13.07.2012
 *      Author: ruslan
 */

#include "metafile.h"

namespace torrent {

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
	if (m_metafile != NULL)
		bencode::_free(m_metafile);
	for (int i = 0; i < m_files.size(); i++)
	{
		if (m_files[i].name != NULL)
			delete[] m_files[i].name;
	}
	if (m_dir_tree != NULL)
		delete m_dir_tree;
}

void Metafile::init(const char * metafile_path)
{
	char * buf = read_file(metafile_path, &m_metafile_len, MAX_TORRENT_FILE_LENGTH);
	if (buf == NULL)
	{
		throw FileException(errno);
	}
	m_metafile = bencode::decode(buf, m_metafile_len, true);
#ifdef BITTORRENT_DEBUG
	bencode::dump(m_metafile);
#endif
	free(buf);
	if (!m_metafile)
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		throw Exception("Invalid metafile, parse error");
	}

	if (get_announces_from_metafile() != ERR_NO_ERROR)
		throw Exception("Invalid metafile, parse error");

	get_additional_info_from_metafile();

	if (get_main_info_from_metafile(m_metafile_len) != ERR_NO_ERROR)
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
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, bad announce-list");
			}
			bencode::be_str * str;
			for(int j = 0; bencode::get_str(l, j, &str) == 0; j++)
			{
				char *c_str = bencode::str2c_str(str);
				m_announces.push_back(c_str);
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
			m_announces.push_back(c_str);
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
		m_comment        = c_str;
		delete[] c_str;
	}

	if (bencode::get_str(m_metafile,"created by",&str) == 0)
	{
		c_str = bencode::str2c_str(str);
		m_created_by        = c_str;
		delete[] c_str;
	}

	bencode::get_int(m_metafile,"creation date",&m_creation_date);
}

int Metafile::get_files_info(bencode::be_node * info)
{
	if (m_metafile == NULL || info == NULL)
		return ERR_NULL_REF;
	m_dir_tree = new dir_tree::DirTree(m_name);
	bencode::be_node * files_node;
	if (bencode::get_node(info,"files",&files_node) == -1 || !bencode::is_list(files_node))
	{
		if (bencode::get_int(info,"length",&m_length) == -1 || m_length <= 0)
		{
			m_error = TORRENT_ERROR_INVALID_METAFILE;
			return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get key length");
		}
		m_files = new file_info[1];
		m_files[0].length = m_length;
		ssize_t sl = m_name.length();
		//std::cout<<sl<<std::endl;
		m_files[0].name = new char[sl + 1];
		memset(m_files[0].name, 0, sl + 1);
		strncpy(m_files[0].name, m_name.c_str(), sl);
		//m_files->name[sl] = '\0';
		m_files[0].download = true;
		m_files_count = 1;
	}
	else
	{//определяем кол-во файлов
		m_files_count = get_list_size(files_node);
		//std::cout<<m_files_count<<std::endl;
		if (m_files_count <= 0)
		{
			m_error = TORRENT_ERROR_INVALID_METAFILE;
			return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get list size");
		}
		//выделяем память
		m_files = new file_info[m_files_count];
		//memset(m_files, 0, sizeof(file_info) * m_files_count);
		m_length = 0;
		for(int i = 0; i < m_files_count; i++)
		{
			bencode::be_node * file_node;// = temp.val.l[i];
			if ( bencode::get_node(files_node, i, &file_node) == -1 || !bencode::is_dict(file_node))
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
			}

			bencode::be_node * path_node;
			if (bencode::get_node(file_node, "path", &path_node) == -1 || !bencode::is_list(path_node))
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
			}
			if (bencode::get_int(file_node, "length", &m_files[i].length) == -1)
			{
				m_error = TORRENT_ERROR_INVALID_METAFILE;
				return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
			}

			m_dir_tree->reset();
			int path_length = 0;
			int path_list_size = bencode::get_list_size(path_node);
			for(int j = 0; j < path_list_size; j++)
			{
				bencode::be_str * str;
				if (bencode::get_str(path_node, j, &str) == -1)
				{
					m_error = TORRENT_ERROR_INVALID_METAFILE;
					return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
				}
				path_length += str->len + 1;// 1 for slash and \0 at the end;
			}
			m_files[i].name = new char[path_length];
			int substring_offset = 0;
			for(int j = 0; j < path_list_size; j++)
			{
				bencode::be_str * str;
				if (bencode::get_str(path_node, j, &str) == -1)
				{
					m_error = TORRENT_ERROR_INVALID_METAFILE;
					return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get files dictionary");
				}
				strncpy(&m_files[i].name[substring_offset], str->ptr, str->len);
				if (j < path_list_size - 1)
				{
					m_files[i].name[substring_offset + str->len] = '\0';
					if (m_dir_tree->put(&m_files[i].name[substring_offset]) != ERR_NO_ERROR)
					{
						m_error = GENERAL_ERROR_UNDEF_ERROR;
						return ERR_INTERNAL;
					}
				}

				substring_offset += str->len;
				m_files[i].name[substring_offset++] = '/';
			}
			m_files[i].name[path_length - 1] = '\0';
			m_files[i].download = true;
			m_length += m_files[i].length;
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
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get info dictionary");
	}

	if (bencode::get_str(info,"name",&str) == -1)
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL; //throw Exception("Invalid metafile, can not get key name");
	}
	char * c_str = bencode::str2c_str(str);
	m_name        = c_str;
	delete[] c_str;

	get_files_info_from_metafile(info);

	if (bencode::get_int(info,"piece length",&m_piece_length) == -1 || m_piece_length == 0)
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL; //throw Exception("Invalid metafile");
	}

	if (bencode::get_str(info,"pieces",&str) == -1 || str->len % 20 != 0)
	{
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL; //throw Exception("Invalid metafile");
	}
	m_pieces	 = str->ptr;
	m_piece_count = str->len / 20;

	if (bencode::get_int(m_metafile,"private",&m_private) == -1)
		bencode::get_int(info,"private",&m_private);

	return calculate_info_hash(info, metafile_len);
}

int Metafile::calculate_info_hash(bencode::be_node * info, uint64_t metafile_len)
{
	if (info == NULL || metafile_len == 0)
		return ERR_BAD_ARG;
	memset(m_info_hash_bin,0,20);
	memset(m_info_hash_hex,0,41);
	char * bencoded_info = new char[metafile_len];
	uint32_t bencoded_info_len = 0;
	if (bencode::encode(info, &bencoded_info, metafile_len, &bencoded_info_len) != ERR_NO_ERROR)
	{
		delete[] bencoded_info;
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL;
	}
	bencode::dump(info);
	CSHA1 csha1;
	csha1.Update((unsigned char*)bencoded_info,bencoded_info_len);
	csha1.Final();
	csha1.ReportHash(m_info_hash_hex,CSHA1::REPORT_HEX);
	csha1.GetHash(m_info_hash_bin);
	delete[] bencoded_info;
	return ERR_NO_ERROR;
}

int save_meta2file(const std::string & filepath)
{
	return save_meta2file(filepath.c_str());
}

int Metafile::save_meta2file(const char * filepath)
{
	if (filepath == NULL)
	{
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_BAD_ARG;
	}

	if (m_metafile == NULL)
	{
		m_error = GENERAL_ERROR_UNDEF_ERROR;
		return ERR_NULL_REF;
	}

	char * bencoded_metafile = new(std::nothrow) char[m_metafile_len];
	if (bencoded_metafile == NULL)
	{
		m_error = GENERAL_ERROR_NO_MEMORY_AVAILABLE;
		return ERR_INTERNAL;
	}
	uint32_t bencoded_metafile_len = 0;
	if (bencode::encode(m_metafile, &bencoded_metafile, m_metafile_len, &bencoded_metafile_len) != ERR_NO_ERROR)
	{
		delete[] bencoded_metafile;
		m_error = TORRENT_ERROR_INVALID_METAFILE;
		return ERR_INTERNAL;
	}
	int fd = open(filepath, O_RDWR | O_CREAT, S_IRWXU);
	if (fd == -1)
	{
		delete[] bencoded_metafile;
		m_error = TORRENT_ERROR_IO_ERROR;
		return ERR_SYSCALL_ERROR;
	}
	ssize_t ret = write(fd, bencoded_metafile, bencoded_metafile_len);
	if (ret == -1)
	{
		delete[] bencoded_metafile;
		m_error = TORRENT_ERROR_IO_ERROR;
		return ERR_SYSCALL_ERROR;
		close(fd);
		return ERR_SYSCALL_ERROR;
	}
	close(fd);
	delete[] bencoded_metafile;
	return ERR_NO_ERROR;
}


} /* namespace torrent */
