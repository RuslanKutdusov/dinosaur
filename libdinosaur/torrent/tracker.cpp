/*
 * tracker.cpp
 *
 *  Created on: 28.02.2012
 *      Author: ruslan
 */

#include "torrent.h"

namespace dinosaur {
namespace torrent {

Tracker::Tracker()
	:network::SocketAssociation()
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Tracker default constructor";
#endif
	memset(m_buf, 0, 16384);
	m_buflen = 0;
	m_peers = NULL;
	m_peers_count = 0;
	m_state = TRACKER_STATE_NONE;
	m_ready2release = false;
	m_addr = NULL;
	m_tracker_failure = "";
}

int Tracker::parse_announce()
{
	//из анонса выдираем доменное имя хоста и параметры
	//регвыр
	char pattern[] = "^(http):\\/\\/(.+?)(\\/.*$)";
	pcre *re;
	const char * error;
	int erroffset;
	//компилим регвыр
	re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
	if (!re)
		return ERR_INTERNAL;

	int ovector[1024];
	int count = pcre_exec(re, NULL, m_announce.c_str(), m_announce.length(), 0, 0, ovector, 1024);
	if (count <= 0)
	{
		pcre_free(re);
		return ERR_INTERNAL;
	}
	pcre_free(re);
	m_host = m_announce.substr(ovector[4], ovector[5] - ovector[4]);
	m_params = m_announce.substr(ovector[6], ovector[7] - ovector[6]);
	return ERR_NO_ERROR;
}

Tracker::Tracker(const TorrentInterfaceInternalPtr & torrent, std::string & announce)
	:network::SocketAssociation()
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Tracker constructor " << announce.c_str();
#endif
	m_torrent = torrent;
	m_nm = m_torrent->get_nm();
	m_g_cfg = m_torrent->get_cfg();
	m_announce = announce;
	memset(m_buf, 0, 16384);
	m_buflen = 0;
	m_interval = m_g_cfg->get_tracker_default_interval();
	m_min_interval = 0;
	m_seeders = 0;
	m_leechers = 0;
	m_peers = NULL;
	m_addr = NULL;
	m_downloaded = torrent->get_downloaded();
	m_uploaded = torrent->get_uploaded();
	m_peers_count = 0;
	m_ready2release = false;
	m_tracker_failure = "";
	m_state = TRACKER_STATE_NONE;
	hash2urlencode();
	if (parse_announce() != ERR_NO_ERROR)
	{
		m_state = TRACKER_STATE_FAILURE;
		m_status = TRACKER_STATUS_INVALID_ANNOUNCE;
	}
}

void Tracker::hash2urlencode()
{
	memset(m_infohash, 0, SHA1_LENGTH * 3 + 1);
	for(int i = 0; i < SHA1_LENGTH; i++)
	{
		m_infohash[i * 3] = '%';
		sprintf(&m_infohash[i * 3 + 1], "%02x", m_torrent->get_infohash()[i]);
	}
}

/*
 * Exception::ERR_CODE_NULL_REF
 * Exception::ERR_CODE_SOCKET_CLOSED
 * Exception::ERR_CODE_SOCKET_CAN_NOT_SEND
 */

void Tracker::send_request(TRACKER_EVENT event )
{
	m_status = TRACKER_STATUS_UPDATING;
	std::string urn(m_params);
	if (urn.find('?') < urn.length())
		urn.append("&");
	else
		urn.append("?");
	//if (urn[urn.length() - 1] != '&')
	//	urn.append("&");

	urn.append("info_hash=");
	urn.append(m_infohash);

	urn.append("&peer_id=");
	char peer_id[PEER_ID_LENGTH + 1];
	memset(peer_id, 0, PEER_ID_LENGTH + 1);
	m_g_cfg->get_peer_id(peer_id);
	urn.append(peer_id);

	char t[256];
	urn.append("&port=");
	sprintf(t, "%d", m_g_cfg->get_port());
	urn.append(t);

	if (event != TRACKER_EVENT_STARTED)
	{
		urn.append("&downloaded=");
		sprintf(t, "%lld", m_torrent->get_downloaded() - m_downloaded);
		urn.append(t);

		urn.append("&uploaded=");
		sprintf(t, "%lld", m_torrent->get_uploaded() - m_uploaded);
		urn.append(t);
	}
	else
	{
		urn.append("&downloaded=0&uploaded=0");
	}

	urn.append("&left=");
	sprintf(t, "%lld", m_torrent->get_length() - m_torrent->get_downloaded());
	urn.append(t);

	urn.append("&numwant=");
	sprintf(t, "%d", m_g_cfg->get_tracker_numwant());
	urn.append(t);

	urn.append("&compact=1");
	urn.append("&supportcrypto=0");

	const char * TRACKER_EVENT_STR[4] = {"&event=started", "&event=stopped", "&event=completed",""};
	urn.append(TRACKER_EVENT_STR[event]);
	std::string http = "GET ";
	http.append(urn);
	http.append(" HTTP/1.0\r\nUser-Agent: DinosaurBT\r\nHost: ");
	http.append(m_host);
	http.append("\r\n");
	http.append("Connection: close\r\n\r\n");
	m_nm->Socket_send(m_sock, http.c_str(), http.length(), true);
}

