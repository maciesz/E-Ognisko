#ifndef STRING_CONVERTER_HH
#define STRING_CONVERTER_HH

#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

class string_converter
{
public:
	static std::vector<std::int16_t> to_vector_int16(std::string& body);
};
#endif
