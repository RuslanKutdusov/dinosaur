/*
 * log.h
 *
 *  Created on: 27.08.2012
 *      Author: ruslan
 */

#ifndef LOG_H_
#define LOG_H_
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace dinosaur
{
namespace logger
{

void InitLogger(const std::string & log_dir);
std::ostream & LOGGER();

}
}
#endif /* LOG_H_ */
