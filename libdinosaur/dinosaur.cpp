/*
 * Dinosaur.cpp
 *
 *  Created on: 09.04.2012
 *      Author: ruslan
 */

#include "dinosaur.h"

namespace dinosaur {

/*
 * SyscallException
 */

Dinosaur::Dinosaur()
 {
	m_thread = 0;
	//создаем рабочую папку в пользовательской папке
	errno = 0;
	passwd *pw = getpwuid(getuid());
	if (pw == NULL)
		throw SyscallException();
	m_directory = pw->pw_dir;
	if (m_directory.length() <= 0)
		throw SyscallException();
	if (m_directory[m_directory.length() - 1] != '/')
		m_directory.append("/");
	m_directory.append(".dinosaur/");
	mkdir(m_directory.c_str(), S_IRWXU);

	//инициализация всего и вся
	Config = cfg::Glob_cfg(m_directory);
	m_nm.Init();
	m_fm.Init(&Config);
	m_bc.Init(Config.get_read_cache_size());

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutexattr_t   mta;

	int ret = pthread_mutexattr_init(&mta);
	if (ret != 0)
		throw SyscallException(ret);
	ret = pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	if (ret != 0)
		throw SyscallException(ret);
	ret = pthread_mutex_init(&m_mutex, &mta);
	if (ret != 0)
		throw SyscallException(ret);

	if (pthread_create(&m_thread, &attr, Dinosaur::thread, (void *)this))
		throw Exception(Exception::ERR_CODE_DINOSAUR_INIT_ERROR);
	pthread_attr_destroy(&attr);

	load_our_torrents();
}

void Dinosaur::init_listen_socket()
{
	in_addr addr;
	Config.get_listen_on(&addr);
	m_sock_status.listen = false;
	m_sock_status.errno_ = 0;
	m_sock_status.exception_errcode = Exception::NO_ERROR;
	try
	{
		m_nm.ListenSocket_add(Config.get_port(), &addr, shared_from_this(), m_sock);
	}
	catch(Exception & e)
	{
		m_sock_status.exception_errcode = e.get_errcode();
		return;
	}
	catch(SyscallException & e)
	{
		m_sock_status.errno_ = e.get_errno();
		return;
	}
	m_sock_status.listen = true;
	return;
}

Dinosaur::~Dinosaur() {
	// TODO Auto-generated destructor stub
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Dinosaur destructor";
#endif
	pthread_mutex_lock(&m_mutex);
	for(torrent_map_iter iter = m_torrents.begin(); iter != m_torrents.end(); ++iter)
	{
;		(*iter).second->prepare2release();
	}
	pthread_mutex_unlock(&m_mutex);
	time_t release_start = time(NULL);
	while(m_torrents.size() != 0)
	{
		usleep(100000);
		if (time(NULL) - release_start > MAX_RELEASE_TIME)
		{
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
	LOG(INFO) << "Dinosaur destroyed";
#endif
}

int Dinosaur::load_our_torrents()
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
		try
		{
			init_torrent(full_path2torrent_file, Config.get_download_directory(), hash);
			StartTorrent(hash);
		}
		catch (Exception & e) {
			torrent_failure tf;
			tf.exception_errcode = e.get_errcode();
			tf.errno_ = 0;
			tf.where = TORRENT_FAILURE_INIT_TORRENT;
			m_fails_torrents.push_back(tf);
		}
	}
	fin.close();
	return ERR_NO_ERROR;
}

void Dinosaur::bin2hex(unsigned char * bin, char * hex, int len)
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

/*
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_TORRENT_EXISTS
 */


void Dinosaur::init_torrent(const torrent::Metafile & metafile, const std::string & download_directory, std::string & hash)
{
	hash = "";
	torrent::TorrentInterfaceBasePtr torrent;

	std::string infohash_str = metafile.info_hash_hex;

	pthread_mutex_lock(&m_mutex);
	if (m_torrents.count(infohash_str) !=0)
	{
		pthread_mutex_unlock(&m_mutex);
		throw Exception(Exception::ERR_CODE_TORRENT_EXISTS);
	}
	torrent::TorrentInterfaceBase::CreateTorrent(&m_nm, &Config, &m_fm, &m_bc, metafile, m_directory, download_directory, torrent);
	m_torrents[infohash_str] = torrent;
	pthread_mutex_unlock(&m_mutex);
	hash = infohash_str;
}

/*
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_TORRENT_EXISTS
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_BENCODE_ENCODE_ERROR
 * Exception::ERR_CODE_STATE_FILE_ERROR
 * SyscallException
 */

void Dinosaur::AddTorrent(torrent::Metafile & metafile, const std::string & download_directory, std::string & hash)
{
	init_torrent(metafile, download_directory, hash);

	std::string fname =  m_directory + metafile.info_hash_hex + ".torrent";
	metafile.save2file(fname);


	std::ofstream fout;
	std::string state_file = m_directory;

	state_file.append("our_torrents");
	fout.open(state_file.c_str(), std::ios_base::app);
	if (fout.fail())
		throw Exception(Exception::ERR_CODE_STATE_FILE_ERROR);
	fout<<metafile.info_hash_hex<<".torrent"<<std::endl;
	fout.close();
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void Dinosaur::StartTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		m_torrents[hash]->start();
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void Dinosaur::StopTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		m_torrents[hash]->stop();
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void Dinosaur::PauseTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		m_torrents[hash]->pause();
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void Dinosaur::ContinueTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		m_torrents[hash]->continue_();
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void Dinosaur::CheckTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		m_torrents[hash]->check();
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void Dinosaur::DeleteTorrent(const std::string & hash)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	torrent::TorrentInterfaceBasePtr torrent = m_torrents[hash];
	torrent->erase_state();
	torrent->prepare2release();

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
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 */

void Dinosaur::get_torrent_info_stat(const std::string & hash, info::torrent_stat & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->get_info_stat(ref);
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 */

void Dinosaur::get_torrent_info_dyn(const std::string & hash, info::torrent_dyn & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->get_info_dyn(ref);
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 */

void Dinosaur::get_torrent_info_trackers(const std::string & hash, info::trackers & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->get_info_trackers(ref);
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 */

void Dinosaur::get_torrent_info_file_stat(const std::string & hash, FILE_INDEX index, info::file_stat & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		m_torrents[hash]->get_info_file_stat(index, ref);
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 * Exception::ERR_CODE_INVALID_FILE_INDEX
 */

void  Dinosaur::get_torrent_info_file_dyn(const std::string & hash, FILE_INDEX index, info::file_dyn & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		m_torrents[hash]->get_info_file_dyn(index, ref);
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 */

void Dinosaur::get_torrent_info_seeders(const std::string & hash, info::peers & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->get_info_seeders(ref);
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 */

void Dinosaur::get_torrent_info_leechers(const std::string & hash, info::peers & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->get_info_leechers(ref);
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 */

void Dinosaur::get_torrent_info_downloadable_pieces(const std::string & hash, info::downloadable_pieces & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	m_torrents[hash]->get_info_downloadable_pieces(ref);
	pthread_mutex_unlock(&m_mutex);
}


void Dinosaur::get_TorrentList(std::list<std::string> & ref)
{
	ref.clear();
	pthread_mutex_lock(&m_mutex);
	for (torrent_map_iter iter = m_torrents.begin(); iter != m_torrents.end(); ++iter)
		ref.push_back((*iter).first);
	pthread_mutex_unlock(&m_mutex);
}

void Dinosaur::UpdateConfigs()
{
	pthread_mutex_lock(&m_mutex);
	try
	{
		Config.save();
		m_nm.Socket_delete(m_sock);
		init_listen_socket();
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
}

int Dinosaur::event_sock_ready2read(network::Socket sock)
{
	try
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
			printf("Not handshake\n");
			m_nm.Socket_delete(sock);
			return ERR_INTERNAL;
		}
		if (memcmp(&handshake[1], "BitTorrent protocol", 19) != 0)
		{
			printf("Not bt\n");
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
			printf("Not torrent\n");
			m_nm.Socket_delete(sock);
			return ERR_INTERNAL;
		}
		(*iter).second->add_leecher(sock);
		return 0;
	}
	catch (Exception & e) {
		printf("Rejected\n");
		m_nm.Socket_delete(sock);
		return ERR_INTERNAL;
	}
}

int Dinosaur::event_sock_closed(network::Socket sock)
{
	if (sock != m_sock)
		m_nm.Socket_delete(sock);
	return 0;
}

int Dinosaur::event_sock_sended(network::Socket sock)
{
	return 0;
}

int Dinosaur::event_sock_connected(network::Socket sock)
{
	return 0;
}

int Dinosaur::event_sock_accepted(network::Socket sock, network::Socket accepted_sock)
{
	m_nm.Socket_set_assoc(accepted_sock, shared_from_this());
	return 0;
}

int Dinosaur::event_sock_timeout(network::Socket sock)
{
	return 0;
}

int Dinosaur::event_sock_unresolved(network::Socket sock)
{
	return 0;
}

void * Dinosaur::thread(void * arg)
{
	int ret = 1;
	Dinosaur * bt = (Dinosaur*)arg;
	while(!bt->m_thread_stop)
	{
		try
		{
			bt->m_nm.clock();
		}
		catch(SyscallException & e)
		{
#ifdef BITTORRENT_DEBUG
			LOG(INFO) << "NetworkManager::clock throws exception: " << e.get_errno_str();
#endif
		}
		pthread_mutex_lock(&bt->m_mutex);
		try
		{
			bt->m_nm.notify();
		}
		catch (Exception & e) {
#ifdef BITTORRENT_DEBUG
			LOG(INFO) <<  "NetworkManager::notify throws exception: " << exception_errcode2str(e).c_str();
#endif
		}
		bt->m_fm.notify();
		for(torrent_map_iter iter = bt->m_torrents.begin(); iter != bt->m_torrents.end(); ++iter)
		{
			(*iter).second->clock();
			if ((*iter).second->i_am_releasing() && (*iter).second.use_count() == 1)
				bt->m_torrents.erase(iter);
		}
		pthread_mutex_unlock(&bt->m_mutex);
		usleep(1000);
	}
	return (void*)ret;
}

} /* namespace Dinosaur */