int Tracker::process_response()
{
	char * rnrn = strchr(m_buf,'\r');
	char * data = NULL;
	ssize_t len = 0;
	while (rnrn != NULL)
	{
		if (strncmp(rnrn, "\r\n\r\n", 4) == 0)
		{
			data = rnrn + 4;
			break;
		}
		rnrn = strchr(rnrn + 4,'\r');
	}
	if (data == NULL)
	{
		m_status = TRACKER_STATUS_ERROR;
		logger::LOGGER() << "Tracker invalid response(data == null)";
		return ERR_INTERNAL;
	}
	len = m_buflen - (data - m_buf);


	bencode::be_node * response = bencode::decode(data, len, false);
	if (response == NULL)
	{
		m_status = TRACKER_STATUS_ERROR;
		logger::LOGGER() << "Tracker invalid response(not bencode)";
		return ERR_INTERNAL;
	}
	//bencode::dump(response);
	bencode::be_str * b_str = NULL;
	char * c_str = NULL;
	m_status = TRACKER_STATUS_OK;
	m_tracker_failure = "";
	if (bencode::get_str(response,"warning message",&b_str) == 0)
	{
		c_str = bencode::str2c_str(b_str);
		m_status = TRACKER_STATUS_SEE_FAILURE_MESSAGE;
		m_tracker_failure = c_str;
		delete[] c_str;
	}
	if (bencode::get_str(response,"failure reason",&b_str) == 0)
	{
		c_str = bencode::str2c_str(b_str);
		m_status = TRACKER_STATUS_SEE_FAILURE_MESSAGE;
		m_tracker_failure = c_str;
		delete[] c_str;
	}
	bencode::get_int(response,"interval",&m_interval);
	bencode::get_int(response,"min interval",&m_min_interval);
	if (bencode::get_str(response,"tracker id",&b_str) == 0)
	{
		c_str = bencode::str2c_str(b_str);
		m_tracker_id = c_str;
		delete[] c_str;
	}
	bencode::get_int(response,"complete",&m_seeders);
	bencode::get_int(response,"incomplete",&m_leechers);
	if (bencode::get_str(response,"peers",&b_str) == 0)
	{
		int len = b_str->len;
		char * t_str = b_str->ptr;
		if (len % 6 != 0)
		{
			bencode::_free(response);
			m_status = TRACKER_STATUS_ERROR;
			logger::LOGGER() << "Tracker invalid response(invalid peer list)";
			return ERR_INTERNAL;
		}
		m_peers_count = len / 6;
		if (m_peers != NULL)
			delete[] m_peers;
		m_peers = new sockaddr_in[m_peers_count];
		for(int i = 0; i < m_peers_count; i++)
		{
			memcpy((void*)&m_peers[i].sin_addr, (void*)&t_str[i * 6], 4);
			memcpy(&m_peers[i].sin_port, &t_str[i * 6 + 4], 2);
			m_peers[i].sin_family = AF_INET;
		}
	}

	bencode::_free(response);
	m_buflen = 0;
	if (m_peers_count > 0)
	{
		m_torrent->add_seeders(m_peers_count, m_peers);
	}
	delete_socket();
	return 0;
}

