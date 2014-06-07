#ifndef HEADERLINE_PARSER_HH
#define HEADERLINE_PARSER_HH

#include <cstdint>
#include <cstring>
#include <vector>
#include <list>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "../common/structures.hh"
#include "../exceptions/invalid_header_exception.hh"

class headerline_parser
{
public:
	static header_data get_data(const std::string& headerline);
};
#endif
