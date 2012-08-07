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

	//m_thread_stop = false;
	//pthread_mutex_unlock(&m_mutex);
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutexattr_t   mta;
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_mutex, NULL);
	pthread_mutex_unlock(&m_mutex);
	if (pthread_create(&m_thread, &attr, Bittorrent::thread, (void *)this))
		throw Exception();
	pthread_attr_destroy(&attr);
	load_our_torrents();
}

void Bittorrent::init_listen_socket()
{
	m_nm.ListenSocket_add(m_gcfg.get_port(), shared_from_this(), m_sock);
}

Bittorrent::~Bittorrent() {
	// TODO Auto-generated destructor stub
#ifdef BITTORRENT_DEBUG
	printf("Bittorrent destructor\n");
#endif
	pthread_mutex_lock(&m_mutex);
	for(torrent_map_iter iter = m_torrents.begin(); iter != m_torrents.end(); ++iter)
	{
		(*iter).second->prepare2release();
	}
	pthread_mutex_unlock(&m_mutex);
	time_t release_start = time(NULL);
	while(m_torrents.size() != 0)
	{
		for(torrent_map_iter iter = m_torrents.begin(); iter != m_torrents.end(); ++iter)
		{
			if ((*iter).second.use_count() == 1)
				m_torrents.erase(iter);
		}
		usleep(100000);
		if (time(NULL) - release_start > MAX_RELEASE_TIME)
		{
#ifdef BITTORRENT_DEBUG
			printf("MAX_RELEASE_TIME\n");
#endif
			break;
		}
	}
	if (m_thread != 0)
	{
		m_thread_stop = true;
		void * status;
		pthread_join(m_thread, &status);
		pthread_mutex_destroy(&m_mutex);
	}
	m_torrents.clear();
#ifdef BITTORRENT_DEBUG
	printf("Bittorrent destroyed\n");
#endif
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
		//std::cout<<full_path2torrent_file<<std::endl;
		init_torrent(full_path2torrent_file, m_gcfg.get_download_directory(), hash);
		StartTorrent(hash);
	}
	fin.close();
	return ERR_NO_ERROR;
}

void Bittorrent::add_error_mes(const std::string & mes)
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


int Bittorrent::init_torrent(const torrent::Metafile & metafile, const std::string & download_directory, std::string & hash)
{
	hash = "";
	torrent::TorrentInterfaceBasePtr torrent;
	try
	{
		torrent::TorrentInterfaceBase::CreateTorrent(&m_nm, &m_gcfg, &m_fm, &m_bc, metafile, m_directory, download_directory, torrent);
	}
	catch (Exception & e)
	{
		add_error_mes(e.get_error());
		return ERR_SEE_ERROR;
	}

	std::string infohash_str = metafile.info_hash_hex;

	pthread_mutex_lock(&m_mutex);
	if (m_torrents.count(infohash_str) !=0)
	{
		torrent->erase_state();
		torrent->forced_releasing();
		add_error_mes(TORRENT_ERROR_EXISTS);
		pthread_mutex_unlock(&m_mutex);
		return ERR_SEE_ERROR;
	}
	m_torrents[infohash_str] = torrent;
	pthread_mutex_unlock(&m_mutex);
	hash = infohash_str;
	return ERR_NO_ERROR;
}

