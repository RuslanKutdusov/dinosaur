/*
 * peer.cpp
 *
 *  Created on: 28.02.2012
 *      Author: ruslan
 */


#include "torrent.h"

namespace dinosaur {
namespace torrent {

Peer::Peer()
:network::SocketAssociation()
{
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Peer default constructor";
#endif
	m_nm = NULL;
	m_g_cfg = NULL;
	m_bitfield = NULL;
	m_peer_choking= true;
	m_peer_interested = false;
	m_am_choking = true;//
	m_am_interested = false;//
	m_sleep_time = 0;
	m_downloaded = 0;
	m_uploaded = 0;
	memset(&m_buf, 0, sizeof(network::buffer));
}

int Peer::Init(sockaddr_in * addr, const TorrentInterfaceInternalPtr & torrent)
{
	memset(&m_buf, 0, sizeof(network::buffer));
	m_torrent = torrent;
	m_nm = torrent->get_nm();
	m_g_cfg = torrent->get_cfg();
	m_bitfield = new unsigned char[m_torrent->get_bitfield_length()];
	memset(m_bitfield, 0, m_torrent->get_bitfield_length());
	m_sleep_time = 0;
	m_peer_choking= true;
	m_peer_interested = false;
	m_am_choking = false;//
	m_am_interested = false;//
	m_downloaded = 0;
	m_uploaded = 0;
	m_sock.reset();
	memcpy(&m_addr, addr, sizeof(sockaddr_in));
	m_state = PEER_STATE_SEND_HANDSHAKE;
	get_peer_key(addr, m_ip);//inet_ntoa(m_sock->m_peer.sin_addr);
	return ERR_NO_ERROR;
}

/*
 * Exception::ERR_CODE_NULL_REF
 */

int Peer::Init(network::Socket & sock, const TorrentInterfaceInternalPtr & torrent, PEER_ADD peer_add)
{
	m_nm->Socket_set_assoc(sock, shared_from_this());
	memset(&m_buf, 0, sizeof(network::buffer));
	m_torrent = torrent;
	m_nm = torrent->get_nm();
	m_g_cfg = torrent->get_cfg();
	m_bitfield = new unsigned char[m_torrent->get_bitfield_length()];
	memset(m_bitfield, 0, m_torrent->get_bitfield_length());
	m_sleep_time = 0;
	m_peer_choking= true;
	m_peer_interested = false;
	m_am_choking = false;//
	m_am_interested = false;//
	m_downloaded = 0;
	m_uploaded = 0;
	m_sock = sock;
	memcpy(&m_addr, &m_sock->m_peer, sizeof(sockaddr_in));
	get_peer_key(&m_sock->m_peer, m_ip);//inet_ntoa(m_sock->m_peer.sin_addr);
	switch(peer_add)
	{
	case PEER_ADD_TRACKER:
		m_state = PEER_STATE_SEND_HANDSHAKE;
		break;
	case PEER_ADD_INCOMING:
		if (send_handshake() != ERR_NO_ERROR)
			goto_sleep();
		if (send_bitfield() != ERR_NO_ERROR)
			goto_sleep();
		m_state = PEER_STATE_GENERAL_READY;
		break;
	}
	return ERR_NO_ERROR;
}

int Peer::send_handshake()
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": sending handshake";
#endif

	unsigned char handshake[HANDSHAKE_LENGHT];
	memset(handshake, 0, HANDSHAKE_LENGHT);
	handshake[0] = '\x13';
	strncpy((char*)&handshake[1], "BitTorrent protocol", 19);
	m_torrent->copy_infohash_bin(&handshake[28]);
	m_g_cfg->get_peer_id((char*)&handshake[48]);
	try
	{
		if (m_nm->Socket_send(m_sock, handshake, HANDSHAKE_LENGHT) == HANDSHAKE_LENGHT)
			return ERR_NO_ERROR;
		else
			return ERR_INTERNAL;
	}
	catch (Exception  & e)
	{
		return ERR_INTERNAL;
	}
}

int Peer::send_bitfield()
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": sending bitfield";
#endif

