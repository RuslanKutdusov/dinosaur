/*
 * peer.cpp
 *
 *  Created on: 28.02.2012
 *      Author: ruslan
 */


#include "torrent.h"

namespace torrent {

Peer::Peer()
:network::sock_event()
{
	m_nm = NULL;
	m_g_cfg = NULL;
	m_torrent = NULL;
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

Peer::Peer(sockaddr_in * addr, Torrent * torrent, PEER_ADD peer_add )
:network::sock_event()
{
	if (addr == NULL)
		throw NoneCriticalException("Can not initialize the peer");
	//memset(&m_buf, 0, sizeof(network::buffer));
	memcpy(&m_addr, addr, sizeof(sockaddr_in));
	network::socket_ * sock = torrent->m_nm->Socket_add(&m_addr,torrent);
	init(sock, torrent, peer_add);
}

Peer::Peer(network::socket_ * sock, Torrent * torrent, PEER_ADD peer_add)
:network::sock_event()
{
	init(sock, torrent, peer_add);
}

void Peer::init(network::socket_ * sock, Torrent * torrent, PEER_ADD peer_add)
{
	if (sock == NULL || torrent == NULL)
		throw NoneCriticalException("Can not initialize the peer");
	memset(&m_buf, 0, sizeof(network::buffer));
	m_torrent = torrent;
	m_nm = torrent->m_nm;
	m_g_cfg = torrent->m_g_cfg;
	m_bitfield = new unsigned char[m_torrent->m_bitfield_len];
	memset(m_bitfield, 0, m_torrent->m_bitfield_len);
	m_sleep_time = 0;
	m_peer_choking= true;
	m_peer_interested = false;
	m_am_choking = false;//
	m_am_interested = false;//
	m_downloaded = 0;
	m_uploaded = 0;
	if (m_nm == NULL || m_g_cfg == NULL)
		throw NoneCriticalException("Can not initialize the peer");
	m_sock = sock;
	memcpy(&m_addr, &m_sock->m_peer, sizeof(sockaddr_in));
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
	get_peer_key(&m_sock->m_peer, &m_ip);//inet_ntoa(m_sock->m_peer.sin_addr);
	//std::cout<<"PEER created "<<m_ip<<std::endl;
}

network::socket_ * Peer::get_sock()
{
	return m_sock;
}

int Peer::send_handshake()
{
	if (m_state != PEER_STATE_SEND_HANDSHAKE)
		return ERR_INTERNAL;
	char handshake[HANDSHAKE_LENGHT];
	memset(handshake, 0, HANDSHAKE_LENGHT);
	handshake[0] = '\x13';
	strncpy(&handshake[1], "BitTorrent protocol", 19);
	memcpy(&handshake[28], m_torrent->m_info_hash_bin, 20);
	m_g_cfg->get_peer_id(&handshake[48]);
	if (m_nm->Socket_send(m_sock, handshake, HANDSHAKE_LENGHT) == HANDSHAKE_LENGHT)
		return ERR_NO_ERROR;
	else
		return ERR_INTERNAL;
}

int Peer::send_bitfield()
{
	if (m_state == PEER_STATE_WAIT_HANDSHAKE)
		return ERR_INTERNAL;
	int len =  m_torrent->m_bitfield_len;
	char * bitfield = new char[5 + len];
	uint32_t net_len = htonl(len + 1);
	memcpy(bitfield, &net_len, 4);
	bitfield[4] = '\x05';
	memcpy(&bitfield[5], m_torrent->m_bitfield, len);
	int ret =  m_nm->Socket_send(m_sock, bitfield, 5 + len);
	delete[] bitfield;
	if (ret == 5 + len)
		return ERR_NO_ERROR;
	else
		return ERR_INTERNAL;
}

int Peer::send_request(uint32_t piece, uint32_t block, uint32_t block_length)
{
	if (m_peer_choking || m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return ERR_INTERNAL;
	//<len=0013><id=6><index><begin><length>
	char request[17];
	uint32_t len = htonl(13);
	memcpy(&request[0], &len, 4);
	request[4] = '\x06';
	uint32_t piece_n = htonl(piece);
	memcpy(&request[5], &piece_n, 4);
	uint32_t offset = block * BLOCK_LENGTH;
	offset = htonl(offset);
	memcpy(&request[9], &offset, 4);
	block_length = htonl(block_length);
	memcpy(&request[13], &block_length, 4);
	int ret = m_nm->Socket_send(m_sock, request, 17);
	if (ret == 17)
	{
		uint64_t id = generate_block_id(piece, block);
		m_requested_blocks.insert(id);
		return ERR_NO_ERROR;
	}
	return ERR_INTERNAL;
}

int Peer::send_piece(uint32_t piece, uint32_t offset, uint32_t length, char * block)
{ //<len=0009+X><id=7><index><begin><block>
	if (m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return ERR_INTERNAL;
	char mes[BLOCK_LENGTH + 13];
	uint32_t _length = htonl(length + 9);
	memcpy(&mes[0], &_length, 4);
	mes[4] = '\x07';
	uint32_t index = htonl(piece);
	memcpy(&mes[5], &index, 4);
	uint32_t begin =htonl(offset);
	memcpy(&mes[9], &begin, 4);
	memcpy(&mes[13], block, length);
	if (m_nm->Socket_send(m_sock, mes, length + 13) == (int)length + 13)
		return ERR_NO_ERROR;
	else
		return ERR_INTERNAL;
}

int Peer::send_choke()
{
	if (m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return ERR_INTERNAL;
	char mes[5];
	memset(mes, 0, 5);
	mes[3] = '\x01';
	mes[4] = '\x00';
	if (m_nm->Socket_send(m_sock, mes, 5) == 5)
		return ERR_NO_ERROR;
	else
		return ERR_INTERNAL;
}

int Peer::send_unchoke()
{
	if (m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return ERR_INTERNAL;
	char mes[5];
	memset(mes, 0, 5);
	mes[3] = '\x01';
	mes[4] = '\x01';
	if (m_nm->Socket_send(m_sock, mes, 5) == 5)
		return ERR_NO_ERROR;
	else
		return ERR_INTERNAL;
}

int Peer::send_interested()
{
	if (m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return ERR_INTERNAL;
	char mes[5];
	memset(mes, 0, 5);
	mes[3] = '\x01';
	mes[4] = '\x02';
	if (m_nm->Socket_send(m_sock, mes, 5) == 5)
		return ERR_NO_ERROR;
	else
		return ERR_INTERNAL;
}

int Peer::send_not_interested()
{
	if (m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return ERR_INTERNAL;
	char mes[5];
	memset(mes, 0, 5);
	mes[3] = '\x01';
	mes[4] = '\x03';
	if (m_nm->Socket_send(m_sock, mes, 5) == 5)
		return ERR_NO_ERROR;
	else
		return ERR_INTERNAL;
}

int Peer::send_have(uint32_t piece_index)
{//have: <len=0005><id=4><piece index>
	if (m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return ERR_INTERNAL;
	char mes[9];
	memset(mes, 0, 9);
	mes[3] = '\x5';
	mes[4] = '\x4';
	uint32_t pi = htonl(piece_index);
	memcpy(&mes[5], &pi, 4);
	if (m_nm->Socket_send(m_sock, mes, 9) == 9)
		return ERR_NO_ERROR;
	else
		return ERR_INTERNAL;

}

bool Peer::may_request()
{
	if (m_peer_choking || m_state == PEER_STATE_WAIT_HANDSHAKE || m_state == PEER_STATE_SEND_HANDSHAKE || m_state == PEER_STATE_SLEEP)
			return false;
	return true;//!m_peer_choking;
}

bool Peer::request_limit()
{
	return m_requested_blocks.size() >= PEER_MAX_REQUEST_NUMBER;//m_torrent->m_piece_length / BLOCK_LENGTH;
}

int Peer::process_messages()
{
	//is handshake
	if (m_buf.length - m_buf.pos >= HANDSHAKE_LENGHT &&
		m_buf.data[m_buf.pos] == '\x13' &&
		memcmp(&m_buf.data[m_buf.pos + 1], "BitTorrent protocol", 19) == 0 &&
		memcmp(&m_buf.data[m_buf.pos + 28], m_torrent->m_info_hash_bin, SHA1_LENGTH) == 0)
	{
		if (m_state != PEER_STATE_WAIT_HANDSHAKE)
			return ERR_INTERNAL;
		m_buf.pos += HANDSHAKE_LENGHT;
		m_state = PEER_STATE_GENERAL_READY;
	}
	char ip[16];
	memset(ip, 0, 16);
	strcpy(ip,  inet_ntoa(m_sock->m_peer.sin_addr));
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
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				m_peer_choking = true;
				if (m_state == PEER_STATE_WAIT_UNCHOKE)
					m_state = PEER_STATE_GENERAL_READY;
				break;
			case '\x01'://unchoke: <len=0001><id=1>
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				m_peer_choking = false;
				if (m_state == PEER_STATE_WAIT_UNCHOKE)
					m_state = PEER_STATE_REQUEST_READY;
				break;
			case '\x02'://interested: <len=0001><id=2>
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				m_peer_interested = true;
				send_unchoke();
				break;
			case '\x03'://not interested: <len=0001><id=3>
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				m_peer_interested = false;
				break;
			case '\x04'://have: <len=0005><id=4><piece index>
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				uint32_t piece_index;
				memcpy(&piece_index, &m_buf.data[m_buf.pos], 4);
				piece_index = ntohl(piece_index);
				if (piece_index >= m_torrent->m_piece_count)
					return ERR_INTERNAL;
				m_available_pieces.insert(piece_index);
				set_bitfield(piece_index, m_torrent->m_piece_count, m_bitfield);
				break;
			case '\x05'://bitfield: <len=0001+X><id=5><bitfield>

				if (len - 1 != m_torrent->m_bitfield_len)
					return ERR_INTERNAL;
				if (m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				memcpy(m_bitfield, &m_buf.data[m_buf.pos], len - 1);
				for(uint32_t i = 0; i < m_torrent->m_piece_count; i++)
				{
					if (bit_in_bitfield(i, m_torrent->m_piece_count, m_bitfield))
						m_available_pieces.insert(i);
				}
				break;
			case '\x06'://request: <len=0013><id=6><index><begin><length>
				if ( m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				uint32_t index;//индекс куска
				uint32_t offset;
				uint32_t length;
				uint64_t block_id;
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
				if (m_torrent->m_torrent_file.get_block_index_by_offset(index, offset, &block_index) != ERR_NO_ERROR)
					return ERR_INTERNAL;
				uint32_t real_block_length;
				if (m_torrent->m_torrent_file.get_block_length_by_index(index, block_index, &real_block_length) != ERR_NO_ERROR || real_block_length != length)
					return ERR_INTERNAL;
				//кладем индекс блока в очередь запросов
				block_id = generate_block_id(index, block_index);
				m_requests_queue.insert(block_id);
				break;
			case '\x07'://piece: <len=0009+X><id=7><index><begin><block>
			{
				uint32_t index;//индекс куска
				uint32_t offset;
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
				m_downloaded += block_length;
				if (m_torrent->m_torrent_file.save_block(index, offset, block_length, &m_buf.data[m_buf.pos + 8]) != ERR_NO_ERROR)
					return ERR_INTERNAL;
				break;
			}
			case '\x08'://cancel: <len=0013><id=8><index><begin><length>
			{
				if (m_state == PEER_STATE_WAIT_HANDSHAKE)
					return ERR_INTERNAL;
				uint32_t index;//индекс куска
				uint32_t offset;
				uint32_t length;
				uint64_t block_id;
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
				if (m_torrent->m_torrent_file.get_block_index_by_offset(index, offset, &block_index) != ERR_NO_ERROR)
					return ERR_INTERNAL;
				uint32_t real_block_length;
				if (m_torrent->m_torrent_file.get_block_length_by_index(index, block_index, &real_block_length) != ERR_NO_ERROR || real_block_length != length)
					return ERR_INTERNAL;
				block_id = generate_block_id(index, block_index);
				m_requests_queue.erase(block_id);
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

int Peer::event_sock_ready2read(network::socket_ * sock)
{
	int ret = m_nm->Socket_recv(sock, m_buf.data + m_buf.length, BUFFER_SIZE - m_buf.length);
	if (ret != -1)
		m_buf.length += ret;
	if (process_messages() != ERR_NO_ERROR)
	{
		goto_sleep();
	}
	return ERR_NO_ERROR;
}

int Peer::event_sock_closed(network::socket_ * sock)
{
	if (m_nm->Socket_datalen(sock) > 0)
		event_sock_ready2read(sock);
	goto_sleep();
	return ERR_NO_ERROR;

}

int Peer::event_sock_sended(network::socket_ * sock)
{
	//std::cout<<"PEER sended\n";
	return 0;

}

int Peer::event_sock_connected(network::socket_ * sock)
{
	//std::cout<<"PEER connected "<<m_ip<<std::endl;
	return 0;

}

int Peer::event_sock_accepted(network::socket_ * sock)
{
	//std::cout<<"PEER accepted\n";
	return 0;

}

int Peer::event_sock_timeout(network::socket_ * sock)
{
	if (m_nm->Socket_datalen(sock) > 0)
		event_sock_ready2read(sock);
	goto_sleep();
	return ERR_NO_ERROR;
}

int Peer::event_sock_unresolved(network::socket_ * sock)
{
	return ERR_NO_ERROR;
}

bool Peer::have_piece(uint32_t piece_index)
{
	return bit_in_bitfield(piece_index, m_torrent->m_piece_count, m_bitfield);
}

int Peer::clock()
{
	//крутим автомат
	if (m_state == PEER_STATE_SLEEP && time(NULL) - m_sleep_time > PEER_SLEEP_TIME)
	{
		if (m_torrent->is_downloaded())
			return ERR_NO_ERROR;
		m_sock = m_nm->Socket_add(&m_addr,m_torrent);
		if (m_sock == NULL)
		{
			goto_sleep();
			return ERR_INTERNAL;
		}
		m_torrent->add_socket(m_sock, this);
		m_state = PEER_STATE_SEND_HANDSHAKE;
		//std::cout<<"PEER wake up "<<m_ip<<std::endl;
	}
	if (m_state == PEER_STATE_SEND_HANDSHAKE)
	{
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
		std::set<uint64_t>::iterator iter = m_requests_queue.begin();
		uint64_t id = *iter;
		uint32_t piece_index = get_piece_from_id(id);
		uint32_t block_index = get_block_from_id(id);
		//если удалось прочитать блок и отправить, удаляем индекс блока из очереди
		if (m_torrent->m_torrent_file.read_block(piece_index, block_index, block, &block_length) == ERR_NO_ERROR &&
				send_piece(piece_index, BLOCK_LENGTH * block_index, block_length, block) == ERR_NO_ERROR)
		{
			m_requests_queue.erase(iter);
			m_torrent->m_uploaded += block_length;
			m_uploaded += block_length;
			//printf("rx=%f tx=%f\n", get_rx_speed(), get_tx_speed());
		}
	}
	return ERR_NO_ERROR;
}

void Peer::goto_sleep()
{
	m_torrent->delete_socket(m_sock, this);
	m_nm->Socket_delete(m_sock);
	m_sock = NULL;
	m_peer_choking= true;
	m_peer_interested = false;
	m_am_choking = false;//
	m_am_interested = false;//
	memset(&m_buf, 0, sizeof(network::buffer));
	//memset(m_bitfield, 0, m_torrent->m_bitfield_len);
	m_sleep_time = time(NULL);
	m_state = PEER_STATE_SLEEP;
}

int Peer::wake_up(network::socket_ * sock, PEER_ADD peer_add)
{
	//если не спит
	if (m_state != PEER_STATE_SLEEP)
		return ERR_INTERNAL;
	m_sock = sock;
	if (peer_add == PEER_ADD_INCOMING)
	{
		printf("PEER_ADD_INCOMING %s\n", m_ip.c_str());
		if (send_handshake() != ERR_NO_ERROR)
			goto_sleep();
		if (send_bitfield() != ERR_NO_ERROR)
			goto_sleep();
		printf("peer waked up %s\n", m_ip.c_str());
		m_state = PEER_STATE_GENERAL_READY;
	}
	if (peer_add == PEER_ADD_TRACKER)
	{
		m_sock = m_nm->Socket_add(&m_addr,m_torrent);
		if (m_sock == NULL)
		{
			goto_sleep();
			return ERR_INTERNAL;
		}
		m_state = PEER_STATE_SEND_HANDSHAKE;
	}
	//std::cout<<"PEER wake up 2"<<m_ip<<std::endl;
	return ERR_NO_ERROR;
}

bool Peer::no_requested_blocks()
{
	return m_requested_blocks.empty();
}

uint64_t Peer::get_requested_block()
{
	std::set<uint64_t>::iterator iter = m_requested_blocks.begin();
	uint64_t id = *iter;
	m_requested_blocks.erase(iter);
	return id;
}

void Peer::erase_requested_block(uint64_t block)
{
	m_requested_blocks.erase(block);
}

double Peer::get_rx_speed()
{
	return m_nm->Socket_get_rx_speed(m_sock);
}

double Peer::get_tx_speed()
{
	return m_nm->Socket_get_tx_speed(m_sock);
}

std::string Peer::get_ip_str()
{
	return m_ip;
}

int Peer::get_info(peer_info * info)
{
	if (info == NULL)
		return ERR_NULL_REF;
	info->downSpeed = get_rx_speed();
	info->upSpeed = get_tx_speed();
	info->downloaded = m_downloaded;
	info->uploaded = m_uploaded;
	info->available = m_available_pieces.size() / m_torrent->m_piece_count;
	memset(info->ip, 0, 22);
	memcpy(info->ip, m_ip.c_str(), 22);
	return ERR_NO_ERROR;
}

Peer::~Peer()
{
	if (m_bitfield != NULL)
		delete[] m_bitfield;
	m_nm->Socket_delete(m_sock);
	//std::cout<<"Peer destroyed "<<std::endl;
}

}
