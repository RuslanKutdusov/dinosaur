/*
 * domain_name_resolver.cpp
 *
 *  Created on: 05.07.2012
 *      Author: ruslan
 */

#include "network.h"
#include <pcre.h>
#include <netdb.h>

namespace dinosaur {
namespace network {

DomainNameResolver::DomainNameResolver(const std::string & domain, sockaddr_in * addr)
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "DomainNameResolver constructor " << domain.c_str();
#endif
	if (addr == NULL || domain.length() == 0)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	m_domain = domain;
	m_addr = addr;
	resolve();
}

DomainNameResolver::DomainNameResolver(const char * domain, sockaddr_in * addr)
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "DomainNameResolver constructor " << domain;
#endif
	if (addr == NULL || domain == NULL)
		throw Exception(Exception::ERR_CODE_NULL_REF);
	m_domain = domain;
	m_addr = addr;
	resolve();
}

DomainNameResolver::~DomainNameResolver()
{
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "DomainNameResolver destructor";
#endif
}

void DomainNameResolver::addrinfo()
{
	struct addrinfo * res = NULL;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	//резолвим ipшник хоста
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	if (getaddrinfo(m_domain.c_str(), NULL, &hints, &res) != 0)
		throw Exception(Exception::ERR_CODE_CAN_NOT_RESOLVE);

	uint16_t port = m_addr->sin_port;
	memcpy(m_addr, res->ai_addr, sizeof(struct sockaddr_in));
	m_addr->sin_port = port;
	freeaddrinfo(res);
#ifdef BITTORRENT_DEBUG
	logger::LOGGER() << "DomainNameResolver name " << m_domain.c_str() << " is resolved " << inet_ntoa(m_addr->sin_addr);
#endif
}

void DomainNameResolver::resolve()
{
	try
	{
		addrinfo();
	}
	catch(Exception & e)
	{
		split_domain_and_port();
		addrinfo();
	}
}

void DomainNameResolver::split_domain_and_port()
{
	pcre *re;
	const char * error;
	int erroffset;
	char pattern2[] = "^(.+?):(\\d+)$";
	re = pcre_compile(pattern2, 0, &error, &erroffset, NULL);
	if (!re)
		throw Exception(Exception::ERR_CODE_CAN_NOT_RESOLVE);

	int ovector[1024];
	int count = pcre_exec(re, NULL, m_domain.c_str(), m_domain.length(), 0, 0, ovector, 1024);
	if (count <= 0)
	{
		pcre_free(re);
		throw Exception(Exception::ERR_CODE_CAN_NOT_RESOLVE);
	}
	pcre_free(re);
	std::string port_str = m_domain.substr(ovector[4], ovector[5] - ovector[4]);
	m_domain = m_domain.substr(0, ovector[3] - ovector[2]);

	unsigned int p;
	sscanf(port_str.c_str(), "%u", &p);
	m_addr->sin_port = htons(p);
}

}
}
