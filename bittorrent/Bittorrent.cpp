/*
 * Bittorrent.cpp
 *
 *  Created on: 09.04.2012
 *      Author: ruslan
 */

#include "Bittorrent.h"

namespace bittorrent {

Bittorrent::Bittorrent()
 {
	m_thread = 0;
	//создаем рабочую папку в пользовательской папке
	passwd *pw = getpwuid(getuid());
	if (pw == NULL)
		throw Exception();
	m_directory = pw->pw_dir;
	if (m_directory.length() <= 0)
		throw Exception();
	if (m_directory[m_directory.length() - 1] != '/')
		m_directory.append("/");
	m_directory.append(".dinosaur/");
	mkdir(m_directory.c_str(), S_IRWXU);

	m_error = TORRENT_ERROR_NO_ERROR;
	//инициализация всего и вся
	m_gcfg = cfg::Glob_cfg(m_directory);
	if (m_nm.Init() != ERR_NO_ERROR)
		throw Exception();
	if (m_fm.Init(&m_gcfg) != ERR_NO_ERROR)
		throw Exception();
	if (m_bc.Init(m_gcfg.get_cache_size()) != ERR_NO_ERROR)
		throw Exception();

	m_sock = m_nm.ListenSocket_add(m_gcfg.get_port(), this);
	if (m_sock == NULL)
		throw Exception();
	m_thread_stop = false;
	m_thread_pause = false;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&m_mutex, NULL);
	pthread_mutex_unlock(&m_mutex);
	if (pthread_create(&m_thread, &attr, Bittorrent::thread, (void *)this))
		throw Exception();
	pthread_attr_destroy(&attr);
	load_our_torrents();
}

Bittorrent::~Bittorrent() {
	// TODO Auto-generated destructor stub
	printf("Bittorrent destructor\n");
	if (m_thread != 0)
	{
		m_thread_stop = true;
		void * status;
		pthread_join(m_thread, &status);
		pthread_mutex_destroy(&m_mutex);
	}
	for (torrent_map_iter iter = m_torrents.begin(); iter != m_torrents.end(); ++iter)
		delete (*iter).second;
}

int Bittorrent::load_our_torrents()
{
	std::ifstream fin;
	std::string state_file = m_directory;
	state_file.append("our_torrents");
	fin.open(state_file.c_str());
	std::string fname;
	std::string hash;
	while(fin>>fname)
	{
		std::string full_path2torrent_file = m_directory + fname;
		std::cout<<full_path2torrent_file<<std::endl;
		init_torrent(full_path2torrent_file, &hash, false);
		std::string temp = "";
		m_gcfg.get_download_directory(&temp);
		StartTorrent(hash, temp);
	}
	fin.close();
	return ERR_NO_ERROR;
}

void Bittorrent::add_error_mes(std::string & mes)
{
	m_error = mes;
}

void Bittorrent::bin2hex(unsigned char * bin, char * hex, int len)
{
	memset(hex, 0, len * 2 + 1);
	char temp[4];
	memset(temp, 0, 4);
	for(int i = 0; i < len; i++)
	{
		sprintf(temp, "%02x", bin[i]);
		strcat(hex, temp);
	}
}


int Bittorrent::init_torrent(std::string & filename, std::string * hash, bool is_new)
{
	return init_torrent(filename.c_str(), hash, is_new);
}

int Bittorrent::init_torrent(const  char * filename, std::string * hash, bool is_new)
{
	if (hash == NULL)
		return ERR_NULL_REF;
	*hash = "";
	torrent::Torrent * torrent = NULL;
	torrent = new torrent::Torrent();
	std::string err = "";
	if (torrent->Init(filename,&m_nm,&m_gcfg, &m_fm, &m_bc, m_directory, is_new) != ERR_NO_ERROR)
	{
		err = torrent->get_error();
		delete torrent;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	//ключ infohash в hex-e
	//unsigned char infohash[SHA1_LENGTH];
	torrent::torrent_info t_info;
	torrent->get_info(&t_info);
	//torrent->get_info_hash(infohash);
	//char infohash_hex[SHA1_LENGTH * 2 + 1];
	//bin2hex(infohash, infohash_hex, SHA1_LENGTH);
	std::string infohash_str = t_info.info_hash_hex;

	m_thread_pause = true;
	if (m_torrents.count(infohash_str) !=0)
	{
		err = TORRENT_ERROR_EXISTS;
		add_error_mes(err);
		m_thread_pause = false;
		return ERR_SEE_ERROR;
	}
	m_torrents[infohash_str] = torrent;
	m_thread_pause = false;
	*hash = infohash_str;
	return ERR_NO_ERROR;
}

torrent::Torrent * Bittorrent::OpenTorrent(char * filename)
{
	torrent::Torrent * torrent = NULL;
	torrent = new torrent::Torrent();
	std::string err = "";
	if (torrent->Init(filename,&m_nm,&m_gcfg, &m_fm, &m_bc, m_directory, true) != ERR_NO_ERROR)
	{
		err = torrent->get_error();
		delete torrent;
		add_error_mes(err);
		return NULL;
	}
	return torrent;
}

int Bittorrent::AddTorrent(torrent::Torrent * torrent, std::string * hash)
{
	if (torrent == NULL)
	{
		std::string err = GENERAL_ERROR_UNDEF_ERROR;
		add_error_mes(err);
		return ERR_NULL_REF;
	}
	//unsigned char infohash[SHA1_LENGTH];
	//torrent->get_info_hash(infohash);
	//char infohash_hex[SHA1_LENGTH * 2 + 1];
	//bin2hex(infohash, infohash_hex, SHA1_LENGTH);

	torrent::torrent_info t_info;
	torrent->get_info(&t_info);
	std::string infohash_str = t_info.info_hash_hex;

	m_thread_pause = true;
	if (m_torrents.count(infohash_str) !=0)
	{
		std::string err = TORRENT_ERROR_EXISTS;
		add_error_mes(err);
		m_thread_pause = false;
		return ERR_SEE_ERROR;
	}
	std::ofstream fout;
	std::string state_file = m_directory;
	state_file.append("our_torrents");
	fout.open(state_file.c_str(), std::ios_base::app);
	fout<<infohash_str<<".torrent"<<std::endl;
	fout.close();
	std::string fname =  m_directory + infohash_str + ".torrent";
	if (torrent->save_meta2file(fname.c_str()) != ERR_NO_ERROR)
	{
		std::string err = TORRENT_ERROR_EXISTS;
		add_error_mes(err);
		m_thread_pause = false;
		return ERR_SEE_ERROR;
	}


	m_torrents[infohash_str] = torrent;
	m_thread_pause = false;
	*hash = infohash_str;

	return ERR_NO_ERROR;
}

int Bittorrent::StartTorrent(std::string & hash, std::string & download_directory)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	m_thread_pause = true;
	/*struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (6881);
	inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr) ;
	m_torrents[hash]->take_peers(1,&sockaddr);*/
	if (m_torrents[hash]->start(download_directory) != ERR_NO_ERROR)
	{
		std::string err = TORRENT_ERROR_CAN_NOT_START;
		add_error_mes(err);
		m_thread_pause = false;
		return ERR_SEE_ERROR;
	}
	m_thread_pause = false;
	return ERR_NO_ERROR;
}

