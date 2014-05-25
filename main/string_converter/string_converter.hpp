#ifndef STRING_CONVERTER_HPP
#define STRING_CONVERTER_HPP

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>

#include <vector>

class string_converter
{
public:
	static std::vector<boost::int16_t> to_vector_int16(std::string& body);
};
#endif