void Tracker::event_sock_ready2read(network::Socket sock)
{
	//std::cout<<"TRACKER event_sock_ready2read "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	if (m_state == TRACKER_STATE_STOPPING || m_state == TRACKER_STATE_FAILURE)
		return;
	try
	{
		bool closed;
		size_t received = m_nm->Socket_recv(sock, m_buf + m_buflen, BUFFER_SIZE - m_buflen, closed);
		m_buflen += received;
		process_response();
		if (closed)
		{
			delete_socket();
			if (m_status == TRACKER_STATUS_UPDATING)
				m_status = TRACKER_STATUS_ERROR;
			m_ready2release = (m_state == TRACKER_STATE_STOPPING);
			return;
		}
	}
	catch (Exception & e)
	{
		delete_socket();
		m_status = TRACKER_STATUS_ERROR;
		logger::LOGGER() << "Tracker ready to read event failure";
	}
}

void Tracker::event_sock_error(network::Socket sock, int errno_)
{
	//std::cout<<"TRACKER event_sock_closed "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	if (m_state == TRACKER_STATE_FAILURE)
		return;

	delete_socket();
	if (m_status == TRACKER_STATUS_UPDATING)
		m_status = TRACKER_STATUS_ERROR;
	m_ready2release = (m_state == TRACKER_STATE_STOPPING);
}

void Tracker::event_sock_sended(network::Socket sock)
{
	//std::cout<<"TRACKER event_sock_sended "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	if (m_state == TRACKER_STATE_FAILURE)
		return;
	if (m_state == TRACKER_STATE_STOPPING && m_nm->Socket_sendbuf_remain(sock) == 0)
	{
		m_ready2release = true;
		m_nm->Socket_delete(m_sock);
#ifdef BITTORRENT_DEBUG
		logger::LOGGER() << "Tracker " << m_announce.c_str() << " stop sended";
#endif
	}
}

void Tracker::event_sock_connected(network::Socket sock)
{
	try
	{
		if (m_state == TRACKER_STATE_FAILURE)
			return;
		if (m_addr == NULL)
			m_addr = new sockaddr_in;
		m_nm->Socket_get_addr(sock, *m_addr);
		send_request(m_event_after_connect);
	}
	catch (Exception & e)
	{
		delete_socket();
		logger::LOGGER() << "Tracker " << m_announce.c_str() << "  error while sending request";
		m_status = TRACKER_STATUS_ERROR;
	}
}

void Tracker::event_sock_accepted(network::Socket sock, network::Socket accepted_sock)
{
}

void Tracker::event_sock_timeout(network::Socket sock)
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Tracker " << m_announce.c_str() << " timeout";
#endif
	try
	{
		if (m_state == TRACKER_STATE_FAILURE)
			return;
		m_status = TRACKER_STATUS_TIMEOUT;
		delete_socket();
		m_ready2release = (m_state == TRACKER_STATE_STOPPING);
	}
	catch (Exception & e)
	{
		delete_socket();
		m_status = TRACKER_STATUS_ERROR;
	}
}

void Tracker::event_sock_unresolved(network::Socket sock)
{
	//std::cout<<"TRACKER event_sock_unresolved "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	if (m_state == TRACKER_STATE_FAILURE)
		return;
	m_status = TRACKER_STATUS_UNRESOLVED;
	delete_socket();
	m_ready2release = (m_state == TRACKER_STATE_STOPPING);
}

int Tracker::get_peers_count()
{
	if (m_state == TRACKER_STATE_FAILURE)
		return 0;
	return m_peers_count;
}

const sockaddr_in * Tracker::get_peer(int i)
{
	if (m_state == TRACKER_STATE_FAILURE)
		return NULL;
	if (i < 0 || i >= m_peers_count)
		return NULL;
	return &m_peers[i];
}

int Tracker::restore_socket()
{
	if (m_sock == NULL)
	{
		m_status = TRACKER_STATUS_CONNECTING;
		m_sock.reset();
		try
		{
			if (m_addr == NULL)
				m_nm->Socket_add_domain(m_host, 80, shared_from_this(), m_sock);
			else
				m_nm->Socket_add((const sockaddr_in)*m_addr, shared_from_this(), m_sock);
		}
		catch (Exception  & e)
		{
			return ERR_INTERNAL;
		}
		catch (SyscallException & e)
		{
			return ERR_INTERNAL;
		}
	}
	return ERR_NO_ERROR;
}