	int len =  m_torrent->get_bitfield_length();
	unsigned char * bitfield = new unsigned char[5 + len];
	uint32_t net_len = htonl(len + 1);
	memcpy(bitfield, &net_len, 4);
	bitfield[4] = '\x05';
	//memcpy(&bitfield[5], m_torrent->m_bitfield, len);
	m_torrent->copy_bitfield(&bitfield[5]);
	int ret;
	try
	{
		ret = m_nm->Socket_send(m_sock, bitfield, 5 + len);
	}
	catch (Exception & e) {
		delete[] bitfield;
		return ERR_INTERNAL;
	}
	delete[] bitfield;
	if (ret == 5 + len)
		return ERR_NO_ERROR;
	else
		return ERR_INTERNAL;
}

int Peer::send_request(PIECE_INDEX piece, BLOCK_INDEX block, uint32_t block_length)
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": sending request piece=" << piece << " block=" << block;
#endif

	//<len=0013><id=6><index><begin><length>
	char request[17];
	uint32_t len = htonl(13);
	memcpy(&request[0], &len, 4);
	request[4] = '\x06';
	uint32_t piece_n = htonl(piece);
	memcpy(&request[5], &piece_n, 4);
	BLOCK_OFFSET offset = block * BLOCK_LENGTH;
	offset = htonl(offset);
	memcpy(&request[9], &offset, 4);
	block_length = htonl(block_length);
	memcpy(&request[13], &block_length, 4);
	int ret;
	try
	{
		ret = m_nm->Socket_send(m_sock, request, 17);
	}
	catch (Exception &e)
	{
		return ERR_INTERNAL;
	}
	if (ret == 17)
		return ERR_NO_ERROR;
	return ERR_INTERNAL;
}

int Peer::send_piece(PIECE_INDEX piece, BLOCK_OFFSET offset, uint32_t length, char * block)
{ //<len=0009+X><id=7><index><begin><block>
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": sending piece piece=" << piece << " offset="<< offset;
#endif

	char mes[BLOCK_LENGTH + 13];
	uint32_t _length = htonl(length + 9);
	memcpy(&mes[0], &_length, 4);
	mes[4] = '\x07';
	uint32_t index = htonl(piece);
	memcpy(&mes[5], &index, 4);
	uint32_t begin =htonl(offset);
	memcpy(&mes[9], &begin, 4);
	memcpy(&mes[13], block, length);
	try
	{
		if (m_nm->Socket_send(m_sock, mes, length + 13) == length + 13)
			return ERR_NO_ERROR;
		else
			return ERR_INTERNAL;
	}
	catch (Exception & e) {
		return ERR_INTERNAL;
	}
}

int Peer::send_choke()
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": sending choke";
#endif

	char mes[5];
	memset(mes, 0, 5);
	mes[3] = '\x01';
	mes[4] = '\x00';
	try
	{
		if (m_nm->Socket_send(m_sock, mes, 5) == 5)
			return ERR_NO_ERROR;
		else
			return ERR_INTERNAL;
	}
	catch (Exception & e) {
		return ERR_INTERNAL;
	}
}

int Peer::send_unchoke()
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": sending unchoke";
#endif

	char mes[5];
	memset(mes, 0, 5);
	mes[3] = '\x01';
	mes[4] = '\x01';
	try
	{
		if (m_nm->Socket_send(m_sock, mes, 5) == 5)
			return ERR_NO_ERROR;
		else
			return ERR_INTERNAL;
	}
	catch (Exception & e) {
		return ERR_INTERNAL;
	}
}

int Peer::send_interested()
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": sending interested";
#endif

	char mes[5];
	memset(mes, 0, 5);
	mes[3] = '\x01';
	mes[4] = '\x02';
	try
	{
		if (m_nm->Socket_send(m_sock, mes, 5) == 5)
			return ERR_NO_ERROR;
		else
			return ERR_INTERNAL;
	}
	catch (Exception & e) {
		return ERR_INTERNAL;
	}
}

int Peer::send_not_interested()
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": sending not interested";
#endif

	char mes[5];
	memset(mes, 0, 5);
	mes[3] = '\x01';
	mes[4] = '\x03';
	try
	{
		if (m_nm->Socket_send(m_sock, mes, 5) == 5)
			return ERR_NO_ERROR;
		else
			return ERR_INTERNAL;
	}
	catch (Exception & e) {
		return ERR_INTERNAL;
	}
}

