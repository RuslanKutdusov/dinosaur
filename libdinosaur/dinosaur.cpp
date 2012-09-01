/*
 * Dinosaur.cpp
 *
 *  Created on: 09.04.2012
 *      Author: ruslan
 */

#include "dinosaur.h"

namespace dinosaur {

void catchCrash(int signum)
{
	void *callstack[128];
	std::string fname;
	FILE * f;
    int frames = backtrace(callstack, 128);
    char **strs=backtrace_symbols(callstack, frames);

    passwd *pw = getpwuid(getuid());
	if (pw == NULL)
		goto end;
	fname = pw->pw_dir;
	if (fname.length() <= 0)
		goto end;
	if (fname[fname.length() - 1] != '/')
		fname.append("/");

    fname += "libdinosaur_crash_report_";
    fname += boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time());
    fname += ".txt";
    f = fopen(fname.c_str(), "w");
    if (f)
    {
        for(int i = 0; i < frames; ++i)
        {
            fprintf(f, "%s\n", strs[i]);
        }
        fclose(f);
    }

end:
    free(strs);
    signal(signum, SIG_DFL);
	exit(3);
}

/*
 * Exception::ERR_CODE_CAN_NOT_CREATE_THREAD
 * SyscallException
 */

Dinosaur::Dinosaur()
 {
	m_thread = 0;

	signal(SIGSEGV, catchCrash);
	signal(SIGABRT, catchCrash);

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
	std::string log_dir = m_directory;
	log_dir += "logs/";
	mkdir(log_dir.c_str(), S_IRWXU);
	logger::InitLogger(log_dir);

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

	m_thread_stop = false;
	if (pthread_create(&m_thread, &attr, Dinosaur::thread, (void *)this))
		throw Exception(Exception::ERR_CODE_CAN_NOT_CREATE_THREAD);
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
	logger::LOGGER() << "Dinosaur destructor";
#endif
	pthread_mutex_lock(&m_mutex);
	m_serializable.clear();
	std::string state_file = m_directory;
	state_file.append("our_torrents");
	std::ofstream ofs(state_file.c_str());
	if (!ofs.fail())
	{
		for(string_list::iterator iter = m_torrent_queue.active.begin(); iter != m_torrent_queue.active.end(); ++iter)
		{
			torrent_info ti;
			ti.metafile_path = m_directory;
			ti.metafile_path += *iter;
			ti.metafile_path += ".torrent";
			ti.in_queue = false;
			ti.paused_by_user = m_torrents[*iter].paused_by_user;
			m_serializable.push_back(ti);
		}
		for(string_list::iterator iter = m_torrent_queue.in_queue.begin(); iter != m_torrent_queue.in_queue.end(); ++iter)
		{
			torrent_info ti;
			ti.metafile_path = m_directory;
			ti.metafile_path += *iter;
			ti.metafile_path += ".torrent";
			ti.in_queue = true;
			ti.paused_by_user = m_torrents[*iter].paused_by_user;
			m_serializable.push_back(ti);
		}

		try
		{
			boost::archive::text_oarchive oa(ofs);
			oa << *this;
		}
		catch(...)
		{

		}
	}
	ofs.close();

	for(torrent_map_iter iter = m_torrents.begin(); iter != m_torrents.end(); ++iter)
	{
		(*iter).second.ptr->prepare2release();
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
	logger::LOGGER() << "Dinosaur destroyed";
#endif
}

void Dinosaur::load_our_torrents()
{
	std::string state_file = m_directory;
	state_file.append("our_torrents");
	std::ifstream ifs(state_file.c_str());
	if (ifs.fail())
		return;
	m_serializable.clear();
	try
	{
		boost::archive::text_iarchive ia(ifs);
		ia >> *this;
	}
	catch(...)
	{

	}
	ifs.close();
	pthread_mutex_lock(&m_mutex);
	for(std::list<torrent_info>::iterator iter = m_serializable.begin(); iter != m_serializable.end(); ++iter)
	{
		std::string hash;
		std::string name;
		try
		{
			name = iter->metafile_path;
			torrent::Metafile metafile(iter->metafile_path);
			name = metafile.name;
			hash = metafile.info_hash_hex;
			torrent::TorrentInterfaceBasePtr torrent;

			if (m_torrents.count(hash) !=0)
				throw Exception(Exception::ERR_CODE_TORRENT_EXISTS);

			torrent::TorrentInterfaceBase::CreateTorrent(&m_nm, &Config, &m_fm, &m_bc, metafile, m_directory, Config.get_download_directory(), torrent);

			m_torrents[hash].ptr = torrent;
			m_torrents[hash].paused_by_user = iter->paused_by_user;
			if (iter->in_queue)
				m_torrent_queue.in_queue.push_back(hash);
			else
				m_torrent_queue.active.push_back(hash);
		}
		catch (Exception & e) {
			torrent_failure tf;
			tf.exception_errcode = e.get_errcode();
			tf.errno_ = 0;
			tf.where = TORRENT_FAILURE_INIT_TORRENT;
			tf.description = name + " " + hash;
			m_fails_torrents.push_back(tf);
			continue;
		}
		catch(SyscallException &e)
		{
			torrent_failure tf;
			tf.exception_errcode = Exception::NO_ERROR;
			tf.errno_ = e.get_errno();
			tf.where = TORRENT_FAILURE_INIT_TORRENT;
			tf.description = name + " " + hash;
			m_fails_torrents.push_back(tf);
			continue;
		}
	}
	pthread_mutex_unlock(&m_mutex);
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
	torrent::TorrentInterfaceBasePtr torrent;

	hash = metafile.info_hash_hex;

	pthread_mutex_lock(&m_mutex);
	if (m_torrents.count(hash) !=0)
	{
		pthread_mutex_unlock(&m_mutex);
		throw Exception(Exception::ERR_CODE_TORRENT_EXISTS);
	}
	try
	{
		torrent::TorrentInterfaceBase::CreateTorrent(&m_nm, &Config, &m_fm, &m_bc, metafile, m_directory, download_directory, torrent);
	}
	catch(Exception & e)
	{
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
	m_torrents[hash].ptr = torrent;
	m_torrents[hash].paused_by_user = false;
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_UNDEF
 * Exception::ERR_CODE_TORRENT_EXISTS
 */

void Dinosaur::AddTorrent(torrent::Metafile & metafile, const std::string & download_directory, std::string & hash)
{
	hash = metafile.info_hash_hex;
	if (m_torrents.count(hash) !=0)
		throw Exception(Exception::ERR_CODE_TORRENT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		torrent::TorrentInterfaceBasePtr torrent;
		torrent::TorrentInterfaceBase::CreateTorrent(&m_nm, &Config, &m_fm, &m_bc, metafile, m_directory, download_directory, torrent);
		m_torrents[hash].ptr = torrent;
		m_torrents[hash].paused_by_user = false;
		m_torrent_queue.in_queue.push_back(hash);
		std::string fname =  m_directory + metafile.info_hash_hex + ".torrent";
		metafile.save2file(fname);
	}
	catch(Exception & e)
	{
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
	pthread_mutex_unlock(&m_mutex);
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
		m_torrents[hash].ptr->pause();
		m_torrents[hash].paused_by_user = true;
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
		m_torrents[hash].ptr->continue_();
		m_torrents[hash].paused_by_user = false;
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
		m_torrents[hash].ptr->check();
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

void Dinosaur::DeleteTorrent(const std::string & hash, bool with_data)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	torrent::TorrentInterfaceBasePtr torrent = m_torrents[hash].ptr;
	torrent->prepare2release();
	torrent->erase_state();

	std::string current_fname = m_directory + hash + ".torrent";
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
	m_torrents[hash].ptr->get_info_stat(ref);
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
	m_torrents[hash].ptr->get_info_dyn(ref);
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
	m_torrents[hash].ptr->get_info_trackers(ref);
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 * Exception::ERR_CODE_INVALID_FILE_INDEX
 */

void Dinosaur::get_torrent_info_file_stat(const std::string & hash, FILE_INDEX index, info::file_stat & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		m_torrents[hash].ptr->get_info_file_stat(index, ref);
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
		m_torrents[hash].ptr->get_info_file_dyn(index, ref);
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
	m_torrents[hash].ptr->get_info_seeders(ref);
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
	m_torrents[hash].ptr->get_info_leechers(ref);
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
	m_torrents[hash].ptr->get_info_downloadable_pieces(ref);
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 */

void Dinosaur::get_torrent_failure_desc(const std::string & hash, torrent_failure & ref)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	ref = m_torrents[hash].ptr->get_failure_desc();
	pthread_mutex_unlock(&m_mutex);
}

/*
 * Exception::ERR_CODE_TORRENT_NOT_EXISTS
 * Exception::ERR_CODE_INVALID_FILE_INDEX
 * Exception::ERR_CODE_FAIL_SET_FILE_PRIORITY
 * Exception::ERR_CODE_INVALID_OPERATION
 */

void Dinosaur::set_file_priority(const std::string & hash, FILE_INDEX file, DOWNLOAD_PRIORITY prio)
{
	if (m_torrents.count(hash) == 0)
		throw Exception(Exception::ERR_CODE_TORRENT_NOT_EXISTS);
	pthread_mutex_lock(&m_mutex);
	try
	{
		m_torrents[hash].ptr->set_file_priority(file, prio);
		pthread_mutex_unlock(&m_mutex);
	}
	catch (Exception & e) {
		pthread_mutex_unlock(&m_mutex);
		throw Exception(e);
	}
}


void Dinosaur::get_TorrentList(std::list<std::string> & ref)
{
	ref.clear();
	pthread_mutex_lock(&m_mutex);
	ref = m_torrent_queue.active;
	for(string_list::iterator iter = m_torrent_queue.in_queue.begin(); iter != m_torrent_queue.in_queue.end(); ++iter)
		ref.push_back(*iter);
	pthread_mutex_unlock(&m_mutex);
}

void Dinosaur::get_active_torrents(std::list<std::string>  & ref)
{
	ref.clear();
	pthread_mutex_lock(&m_mutex);
	ref = m_torrent_queue.active;
	pthread_mutex_unlock(&m_mutex);
}

void Dinosaur::get_torrents_in_queue(std::list<std::string>  & ref)
{
	ref.clear();
	pthread_mutex_lock(&m_mutex);
	ref = m_torrent_queue.in_queue;
	pthread_mutex_unlock(&m_mutex);
}

/*
* Exception::ERR_CODE_CAN_NOT_SAVE_CONFIG
*/

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
#ifdef BITTORRENT_DEBUG
	sockaddr_in addr;
	m_nm.Socket_get_addr(sock, &addr);
	std::string ip = inet_ntoa(addr.sin_addr);
#endif
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
			#ifdef BITTORRENT_DEBUG
				logger::LOGGER() << "Leecher " << ip << " sended not handshake";
			#endif
			m_nm.Socket_delete(sock);
			return ERR_INTERNAL;
		}
		if (memcmp(&handshake[1], "BitTorrent protocol", 19) != 0)
		{
			#ifdef BITTORRENT_DEBUG
				logger::LOGGER() << "Leecher " << ip << " works with unsupported Bittorrent protocol";
			#endif
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
			#ifdef BITTORRENT_DEBUG
				logger::LOGGER() << "Leecher " << ip << " wants to work with missed torrent";
			#endif
			m_nm.Socket_delete(sock);
			return ERR_INTERNAL;
		}
		(*iter).second.ptr->add_leecher(sock);
		return 0;
	}
	catch (Exception & e) {
			#ifdef BITTORRENT_DEBUG
				logger::LOGGER() << "Leecher " << ip << " rejected";
			#endif
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
#ifdef BITTORRENT_DEBUG
	sockaddr_in addr;
	m_nm.Socket_get_addr(sock, &addr);
	logger::LOGGER() << "Leecher connected " << inet_ntoa(addr.sin_addr);
#endif
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
	Dinosaur * bt = static_cast<Dinosaur*>(arg);
	while(!bt->m_thread_stop)
	{
		try
		{
			bt->m_nm.clock();
		}
		catch(SyscallException & e)
		{
#ifdef BITTORRENT_DEBUG
			logger::LOGGER() << "NetworkManager::clock throws exception: " << e.get_errno_str();
#endif
		}
		pthread_mutex_lock(&bt->m_mutex);
		try
		{
			bt->m_nm.notify();
		}
		catch (Exception & e) {
#ifdef BITTORRENT_DEBUG
			logger::LOGGER() <<  "NetworkManager::notify throws exception: " << exception_errcode2str(e).c_str();
#endif
		}
		bt->m_fm.notify();
		for(string_list::iterator iter = bt->m_torrent_queue.active.begin(), iter2 = iter; iter != bt->m_torrent_queue.active.end(); iter = iter2)
		{
			++iter2;
			bt->m_torrents[(*iter)].ptr->clock();
			if (bt->m_torrents[(*iter)].ptr->i_am_releasing() && bt->m_torrents[(*iter)].ptr.use_count() == 1)
			{
				bt->m_torrents.erase(*iter);
				bt->m_torrent_queue.active.erase(iter);
				continue;
			}
			info::torrent_dyn dyn;
			bt->m_torrents[(*iter)].ptr->get_info_dyn(dyn);
			if (dyn.work == TORRENT_WORK_PAUSED || dyn.work == TORRENT_WORK_FAILURE || dyn.work == TORRENT_WORK_DONE)
			{
#ifdef BITTORRENT_DEBUG
				logger::LOGGER() << "Pushing " << *iter << " in to queue\n";
#endif
				bt->m_torrent_queue.in_queue.push_back(*iter);
				bt->m_torrent_queue.active.erase(iter);
			}
		}

		for(string_list::iterator iter = bt->m_torrent_queue.in_queue.begin(), iter2 = iter; iter != bt->m_torrent_queue.in_queue.end(); iter = iter2)
		{
			++iter2;

			info::torrent_dyn dyn;
			bt->m_torrents[(*iter)].ptr->get_info_dyn(dyn);
			if (dyn.work == TORRENT_WORK_CHECKING || dyn.work == TORRENT_WORK_RELEASING)
			{
				bt->m_torrents[(*iter)].ptr->clock();
				if (bt->m_torrents[(*iter)].ptr->i_am_releasing() && bt->m_torrents[(*iter)].ptr.use_count() == 1)
				{
					bt->m_torrents.erase(*iter);
					bt->m_torrent_queue.in_queue.erase(iter);
				}
				continue;
			}
			if (bt->m_torrent_queue.active.size() >= bt->Config.get_max_active_torrents())
				continue;
			if (dyn.work == TORRENT_WORK_FAILURE)
				continue;
			if (bt->m_torrents[(*iter)].paused_by_user)
				continue;
			if (dyn.ratio > bt->Config.get_fin_ratio() && dyn.work == TORRENT_WORK_DONE)
				continue;
			if (dyn.work == TORRENT_WORK_PAUSED)
				bt->m_torrents[(*iter)].ptr->continue_();
#ifdef BITTORRENT_DEBUG
		logger::LOGGER() << "Pushing " << *iter << " in to active\n";
#endif
			bt->m_torrent_queue.active.push_back(*iter);
			bt->m_torrent_queue.in_queue.erase(iter);
		}

		pthread_mutex_unlock(&bt->m_mutex);
		usleep(1000);
	}
	return (void*)ret;
}

template<class Archive>
void Dinosaur::serialize(Archive & ar, const unsigned int version)
{
	ar & m_serializable;
}

} /* namespace Dinosaur */