int Bittorrent::StopTorrent(std::string & hash)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	m_thread_pause = true;
	m_torrents[hash]->stop();
	m_thread_pause = false;
	return ERR_NO_ERROR;
}

int Bittorrent::Torrent_info(std::string & hash, torrent::torrent_info * info)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	m_thread_pause = true;
	m_torrents[hash]->get_info(info);
	m_thread_pause = false;
	return ERR_NO_ERROR;
}

int Bittorrent::get_TorrentList(std::list<std::string> * list)
{
	m_thread_pause = true;
	for (torrent_map_iter iter = m_torrents.begin(); iter != m_torrents.end(); ++iter)
		list->push_back((*iter).first);
	m_thread_pause = false;
	return ERR_NO_ERROR;
}

int Bittorrent::get_DownloadDirectory(std::string * dir)
{
	m_gcfg.get_download_directory(dir);
	return ERR_NO_ERROR;
}

/*uint16_t Bittorrent::Torrent_peers(std::string & hash, torrent::peer_info ** peers)
{
	return ERR_NO_ERROR;
}*/

int Bittorrent::event_sock_ready2read(network::socket_ * sock)
{
	printf("ready\n");
	if (m_nm.Socket_datalen(sock) < HANDSHAKE_LENGHT)
	{
		printf("not enough\n");
		return 0;
	}
	char handshake[HANDSHAKE_LENGHT];
	unsigned char infohash[SHA1_LENGTH];
	m_nm.Socket_recv(sock, handshake, HANDSHAKE_LENGHT);
	if (handshake[0] != '\x13')
	{
		printf("not bittorrent1\n");
		m_nm.Socket_delete(sock);
		return ERR_INTERNAL;
	}
	if (memcmp(&handshake[1], "BitTorrent protocol", 19) != 0)
	{
		printf("not bittorrent2\n");
		m_nm.Socket_delete(sock);
		return ERR_INTERNAL;
	}
	//вытаскиваем infohash из handshake-a, преобразуем в hex и ищем соответствующий торрент
	memcpy(infohash, &handshake[28], SHA1_LENGTH);
	char infohash_hex[SHA1_LENGTH * 2 + 1];
	bin2hex(infohash, infohash_hex, SHA1_LENGTH);
	std::string str_infohash = infohash_hex;
	std::cout<<str_infohash<<std::endl;
	torrent_map_iter iter = m_torrents.find(str_infohash);
	if (iter == m_torrents.end() && m_torrents.count(str_infohash) == 0)
	{

		printf("invalid hash\n");

		m_nm.Socket_delete(sock);
		return ERR_INTERNAL;
	}
	printf("accepted\n");
	m_nm.Socket_set_assoc(sock, (*iter).second);
	if ((*iter).second->add_incoming_peer(sock) != ERR_NO_ERROR)
	{
		printf("can not add peer\n");
		m_nm.Socket_delete(sock);
		return ERR_INTERNAL;
	}
	return 0;
}

int Bittorrent::event_sock_closed(network::socket_ * sock)
{
	return 0;
}

int Bittorrent::event_sock_sended(network::socket_ * sock)
{
	return 0;
}

int Bittorrent::event_sock_connected(network::socket_ * sock)
{
	return 0;
}

int Bittorrent::event_sock_accepted(network::socket_ * sock)
{
	return 0;
}

int Bittorrent::event_sock_timeout(network::socket_ * sock)
{
	return 0;
}

int Bittorrent::event_sock_unresolved(network::socket_ * sock)
{
	return 0;
}

} /* namespace Bittorrent */
