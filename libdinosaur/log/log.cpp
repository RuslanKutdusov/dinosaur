/*
 * log.cpp
 *
 *  Created on: 27.08.2012
 *      Author: ruslan
 */

#include "log.h"

namespace dinosaur
{
namespace logger
{

using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace std;
ofstream ofs;
ostream  * log_stream = &cout;

void InitLogger(const std::string & log_dir)
{
	string fname = log_dir;
	fname += "dinosaur.";
	fname +=  to_simple_string(microsec_clock::local_time());
	ofs.open(fname.c_str());
	ofs << "Log created at: " << to_simple_string(microsec_clock::local_time());
	log_stream = &ofs;
}

ostream & LOGGER()
{
    *log_stream << endl << "[ PID=" << getpid() << " " << to_simple_string(microsec_clock::local_time()) <<" ] ";
    return *log_stream;
}

}
}
