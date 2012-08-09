/*
 * domain_name_resolver.cpp
 *
 *  Created on: 05.07.2012
 *      Author: ruslan
 */

#include "network.h"

namespace dinosaur {
namespace network
{

DomainNameResolver::DomainNameResolver(std::string & domain, struct sockaddr_in * addr)
{
#ifdef BITTORRENT_DEBUG
	printf("DomainNameResolver constructor %s\n", domain.c_str());
#endif
	if (addr == NULL || domain.length() == 0)
		throw Exception("Invalid argument");
	m_domain = domain;
	m_addr = addr;
	if (resolve() != ERR_NO_ERROR)
			throw Exception("Can not resolve");
}

DomainNameResolver::DomainNameResolver(const char * domain, struct sockaddr_in * addr)
{
#ifdef BITTORRENT_DEBUG
	printf("DomainNameResolver constructor %s\n", domain);
#endif
	if (addr == NULL || domain == NULL)
		throw Exception("Invalid argument");
	m_domain = domain;
	m_addr = addr;
	if (resolve() != ERR_NO_ERROR)
		throw Exception("Can not resolve");
}

DomainNameResolver::~DomainNameResolver()
{
#ifdef BITTORRENT_DEBUG
	printf("DomainNameResolver destructor\n");
#endif
}

int DomainNameResolver::addrinfo()
{
	struct addrinfo * res = NULL;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	//резолвим ipшник хоста
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	if (getaddrinfo(m_domain.c_str(), NULL, &hints, &res) == 0)
	{
		uint16_t port = m_addr->sin_port;
		memcpy(m_addr, res->ai_addr, sizeof(struct sockaddr_in));
		m_addr->sin_port = port;
		freeaddrinfo(res);
#ifdef BITTORRENT_DEBUG
	printf("DomainNameResolver name %s resolved %s\n", m_domain.c_str(), inet_ntoa(m_addr->sin_addr));
#endif
		return ERR_NO_ERROR;
	}
	else
	{
		//freeaddrinfo(res);
		return ERR_INTERNAL;
	}
}

int DomainNameResolver::resolve()
{
	if (addrinfo() != ERR_NO_ERROR)
	{
		if (split_domain_and_port() != ERR_NO_ERROR)
			return ERR_INTERNAL;
		return addrinfo();
	}
	return ERR_NO_ERROR;
}

int DomainNameResolver::split_domain_and_port()
{
	pcre *re;
	const char * error;
	int erroffset;
	char pattern2[] = "^(.+?):(\\d+)$";
	re = pcre_compile(pattern2, 0, &error, &erroffset, NULL);
	if (!re)
		return ERR_INTERNAL;

	int ovector[1024];
	int count = pcre_exec(re, NULL, m_domain.c_str(), m_domain.length(), 0, 0, ovector, 1024);
	if (count <= 0)
	{
		pcre_free(re);
		return ERR_INTERNAL;
	}
	pcre_free(re);
	std::string port_str = m_domain.substr(ovector[4], ovector[5] - ovector[4]);
	m_domain = m_domain.substr(0, ovector[3] - ovector[2]);

	unsigned int p;
	sscanf(port_str.c_str(), "%u", &p);
	m_addr->sin_port = htons(p);
	return ERR_NO_ERROR;
}

}
}
