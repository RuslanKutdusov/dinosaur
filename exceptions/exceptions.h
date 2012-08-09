#pragma once
#include <iostream>
#include <string>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>


namespace dinosaur {
class Exception
{
protected:
	std::string m_str;
public:
	Exception(void)
		: m_str("")
	{}
	virtual ~Exception(void)
	{
	}
	Exception(std::string str)
		: m_str(str)
	{}
	const std::string & get_error() const
	{
		return m_str;
	}
	void print()
	{
		std::cout<<"Exception: "<<m_str<<std::endl;
	}
};

class NetworkException : public Exception
{
public:
	NetworkException(int err)
		: Exception(sys_errlist[err])
	{
	}
	NetworkException(std::string str)
			: Exception(str)
	{
	}
};

class FileException : public Exception
{
public:
	FileException(int err)
		: Exception(sys_errlist[err])
	{
	}
	FileException(std::string str)
			: Exception(str)
	{
	}
};

class NoneCriticalException
{
protected:
	std::string m_str;
public:
	NoneCriticalException(void)
		: m_str("")
	{}
	virtual ~NoneCriticalException(void)
	{
	}
	NoneCriticalException(std::string str)
		: m_str(str)
	{}
	void print()
	{
		std::cout<<"None critical exception (internal error): "<<m_str<<std::endl;
	}
};
}
