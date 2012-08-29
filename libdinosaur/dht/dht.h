/*
 * dht.h
 *
 *  Created on: 28.08.2012
 *      Author: ruslan
 */

#ifndef DHT_H_
#define DHT_H_

#include "../consts.h"
#include "../types.h"
#include <map>

namespace dinosaur
{
namespace dht
{

class node_id
{
private:
	unsigned char m_data[NODE_ID_LENGHT];
	void clear();
public:
	node_id();
	unsigned char & operator[](size_t i);
	const unsigned char & operator[](size_t i) const;
	virtual ~node_id();
};

node_id distance(const node_id & n1, const node_id & n2);

}
}


#endif /* DHT_H_ */
