/*
 * state_serializator.h
 *
 *  Created on: 12.07.2012
 *      Author: ruslan
 */

#ifndef STATE_SERIALIZATOR_H_
#define STATE_SERIALIZATOR_H_

#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <stdint.h>
#include <string.h>
#include <string>

namespace torrent
{

class StateSerializator
{
private:
	class serializable
	{
	public:
		uint64_t uploaded;
		std::string download_directory;
		unsigned char * bitfield;
		size_t bitfield_len;
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & uploaded;
			ar & download_directory;
			for(size_t i = 0; i < bitfield_len; i++)
				ar & bitfield[i];
		}
		serializable()
		{
			bitfield_len = 0;
			bitfield = NULL;
		}
		serializable(size_t _bitfield_len)
		{
			bitfield_len = _bitfield_len;
			bitfield = new unsigned char[bitfield_len];
			memset(bitfield, 0, bitfield_len);
		}
		~serializable()
		{
			if (bitfield != NULL)
				delete[] bitfield;
		}
		void init_bitfield(size_t _bitfield_len)
		{
			bitfield_len = _bitfield_len;
			bitfield = new unsigned char[bitfield_len];
			memset(bitfield, 0, bitfield_len);
		}
		void set_data(uint64_t _uploaded, const std::string & _download_directory, unsigned char * _bitfield, size_t _bitfield_len)
		{
			uploaded = _uploaded;
			download_directory = _download_directory;
			if (bitfield == NULL)
			{
				bitfield_len = _bitfield_len;
				bitfield = new unsigned char[bitfield_len];
				memset(bitfield, 0, bitfield_len);
			}
			memcpy(bitfield, _bitfield, bitfield_len);
		}
	};
	serializable s;
	std::string state_filename;
public:
	StateSerializator(std::string _state_filename)
	{
		state_filename = _state_filename;
	}
	~StateSerializator(){}
	int serialize(uint64_t _uploaded, const std::string & _download_directory, unsigned char * _bitfield, size_t _bitfield_len)
	{
		std::ofstream ofs(state_filename.c_str());
		if (ofs.fail())
			return -1;
		s.set_data(_uploaded, _download_directory, _bitfield, _bitfield_len);
		{
			boost::archive::text_oarchive oa(ofs);
			oa << s;
		}
		return 0;
	}
	int deserialize(uint64_t & _uploaded, std::string & _download_directory, unsigned char * _bitfield, size_t _bitfield_len)
	{
		std::ifstream ifs(state_filename.c_str());
		s.init_bitfield(_bitfield_len);
		if (ifs.fail())
			return -1;
		{
			boost::archive::text_iarchive ia(ifs);
			ia >> s;
		}
		_uploaded = s.uploaded;
		_download_directory = s.download_directory;
		memcpy(_bitfield, s.bitfield, _bitfield_len);
		return 0;
	}
};

}
#endif /* STATE_SERIALIZATOR_H_ */
