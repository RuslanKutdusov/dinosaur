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

namespace dinosaur
{

SHA1_HASH::SHA1_HASH()
{
	clear();
}

SHA1_HASH::SHA1_HASH(const SHA1_HASH & hash)
{
	memcpy(this->m_data, hash.m_data, SHA1_LENGTH);
}

SHA1_HASH::SHA1_HASH(const char * hash)
{
	memcpy(this->m_data, hash, SHA1_LENGTH);
}

SHA1_HASH::SHA1_HASH(const unsigned char * hash)
{
	memcpy(this->m_data, hash, SHA1_LENGTH);
}

SHA1_HASH::~SHA1_HASH()
{

}

SHA1_HASH & SHA1_HASH::operator=(const SHA1_HASH & hash)
{
	if (this != &hash)
	{
		memcpy(this->m_data, hash.m_data, SHA1_LENGTH);
	}
	return *this;
}

SHA1_HASH & SHA1_HASH::operator=(const char * hash)
{
	memcpy(this->m_data, hash, SHA1_LENGTH);
	return *this;
}

SHA1_HASH & SHA1_HASH::operator=(const unsigned char * hash)
{
	memcpy(this->m_data, hash, SHA1_LENGTH);
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

void SHA1_HASH::print()  const
{
	for(size_t i = 0; i < SHA1_LENGTH; i++)
		printf("%02X", m_data[i]);
	printf("\n");
}

void SHA1_HASH::to_hex(std::string & id) const
{
	id.resize(SHA1_LENGTH * 2);
	for(size_t i = 0; i < SHA1_LENGTH; i++)
		sprintf(&id[i * 2], "%02x", m_data[i]);
}

void SHA1_HASH::clear()
{
	memset(m_data, 0, SHA1_LENGTH);
}

}



