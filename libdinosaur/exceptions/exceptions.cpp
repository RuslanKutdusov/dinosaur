/*
 * exceptions.cpp
 *
 *  Created on: 22.01.2012
 *      Author: ruslan
 */

#include "exceptions.h"

namespace dinosaur {

const std::string exception_errcode2str(Exception::ERR_CODES err)
{
	ERR_CODES_STR ecs;
	return ecs.str_code(err);
}

const std::string exception_errcode2str(Exception & e)
{
	ERR_CODES_STR ecs;
	return ecs.str_code(e.get_errcode());
}

}