int Tracker::clock(bool & release_me)
{
	release_me = false;
	if (m_state == TRACKER_STATE_FAILURE)
		release_me = true;
	if (m_state == TRACKER_STATE_WORK && (uint32_t)(time(NULL) - m_last_update) >= m_interval)
	{
		update();
	}
	if (m_state == TRACKER_STATE_STOPPING)
		release_me = m_ready2release;
	return ERR_NO_ERROR;
}

int Tracker::update()
{
	if (m_state == TRACKER_STATE_FAILURE || m_state == TRACKER_STATE_STOPPING)
		return ERR_NO_ERROR;
	if (restore_socket() != ERR_NO_ERROR)
	{
		m_status = TRACKER_STATUS_ERROR;
		logger::LOGGER() << "Tracker " << m_announce.c_str() << "  can not restore socket";
	}

	m_event_after_connect = TRACKER_EVENT_NONE;
	m_state = TRACKER_STATE_WORK;
	m_last_update = time(NULL);
	return ERR_NO_ERROR;
}

int Tracker::prepare2release()
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Tracker " << m_announce.c_str() << " prepare2release";
#endif
	if (m_state == TRACKER_STATE_FAILURE)
	{
		#ifdef BITTORRENT_DEBUG
			logger::LOGGER() << "Tracker is fail " << m_announce.c_str() << " prepare2release";
		#endif
		return ERR_INTERNAL;
	}
	if (send_stopped() != ERR_NO_ERROR)
	{
		delete_socket();
		return ERR_INTERNAL;
	}
	m_state = TRACKER_STATE_STOPPING;
	return ERR_NO_ERROR;
}

void Tracker::forced_releasing()
{
	if (m_state == TRACKER_STATE_FAILURE)
		return;
	delete_socket();
}

int Tracker::send_completed()
{
	if (m_state == TRACKER_STATE_FAILURE || m_state == TRACKER_STATE_STOPPING)
		return ERR_NO_ERROR;
	if (restore_socket() != ERR_NO_ERROR)
	{
		m_status = TRACKER_STATUS_ERROR;
		logger::LOGGER() << "Tracker " << m_announce.c_str() << "  can not restore socket";
	}
	m_event_after_connect = TRACKER_EVENT_COMPLETED;
	m_state = TRACKER_STATE_WORK;
	m_last_update = time(NULL);
	return ERR_NO_ERROR;
}

int Tracker::send_stopped()
{
	if (m_state == TRACKER_STATE_FAILURE || m_state == TRACKER_STATE_STOPPING)
		return ERR_NO_ERROR;
	if (restore_socket() != ERR_NO_ERROR)
	{
		m_status = TRACKER_STATUS_ERROR;
		logger::LOGGER() << "Tracker " << m_announce.c_str() << "  can not restore socket";
	}
	m_event_after_connect = TRACKER_EVENT_STOPPED;
	m_state = TRACKER_STATE_WORK;
	m_last_update = time(NULL);
	return ERR_NO_ERROR;
}

int Tracker::send_started()
{
	if (m_state == TRACKER_STATE_FAILURE || m_state == TRACKER_STATE_STOPPING)
		return ERR_NO_ERROR;
	if (restore_socket() != ERR_NO_ERROR)
	{
		m_status = TRACKER_STATUS_ERROR;
		logger::LOGGER() << "Tracker " << m_announce.c_str() << "  can not restore socket";
	}
	m_event_after_connect = TRACKER_EVENT_STARTED;
	m_state = TRACKER_STATE_WORK;
	m_last_update = time(NULL);
	return ERR_NO_ERROR;
}

int Tracker::get_info(info::tracker & ref)
{
	ref.announce = m_announce;
	ref.leechers = m_leechers;
	ref.seeders = m_seeders == 0 ? m_peers_count : m_seeders;
	ref.status = m_status;
	if (m_state == TRACKER_STATE_WORK)
		ref.update_in = (time_t)m_interval - time(NULL) + m_last_update;
	else
		ref.update_in = 0;
	ref.failure_mes = m_tracker_failure;
	return ERR_NO_ERROR;
}

Tracker::~Tracker()
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Tracker " << m_announce.c_str() << " destructor";
#endif
	if(m_peers != NULL)
		delete[] m_peers;
	if (m_addr != NULL)
		delete m_addr;
	delete_socket();
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "Tracker " << m_announce.c_str() << " destroyed";
#endif
}

}
}
