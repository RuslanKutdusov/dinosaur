/*
 * sha1_hash.cpp
 *
 *  Created on: 13.09.2012
 *      Author: ruslan
 */

/*
 * SHA1_HASH.cpp
 *
 *  Created on: 29.08.2012
 *      Author: ruslan
 */

#include "types.h"
#include <stdio.h>

namespace dinosaur
{

SHA1_HASH::SHA1_HASH()
{
	clear();
}

SHA1_HASH::SHA1_HASH(const SHA1_HASH & hash)
{
	memcpy(this->m_data, hash.m_data, SHA1_LENGTH);
	to_hex();
}

SHA1_HASH::SHA1_HASH(const char * hash)
{
	memcpy(this->m_data, hash, SHA1_LENGTH);
	to_hex();
}

SHA1_HASH::SHA1_HASH(const unsigned char * hash)
{
	memcpy(this->m_data, hash, SHA1_LENGTH);
	to_hex();
}

SHA1_HASH::~SHA1_HASH()
{

}

SHA1_HASH & SHA1_HASH::operator=(const SHA1_HASH & hash)
{
	if (this != &hash)
	{
		memcpy(this->m_data, hash.m_data, SHA1_LENGTH);
		to_hex();
	}
	return *this;
}

SHA1_HASH & SHA1_HASH::operator=(const char * hash)
{
	memcpy(this->m_data, hash, SHA1_LENGTH);
	to_hex();
	return *this;
}

SHA1_HASH & SHA1_HASH::operator=(const unsigned char * hash)
{
	memcpy(this->m_data, hash, SHA1_LENGTH);
	to_hex();
	return *this;
}

unsigned char & SHA1_HASH::operator[](size_t i)
{
	if (i >= SHA1_LENGTH)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);
	return m_data[i];
}

const unsigned char & SHA1_HASH::operator[](size_t i) const
{
	if (i >= SHA1_LENGTH)
		throw Exception(Exception::ERR_CODE_INVALID_OPERATION);
	return m_data[i];
}

bool SHA1_HASH::operator<(const SHA1_HASH & hash) const
{
	for(size_t i = 0; i < SHA1_LENGTH; i++)
	{
		if (m_data[i] != hash.m_data[i])
			return m_data[i] < hash.m_data[i];
	}
	return false;
}

bool SHA1_HASH::operator==(const SHA1_HASH & hash) const
{
	return memcmp(m_data, hash.m_data, SHA1_LENGTH) == 0;
}

bool SHA1_HASH::operator!=(const SHA1_HASH & hash) const
{
	return memcmp(m_data, hash.m_data, SHA1_LENGTH) != 0;
}

void SHA1_HASH::copy2(char * dst) const
{
	memcpy(dst, m_data, SHA1_LENGTH);
}

void SHA1_HASH::copy2(unsigned char * dst) const
{
	memcpy(dst, m_data, SHA1_LENGTH);
}

void SHA1_HASH::to_hex()
{
	for( size_t i = 0; i < SHA1_LENGTH; i++ )
		sprintf( &m_hex[ i * 2 ], "%02x", m_data[ i ] );
	m_hex[ SHA1_LENGTH * 2 ] = 0;
}

void SHA1_HASH::clear()
{
	memset(m_data, 0, SHA1_LENGTH);
	memset(m_hex, 0, SHA1_LENGTH * 2 + 1);
}

const char * SHA1_HASH::hex() const
{
	return m_hex;
}

}