int Bittorrent::AddTorrent(torrent::Metafile & metafile, const std::string & download_directory, std::string & hash)
{
	int ret = init_torrent(metafile, download_directory, hash);
	if ( ret != ERR_NO_ERROR)
		return ret;
	std::ofstream fout;
	std::string state_file = m_directory;

	state_file.append("our_torrents");
	fout.open(state_file.c_str(), std::ios_base::app);
	fout<<metafile.info_hash_hex<<".torrent"<<std::endl;
	fout.close();

	std::string fname =  m_directory + metafile.info_hash_hex + ".torrent";
	if (metafile.save2file(fname) != ERR_NO_ERROR)
	{
		std::string err = TORRENT_ERROR_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	return ERR_NO_ERROR;
}

int Bittorrent::StartTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	pthread_mutex_lock(&m_mutex);
	/*struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons (6881);
	inet_pton(AF_INET, "127.0.0.1", &sockaddr.sin_addr) ;
	m_torrents[hash]->take_peers(1,&sockaddr);*/
	if (m_torrents[hash]->start() != ERR_NO_ERROR)
	{
		std::string err = TORRENT_ERROR_CAN_NOT_START;
		add_error_mes(err);
		pthread_mutex_unlock(&m_mutex);
		return ERR_SEE_ERROR;
	}
	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

int Bittorrent::StopTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->stop();
	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

int Bittorrent::PauseTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->pause();
	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

int Bittorrent::ContinueTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->continue_();
	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

int Bittorrent::CheckTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->check();
	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

int Bittorrent::DeleteTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	pthread_mutex_lock(&m_mutex);
	torrent::TorrentInterfaceBasePtr torrent = m_torrents[hash];
	m_torrents.erase(hash);
	torrent->stop();
	torrent->erase_state();
	torrent->forced_releasing();
	torrent.reset();

	std::ifstream fin;
	std::ofstream fout;
	std::string state_file = m_directory;
	state_file.append("our_torrents");

	fin.open(state_file.c_str());
	std::string readed_fname;
	std::string current_fname = hash + ".torrent";
	std::list<std::string> our_torrents;
	while(fin>>readed_fname)
	{
		if (readed_fname != current_fname)
			our_torrents.push_back(readed_fname);
	}
	fin.close();

	fout.open(state_file.c_str());
	while(!our_torrents.empty())
	{
		fout<<our_torrents.front()<<std::endl;
		our_torrents.pop_front();
	}
	fout.close();

	remove(current_fname.c_str());

	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

int Bittorrent::Torrent_info(const std::string & hash, torrent::torrent_info * info)
{
	if (m_torrents.count(hash) == 0)
	{
		std::string err = TORRENT_ERROR_NOT_EXISTS;
		add_error_mes(err);
		return ERR_SEE_ERROR;
	}
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->get_info(info);
	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

int Bittorrent::get_TorrentList(std::list<std::string> * list)
{
	pthread_mutex_lock(&m_mutex);
	for (torrent_map_iter iter = m_torrents.begin(); iter != m_torrents.end(); ++iter)
		list->push_back((*iter).first);
	pthread_mutex_unlock(&m_mutex);
	return ERR_NO_ERROR;
}

const std::string & Bittorrent::get_DownloadDirectory()
{
	return m_gcfg.get_download_directory();
}


int Bittorrent::event_sock_ready2read(network::Socket sock)
{
	if (m_nm.Socket_datalen(sock) < HANDSHAKE_LENGHT)
	{
		return 0;
	}
	char handshake[HANDSHAKE_LENGHT];
	unsigned char infohash[SHA1_LENGTH];
	m_nm.Socket_recv(sock, handshake, HANDSHAKE_LENGHT);
	if (handshake[0] != '\x13')
	{
		m_nm.Socket_delete(sock);
		return ERR_INTERNAL;
	}
	if (memcmp(&handshake[1], "BitTorrent protocol", 19) != 0)
	{
		m_nm.Socket_delete(sock);
		return ERR_INTERNAL;
	}
	//вытаскиваем infohash из handshake-a, преобразуем в hex и ищем соответствующий торрент
	memcpy(infohash, &handshake[28], SHA1_LENGTH);
	char infohash_hex[SHA1_LENGTH * 2 + 1];
	bin2hex(infohash, infohash_hex, SHA1_LENGTH);
	std::string str_infohash = infohash_hex;
	torrent_map_iter iter = m_torrents.find(str_infohash);
	if (iter == m_torrents.end() && m_torrents.count(str_infohash) == 0)
	{
		m_nm.Socket_delete(sock);
		return ERR_INTERNAL;
	}
	if ((*iter).second->add_leecher(sock) != ERR_NO_ERROR)
	{
		m_nm.Socket_delete(sock);
		return ERR_INTERNAL;
	}
	return 0;
}

int Bittorrent::event_sock_closed(network::Socket sock)
{
#ifdef BITTORRENT_DEBUG
	printf("LISTEN_SOCKET closed\n");
#endif
	if (sock != m_sock)
		m_nm.Socket_delete(sock);
	return 0;
}

int Bittorrent::event_sock_sended(network::Socket sock)
{
	return 0;
}

int Bittorrent::event_sock_connected(network::Socket sock)
{
	return 0;
}

int Bittorrent::event_sock_accepted(network::Socket sock, network::Socket accepted_sock)
{
#ifdef BITTORRENT_DEBUG
	printf("LISTEN_SOCKET accepted\n");
#endif
	m_nm.Socket_set_assoc(accepted_sock, shared_from_this());
	return 0;
}

int Bittorrent::event_sock_timeout(network::Socket sock)
{
	return 0;
}

int Bittorrent::event_sock_unresolved(network::Socket sock)
{
	return 0;
}

void * Bittorrent::thread(void * arg)
{
	int ret = 1;
	Bittorrent * bt = (Bittorrent*)arg;
	while(!bt->m_thread_stop)
	{
		bt->m_nm.clock();
		pthread_mutex_lock(&bt->m_mutex);
		bt->m_nm.notify();
		bt->m_fm.notify();
		for(torrent_map_iter iter = bt->m_torrents.begin(); iter != bt->m_torrents.end(); ++iter)
		{
			(*iter).second->clock();
		}
		pthread_mutex_unlock(&bt->m_mutex);
		usleep(1000);
	}
	return (void*)ret;
}

} /* namespace Bittorrent */
