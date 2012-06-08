/*
 * tracker.cpp
 *
 *  Created on: 28.02.2012
 *      Author: ruslan
 */

#include "torrent.h"

namespace torrent {

Tracker::Tracker()
	:network::sock_event()
{
	memset(m_buf, 0, 16384);
	m_buflen = 0;
	m_peers = NULL;
	m_peers_count = 0;
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

Tracker::Tracker(Torrent * torrent, std::string & announce)
	:network::sock_event()
{
	if (torrent == NULL || announce == "")
		throw NoneCriticalException("Bad args");
	m_torrent = torrent;
	m_nm = m_torrent->m_nm;
	m_g_cfg = m_torrent->m_g_cfg;
	m_announce = announce;
	memset(m_buf, 0, 16384);
	m_buflen = 0;
	m_interval = m_g_cfg->get_tracker_default_interval();
	m_last_update = time(NULL);
	m_min_interval = 0;
	m_seeders = 0;
	m_leechers = 0;
	m_peers = NULL;
	m_downloaded = torrent->m_downloaded;
	m_uploaded = torrent->m_uploaded;
	m_peers_count = 0;
	m_sock = NULL;
	if (parse_announce() != ERR_NO_ERROR)
		throw NoneCriticalException("Invalid announce");
	restore_socket() ;
	m_event_after_connect = TRACKER_EVENT_STARTED;
	hash2urlencode();
}


network::socket_ * Tracker::get_sock()
{
	return m_sock;
}

void Tracker::hash2urlencode()
{
	memset(m_infohash, 0, SHA1_LENGTH * 3);
	for(int i = 0; i < SHA1_LENGTH; i++)
	{
		m_infohash[i * 3] = '%';
		sprintf(&m_infohash[i * 3 + 1], "%02x", m_torrent->m_info_hash_bin[i]);
	}
}

int Tracker::send_request(TRACKER_EVENT event )
{
	m_status = TRACKER_STATUS_UPDATING;
	m_last_update = time(NULL);
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
	char peer_id[PEER_ID_LENGTH];
	m_g_cfg->get_peer_id(peer_id);
	urn.append(peer_id);

	char t[256];
	urn.append("&port=");
	sprintf(t, "%d", m_g_cfg->get_port());
	urn.append(t);

	if (event != TRACKER_EVENT_STARTED)
	{
		urn.append("&downloaded=");
		sprintf(t, "%lld", m_torrent->m_downloaded - m_downloaded);
		urn.append(t);

		urn.append("&uploaded=");
		sprintf(t, "%lld", m_torrent->m_uploaded - m_uploaded);
		urn.append(t);
	}
	else
	{
		m_downloaded = m_torrent->m_downloaded;
		m_uploaded = m_torrent->m_uploaded;

		urn.append("&downloaded=0&uploaded=0");
	}

	urn.append("&left=");
	sprintf(t, "%lld", m_torrent->m_length - m_torrent->m_downloaded);
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
	return m_nm->Socket_send(m_sock, http.c_str(), http.length(), true);
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
		return ERR_INTERNAL;
	}
	len = m_buflen - (data - m_buf);


	bencode::be_node * response = bencode::decode(data, len, false);
	if (response == NULL)
	{
		m_status = TRACKER_STATUS_ERROR;
		return ERR_INTERNAL;
	}
	//bencode::dump(response);
	bencode::be_str * b_str = NULL;
	char * c_str = NULL;
	m_status = TRACKER_STATUS_OK;
	if (bencode::get_str(response,"warning message",&b_str) == 0)
	{
		c_str = bencode::str2c_str(b_str);
		m_status = c_str;
		delete[] c_str;
	}
	if (bencode::get_str(response,"failure reason",&b_str) == 0)
	{
		c_str = bencode::str2c_str(b_str);
		m_status = c_str;
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
		m_torrent->take_peers(m_peers_count, m_peers);
	}
	m_nm->Socket_close(m_sock);
	m_nm->Socket_delete(m_sock);
	delete_socket();
	return 0;
}

int Tracker::event_sock_ready2read(network::socket_ * sock)
{
	//std::cout<<"TRACKER event_sock_ready2read "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	int len = m_nm->Socket_recv(sock, m_buf + m_buflen, BUFFER_SIZE - m_buflen);
	if (len < ERR_NO_ERROR)
		return ERR_INTERNAL;
	m_buflen += len;
	process_response();
	return ERR_NO_ERROR;
}

int Tracker::event_sock_closed(network::socket_ * sock)
{
	//std::cout<<"TRACKER event_sock_closed "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	if (m_nm->Socket_datalen(sock) > 0)
		event_sock_ready2read(sock);
	delete_socket();
	m_nm->Socket_delete(sock);
	if (m_status == TRACKER_STATUS_UPDATING)
		m_status = TRACKER_STATUS_ERROR;
	return ERR_NO_ERROR;
}

int Tracker::event_sock_sended(network::socket_ * sock)
{
	//std::cout<<"TRACKER event_sock_sended "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	return ERR_NO_ERROR;
}

int Tracker::event_sock_connected(network::socket_ * sock)
{
	//std::cout<<"TRACKER event_sock_connected "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	return send_request(m_event_after_connect);
}

int Tracker::event_sock_accepted(network::socket_ * sock)
{
	return ERR_NO_ERROR;
}

int Tracker::event_sock_timeout(network::socket_ * sock)
{
	//std::cout<<"TRACKER event_sock_timeout "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	m_status = TRACKER_STATUS_TIMEOUT;
	m_last_update = time(NULL);
	if (m_nm->Socket_datalen(sock) > 0)
		event_sock_ready2read(sock);
	delete_socket();
	m_nm->Socket_close(sock);
	return m_nm->Socket_delete(sock);
	//return ERR_NO_ERROR;
}

int Tracker::event_sock_unresolved(network::socket_ * sock)
{
	//std::cout<<"TRACKER event_sock_unresolved "<<sock->get_fd()<<" "<<m_announce<<std::endl;
	m_status = TRACKER_STATUS_UNRESOLVED;
	m_last_update = time(NULL);
	delete_socket();
	m_nm->Socket_close(sock);
	return m_nm->Socket_delete(sock);
}

int Tracker::get_peers_count()
{
	return m_peers_count;
}

const sockaddr_in * Tracker::get_peer(int i)
{
	if (i < 0 || i >= m_peers_count)
		return NULL;
	return &m_peers[i];
}

int Tracker::restore_socket()
{
	if (m_sock == NULL)
	{
		m_status = TRACKER_STATUS_CONNECTING;
		m_sock = m_nm->Socket_add_domain(m_host, 80, m_torrent);
		if (m_sock == NULL)
			return ERR_INTERNAL;
		m_torrent->m_sockets[m_sock] = this;
	}
	return ERR_NO_ERROR;
}

void Tracker::delete_socket()
{
	m_torrent->delete_socket(m_sock, this);
	m_sock = NULL;
}

int Tracker::clock()
{
	if ((uint32_t)(time(NULL) - m_last_update) >= m_interval)
	{
		return update();
	}
	return ERR_NO_ERROR;
}

int Tracker::update()
{
	if (restore_socket() != ERR_NO_ERROR)
		return ERR_INTERNAL;
	m_event_after_connect = TRACKER_EVENT_NONE;
	return ERR_NO_ERROR;
}

int Tracker::send_completed()
{
	if (restore_socket() != ERR_NO_ERROR)
			return ERR_INTERNAL;
	m_event_after_connect = TRACKER_EVENT_COMPLETED;
	return ERR_NO_ERROR;
}

int Tracker::send_stopped()
{
	if (restore_socket() != ERR_NO_ERROR)
			return ERR_INTERNAL;
	m_event_after_connect = TRACKER_EVENT_STOPPED;
	return ERR_NO_ERROR;
}

int Tracker::send_started()
{
	if (restore_socket() != ERR_NO_ERROR)
			return ERR_INTERNAL;
	m_event_after_connect = TRACKER_EVENT_STARTED;
	return ERR_NO_ERROR;
}

int Tracker::get_info(tracker_info * info)
{
	if (info == NULL)
		return ERR_NULL_REF;
	info->announce = m_announce;
	info->leechers = m_leechers;
	info->seeders = m_seeders == 0 ? m_peers_count : m_seeders;
	info->status = m_status;
	info->update_in = (time_t)m_interval - time(NULL) + m_last_update;
	return ERR_NO_ERROR;
}

Tracker::~Tracker()
{
	if(m_peers != NULL)
		delete[] m_peers;
}

}
