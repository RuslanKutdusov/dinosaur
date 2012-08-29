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

node_id distance(const node_id & n1, const node_id & n2)
{
	node_id r;
	for(size_t i = 0; i < NODE_ID_LENGHT; i++)
	{
		r[i] = n1[i] ^ n2[i];
	}
	return r;
}

node_id::node_id()
{
	clear();
}

node_id::~node_id()
{

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

void node_id::clear()
{
	memset(m_data, 0, NODE_ID_LENGHT);
}
}
}
