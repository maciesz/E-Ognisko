#ifndef HEADERLINE_PARSER_HPP
#define HEADERLINE_PARSER_HPP

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast/hpp>
#include <boost/cstdint.hpp>

#include <cstring>
#include <vector>
#include <list>
#include <iostream>

#include "../exceptions/invalid_header_exception.hpp"

class headerline_parser
{
public:
	static header_data get_data(const char* headerline);
};
#endif