#include "dht.h"

namespace dinosaur{
namespace dht{

searcher::searcher(SHA1_HASH infohash, routing_tablePtr & rt, message_sender * ms)
{
	m_rt = rt;
	m_ms = ms;
	node_id infohash_as_id(&infohash[0]);
	node_list close_nodes;
	m_rt->get_closest_ids(infohash_as_id, close_nodes);
	m_fronts.push_back(close_nodes);
}

searcher::~searcher()
{

}

}
}
