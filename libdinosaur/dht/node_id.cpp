#include "dht.h"

//
namespace dinosaur
{
namespace dht
{


//
node_id distance( const node_id& id1, const node_id& id2 )
{
	node_id r;
	for( size_t i = 0; i < NODE_ID_LENGTH; i++ )
	{
		r[i] = id1[i] ^ id2[i];
	}
	return r;
}


//
node_id generate_random_node_id()
{
	node_id ret;
	generate_random_node_id( ret );
	return ret;
}


//
void generate_random_node_id( node_id& id )
{
	/*for( size_t i = 0; i < NODE_ID_LENGTH; i++ )
	{
		id[i] = rand() % 256 ;
	}*/
	CSHA1 csha1;
	timeval tv;
	gettimeofday( &tv, NULL );
	csha1.Update( ( unsigned char* )&tv, sizeof( timeval ) );
	csha1.Final();
	csha1.GetHash( id );
}


//
size_t get_bucket( const node_id& id1, const node_id& id2 )
{
	size_t ret = 0;

	for( size_t i = 0; i < NODE_ID_LENGTH; i++ )
	{
		unsigned char t = id1[i] ^ id2[i];
		if( t == 0 )
			break;

		bool breakThisLoop = false;

		for( int b = 7; b >= 0; --b )
		{
			if( t& ( 1 << b ) )
			{
				ret++;
			}
			else
			{
				breakThisLoop = true;
				break;
			}
		}

		if( breakThisLoop )
			break;
	}
	return ret;
}


//
node_id::node_id()
 	 : SHA1_HASH()
{

}


//
node_id::node_id( const node_id& id )
	: SHA1_HASH( id )
{

}


//
node_id::node_id( const char* id )
	: SHA1_HASH( id )
{

}


//
node_id::node_id( const unsigned char* id )
	: SHA1_HASH( id )
{

}


//
node_id::~node_id()
{

}


//
const node_id node_id::operator ^( const node_id& id )
{
	return distance(*this, id );
}

}
}
