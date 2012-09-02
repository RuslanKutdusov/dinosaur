/*
 * node_id.cpp
 *
 *  Created on: 29.08.2012
 *      Author: ruslan
 */

#include "dht.h"

namespace dinosaur
{
namespace dht
{

node_id distance(const node_id & id1, const node_id & id2)
{
	node_id r;
	for(size_t i = 0; i < NODE_ID_LENGHT; i++)
	{
		r[i] = id1[i] ^ id2[i];
	}
	return r;
}

node_id generate_random_node_id()
{
	node_id ret;
	generate_random_node_id(ret);
	return ret;
}

void generate_random_node_id(node_id & id)
{
	for(size_t i = 0; i < NODE_ID_LENGHT; i++)
	{
		id[i] = rand() % 256 ;
	}
}

size_t get_bucket(const node_id & id1, const node_id & id2)
{
	int byte = NODE_ID_LENGHT - 1;
	for (size_t i = 0; i < NODE_ID_LENGHT; i++, byte--)
	{
		unsigned char t = id1[i] ^ id2[i];
		if (t == 0) continue;
		int bit = byte * 8;
		for (int b = 7; b >= 0; --b)
			if (t >= (1 << b)) return bit + b;
		return bit;
	}
	return 0;
}

node_id::node_id()
{
	clear();
}

node_id::node_id(const node_id & id)
{
	memcpy(this->m_data, id.m_data, NODE_ID_LENGHT);
}

node_id::node_id(const char * id)
{
	memcpy(this->m_data, id, NODE_ID_LENGHT);
}

node_id::~node_id()
{

}

node_id & node_id::operator=(const node_id & id)
{
	if (this != &id)
	{
		memcpy(this->m_data, id.m_data, NODE_ID_LENGHT);
	}
	return *this;
}

node_id & node_id::operator=(const char * id)
{
	memcpy(this->m_data, id, NODE_ID_LENGHT);
	return *this;
}

unsigned char & node_id::operator[](size_t i)
{
	if (i >= NODE_ID_LENGHT)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);
	return m_data[i];
}

const unsigned char & node_id::operator[](size_t i) const
{
	if (i >= NODE_ID_LENGHT)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);
	return m_data[i];
}

const node_id node_id::operator ^(const node_id & id)
{
	return distance(*this, id);
}

bool node_id::operator<(const node_id & id) const
{
	for(size_t i = 0; i < NODE_ID_LENGHT; i++)
	{
		if (m_data[i] != id.m_data[i])
			return m_data[i] < id.m_data[i];
	}
	return false;
}

bool node_id::operator==(const node_id & id) const
{
	return memcmp(m_data, id.m_data, NODE_ID_LENGHT);
}

bool node_id::operator!=(const node_id & id) const
{
	return !memcmp(m_data, id.m_data, NODE_ID_LENGHT);
}

void node_id::copy2(char * dst) const
{
	memcpy(dst, m_data, NODE_ID_LENGHT);
}

void node_id::copy2(unsigned char * dst) const
{
	memcpy(dst, m_data, NODE_ID_LENGHT);
}

void node_id::print()  const
{
	for(size_t i = 0; i < NODE_ID_LENGHT; i++)
		printf("%02X", m_data[i]);
	printf("\n");
}

void node_id::to_hex(node_id_hex id) const
{
	for(size_t i = 0; i < NODE_ID_LENGHT; i++)
		sprintf(&id[i * 2], "%02X", m_data[i]);
}

void node_id::clear()
{
	memset(m_data, 0, NODE_ID_LENGHT);
}
}
}