int Peer::send_have(PIECE_INDEX piece_index)
{//have: <len=0005><id=4><piece index>
	if (m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return ERR_INTERNAL;

#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": sending have";
#endif

	char mes[9];
	memset(mes, 0, 9);
	mes[3] = '\x5';
	mes[4] = '\x4';
	uint32_t pi = htonl(piece_index);
	memcpy(&mes[5], &pi, 4);
	try
	{
		if (m_nm->Socket_send(m_sock, mes, 9) == 9)
			return ERR_NO_ERROR;
		else
			return ERR_INTERNAL;
	}
	catch (Exception & e) {
		return ERR_INTERNAL;
	}

}

int Peer::process_messages()
{
	//is handshake
	if (m_buf.length - m_buf.pos >= HANDSHAKE_LENGHT &&
		m_buf.data[m_buf.pos] == '\x13' &&
		memcmp(&m_buf.data[m_buf.pos + 1], "BitTorrent protocol", 19) == 0 &&
		m_torrent->memcmp_infohash_bin((unsigned char*)&m_buf.data[m_buf.pos + 28]) == 0)
	{
		if (m_state != PEER_STATE_WAIT_HANDSHAKE)
			return ERR_INTERNAL;
		m_buf.pos += HANDSHAKE_LENGHT;
		m_state = PEER_STATE_GENERAL_READY;
		#ifdef PEER_DEBUG
			LOG(INFO) << "Peer " << m_ip.c_str() << ": is received handshake\n";
		#endif
	}
	char ip[16];
	memset(ip, 0, 16);
	strcpy(ip,  inet_ntoa(m_sock->m_peer.sin_addr));
	uint32_t piece_count = m_torrent->get_piece_count();
	BLOCK_ID block_id;
	while((int)m_buf.length - (int)m_buf.pos > 4)
	{
		uint32_t len = 0;
		memcpy(&len, &m_buf.data[m_buf.pos], 4);
		len = ntohl(len);
		if (m_buf.length - m_buf.pos < len + 4)
			goto end;
		m_buf.pos += 4;
		if (len != 0)
		{
			char id = m_buf.data[m_buf.pos++];
			switch (id)
			{
			case '\x00'://choke: <len=0001><id=0>
				#ifdef PEER_DEBUG
					LOG(INFO) << "Peer " << m_ip.c_str() << ": is received choke";
				#endif
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				m_peer_choking = true;
				if (m_state == PEER_STATE_WAIT_UNCHOKE)
					return ERR_INTERNAL;//m_state = PEER_STATE_GENERAL_READY; иначе зацикливаемся
				break;
			case '\x01'://unchoke: <len=0001><id=1>
				#ifdef PEER_DEBUG
					LOG(INFO) << "Peer " << m_ip.c_str() << ": is received unchoke";
				#endif
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				m_peer_choking = false;
				if (m_state == PEER_STATE_WAIT_UNCHOKE)
					m_state = PEER_STATE_REQUEST_READY;
				break;
			case '\x02'://interested: <len=0001><id=2>
				#ifdef PEER_DEBUG
					LOG(INFO) << "Peer " << m_ip.c_str() << ": is received interested";
				#endif
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				m_peer_interested = true;
				send_unchoke();
				break;
			case '\x03'://not interested: <len=0001><id=3>
				#ifdef PEER_DEBUG
					LOG(INFO) << "Peer " << m_ip.c_str() << ": is received not interested";
				#endif
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				m_peer_interested = false;
				break;
			case '\x04'://have: <len=0005><id=4><piece index>
				#ifdef PEER_DEBUG
					LOG(INFO) << "Peer " << m_ip.c_str() << ": is received have";
				#endif
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				PIECE_INDEX piece_index;
				memcpy(&piece_index, &m_buf.data[m_buf.pos], 4);
				piece_index = ntohl(piece_index);
				if (piece_index >= piece_count)
					return ERR_INTERNAL;
				m_available_pieces.insert(piece_index);
				set_bitfield(piece_index, piece_count, m_bitfield);
				break;
			case '\x05'://bitfield: <len=0001+X><id=5><bitfield>
				#ifdef PEER_DEBUG
					LOG(INFO) << "Peer " << m_ip.c_str() << ": is received bitfield";
				#endif

				if (len - 1 != m_torrent->get_bitfield_length())
					return ERR_INTERNAL;
				if (m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				memcpy(m_bitfield, &m_buf.data[m_buf.pos], len - 1);
				for(PIECE_INDEX i = 0; i < piece_count; i++)
				{
					if (bit_in_bitfield(i, piece_count, m_bitfield))
						m_available_pieces.insert(i);
				}
				break;
			case '\x06'://request: <len=0013><id=6><index><begin><length>
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				PIECE_INDEX index;//индекс куска
				BLOCK_OFFSET offset;
				uint32_t length;
				memcpy(&index,&m_buf.data[m_buf.pos], 4);
				index = ntohl(index);
				memcpy(&offset,&m_buf.data[m_buf.pos + 4], 4);
				offset = ntohl(offset);
				memcpy(&length,&m_buf.data[m_buf.pos + 8], 4);
				length = ntohl(length);
				//проверяем валидность индексов, смещений
				if (length == 0 || length > BLOCK_LENGTH || offset % BLOCK_LENGTH != 0)
					return ERR_INTERNAL;
				BLOCK_INDEX block_index;
				m_torrent->get_block_index_by_offset(index, offset, block_index) ;
				uint32_t real_block_length;
				m_torrent->get_block_length_by_index(index, block_index, real_block_length);
				if (real_block_length != length)
					return ERR_INTERNAL;
				//кладем индекс блока в очередь запросов
				generate_block_id(index, block_index, block_id);
				m_requests_queue.insert(block_id);

				#ifdef PEER_DEBUG
					LOG(INFO) << "Peer " << m_ip.c_str() << ": is received request piece=" << index << " block=" << block_index;
				#endif

				break;
			case '\x07'://piece: <len=0009+X><id=7><index><begin><block>
			{
				PIECE_INDEX index;//индекс куска
				BLOCK_OFFSET offset;
				uint32_t block_length = len - 9;
				if (m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				if (block_length == 0 || block_length > BLOCK_LENGTH)
					return ERR_INTERNAL;
				memcpy(&index,&m_buf.data[m_buf.pos], 4);
				index = ntohl(index);
				memcpy(&offset,&m_buf.data[m_buf.pos + 4], 4);
				offset = ntohl(offset);
				if (offset % BLOCK_LENGTH != 0)
					return ERR_INTERNAL;

				generate_block_id(index, offset / BLOCK_LENGTH, block_id);
				std::set<BLOCK_ID>::iterator iter = m_requested_blocks.find(block_id);

				#ifdef PEER_DEBUG
					LOG(INFO) << "Peer " << m_ip.c_str() << ": is received piece piece=" << index << " block=" << offset / BLOCK_LENGTH;
				#endif

				if (iter == m_requested_blocks.end())
					return ERR_INTERNAL;

				if (m_torrent->save_block(index, offset, block_length, &m_buf.data[m_buf.pos + 8]) != ERR_NO_ERROR)
					return ERR_INTERNAL;

				m_requested_blocks.erase(iter);
				m_downloaded += block_length;

				break;
			}
			case '\x08'://cancel: <len=0013><id=8><index><begin><length>
			{
				if (m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				PIECE_INDEX index;//индекс куска
				BLOCK_OFFSET offset;
				uint32_t length;
				memcpy(&index,&m_buf.data[m_buf.pos], 4);
				index = ntohl(index);
				memcpy(&offset,&m_buf.data[m_buf.pos + 4], 4);
				offset = ntohl(offset);
				memcpy(&length,&m_buf.data[m_buf.pos + 8], 4);
				length = ntohl(length);
				//проверяем валидность индексов, смещений
				if (length == 0 || length > BLOCK_LENGTH || offset % BLOCK_LENGTH != 0)
					return ERR_INTERNAL;
				uint32_t block_index;
				m_torrent->get_block_index_by_offset(index, offset, block_index);

				#ifdef PEER_DEBUG
					LOG(INFO) << "Peer " << m_ip.c_str() << ": is received cancel piece=" << index << " block=" << block_index;
				#endif

				uint32_t real_block_length;
				m_torrent->get_block_length_by_index(index, block_index, real_block_length);
				if (real_block_length != length)
					return ERR_INTERNAL;
				generate_block_id(index, block_index, block_id);
				m_requested_blocks.erase(block_id);
				break;
			}
			case '\x09'://port: <len=0003><id=9><listen-port>
				if (m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				break;
			default:
				return ERR_INTERNAL;
				break;
			}
			m_buf.pos += len - 1;
		}
	}
end:
	char temp[BUFFER_SIZE];
	size_t len = m_buf.length - m_buf.pos;
	memcpy(temp, &m_buf.data[m_buf.pos], len);
	m_buf.length = len;
	m_buf.pos = 0;
	memcpy(m_buf.data, temp, len);
	return ERR_NO_ERROR;
}

int Peer::event_sock_ready2read(network::Socket sock)
{
	int ret;
	try
	{
		ret = m_nm->Socket_recv(sock, m_buf.data + m_buf.length, BUFFER_SIZE - m_buf.length);
	}
	catch (Exception & e) {
		return ERR_INTERNAL;
	}
	m_buf.length += ret;
	if (process_messages() != ERR_NO_ERROR)
	{
		goto_sleep();
	}
	return ERR_NO_ERROR;
}

int Peer::event_sock_closed(network::Socket sock)
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": close connection";
#endif
	try
	{
		if (m_nm->Socket_datalen(sock) > 0)
			event_sock_ready2read(sock);
		goto_sleep();
		return ERR_NO_ERROR;
	}
	catch (Exception & e) {
		return ERR_INTERNAL;
	}

}

int Peer::event_sock_sended(network::Socket sock)
{
	return 0;

}

int Peer::event_sock_connected(network::Socket sock)
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": connected";
#endif
	return 0;

}

int Peer::event_sock_accepted(network::Socket sock, network::Socket accepted_sock)
{
	return 0;
}

int Peer::event_sock_timeout(network::Socket sock)
{
#ifdef PEER_DEBUG
	LOG(INFO) << "Peer " << m_ip.c_str() << ": timeout";
#endif
	try
	{
		if (m_nm->Socket_datalen(sock) > 0)
			event_sock_ready2read(sock);
		goto_sleep();
		return ERR_NO_ERROR;
	}
	catch (Exception & e) {
		return ERR_INTERNAL;
	}
}

int Peer::event_sock_unresolved(network::Socket sock)
{
	return ERR_NO_ERROR;
}

bool Peer::have_piece(PIECE_INDEX piece_index)
{
	return bit_in_bitfield(piece_index, m_torrent->get_piece_count(), m_bitfield);
}

bool Peer::peer_interested()
{
	return m_peer_interested;
}

bool Peer::is_choking()
{
	return m_peer_choking;
}

int Peer::clock()
{
	if (m_state == PEER_STATE_SEND_HANDSHAKE)
	{
		if (m_sock == NULL)
		{
			try
			{
				m_nm->Socket_add(&m_addr, shared_from_this(), m_sock);
			}
			catch (...)
			{
				goto_sleep();
				return ERR_INTERNAL;
			}
		}
		if (send_handshake() != ERR_NO_ERROR)
			goto_sleep();
		if (send_bitfield() != ERR_NO_ERROR)
			goto_sleep();
		m_state = PEER_STATE_WAIT_HANDSHAKE;
	}
	if (!m_peer_choking)
	{
		if (m_state == PEER_STATE_GENERAL_READY )
			m_state = PEER_STATE_REQUEST_READY;
	}
	else if (m_state == PEER_STATE_GENERAL_READY || m_state == PEER_STATE_REQUEST_READY)
	{
		send_interested();
		m_state = PEER_STATE_WAIT_UNCHOKE;
	}
	//если находимся в нужном состоянии и очередь не пуста
	if ((m_state == PEER_STATE_GENERAL_READY || m_state == PEER_STATE_REQUEST_READY || m_state == PEER_STATE_WAIT_UNCHOKE) && !m_requests_queue.empty())
	{
		char block[BLOCK_LENGTH];
		uint32_t block_length;
		PIECE_INDEX piece_index;
		BLOCK_INDEX block_index;
		std::set<BLOCK_ID>::iterator iter = m_requests_queue.begin();
		get_piece_block_from_block_id(*iter, piece_index, block_index);
		//если удалось прочитать блок и отправить, удаляем индекс блока из очереди
		if (m_torrent->read_block(piece_index, block_index, block, block_length) == ERR_NO_ERROR &&
				send_piece(piece_index, BLOCK_LENGTH * block_index, block_length, block) == ERR_NO_ERROR)
		{
			m_requests_queue.erase(iter);
			m_torrent->inc_uploaded(block_length);
			m_uploaded += block_length;
			//LOG(INFO) << "rx=%f tx=%f\n", get_rx_speed(), get_tx_speed());
		}
	}
	if (m_state == PEER_STATE_REQUEST_READY && m_requested_blocks.size() <= PEER_MAX_REQUEST_NUMBER && !m_blocks2request.empty())
	{
		std::set<BLOCK_ID>::iterator iter = m_blocks2request.begin();
		PIECE_INDEX piece;
		BLOCK_INDEX block;
		uint32_t length;
		get_piece_block_from_block_id(*iter, piece, block);
		m_torrent->get_block_length_by_index(piece, block, length);
		if (send_request(piece, block, length) == ERR_NO_ERROR)
		{
			m_requested_blocks.insert(*iter);
			m_blocks2request.erase(iter);
		}
	}
	return ERR_NO_ERROR;
}


bool Peer::is_sleep()
{
	if (m_state == PEER_STATE_SLEEP)
	{
		if (time(NULL) - m_sleep_time < PEER_SLEEP_TIME)
			return true;
		m_state = PEER_STATE_SEND_HANDSHAKE;
		return false;
	}
	return false;
}

void Peer::wake_up()
{
	if (m_state == PEER_STATE_SLEEP)
		m_state = PEER_STATE_SEND_HANDSHAKE;
}

void Peer::goto_sleep()
{
	m_sleep_time = time(NULL);
	if (m_state == PEER_STATE_SLEEP)
		return;
	m_nm->Socket_delete(m_sock);
	m_peer_choking = true;
	m_peer_interested = false;
	m_am_choking = false;//
	m_am_interested = false;//
	memset(&m_buf, 0, sizeof(network::buffer));
	//memset(m_bitfield, 0, m_torrent->m_bitfield_len);
	m_state = PEER_STATE_SLEEP;
}

bool Peer::may_request()
{
	if (m_peer_choking || m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return false;
	if (m_blocks2request.size() > 0 || m_requested_blocks.size() > 0)
		return false;
	return true;
}

int Peer::request(PIECE_INDEX piece_index, BLOCK_INDEX block_index)
{
	BLOCK_ID id;
	generate_block_id(piece_index, block_index, id);
	return request(id);
}

int Peer::request(const BLOCK_ID & block_id)
{
	m_blocks2request.insert(block_id);
	return ERR_NO_ERROR;
}

bool Peer::get_requested_block(BLOCK_ID & block_id)
{
	if (!m_requested_blocks.empty())
	{
		std::set<BLOCK_ID>::iterator iter = m_requested_blocks.begin();
		block_id = *iter;
		m_requested_blocks.erase(iter);
		return true;
	}
	if (!m_blocks2request.empty())
	{
		std::set<BLOCK_ID>::iterator iter = m_blocks2request.begin();
		block_id = *iter;
		m_blocks2request.erase(iter);
		return true;
	}

	return false;
}

int Peer::get_rx_speed()
{
	try
	{
		return m_nm->Socket_get_rx_speed(m_sock);
	}
	catch (Exception & e) {
		return 0;
	}
	return 0;
}

int Peer::get_tx_speed()
{
	try
	{
		return m_nm->Socket_get_tx_speed(m_sock);
	}
	catch (Exception & e) {
		return 0;
	}
	return 0;
}

const std::string & Peer::get_ip_str()
{
	return m_ip;
}

int Peer::get_info(info::peer & ref)
{
	ref.downSpeed = get_rx_speed();
	ref.upSpeed = get_tx_speed();
	ref.downloaded = m_downloaded;
	ref.uploaded = m_uploaded;
	ref.available = m_available_pieces.size() / m_torrent->get_piece_count();
	ref.blocks2request = m_blocks2request.size();
	ref.requested_blocks = m_requested_blocks.size();
	ref.may_request = may_request();
	memset(ref.ip, 0, 22);
	memcpy(ref.ip, m_ip.c_str(), 22);
	return ERR_NO_ERROR;
}

void Peer::prepare2release()
{
	m_nm->Socket_delete(m_sock);
}

Peer::~Peer()
{
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Peer destructor "<<  m_ip.c_str();
#endif
	if (m_bitfield != NULL)
		delete[] m_bitfield;
	prepare2release();
#ifdef BITTORRENT_DEBUG
	LOG(INFO) << "Peer destroyed "<< m_ip.c_str();
#endif
}

}
}
