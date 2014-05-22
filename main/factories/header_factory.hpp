#ifndef HEADER_FACTORY_HPP
#define HEADER_FACTORY_HPP

#include <boost/cstdint.hpp>

#include <map>
#include <utility>
#include <iostream>

#include "../common/structures.hpp"
#include "../exceptions/invalid_header_exception.hpp"

class header_factory
{
public:
	header_factory();

	base_header* match_header(const header_data& data);
private:
	std::map<std::string, base_header*> factory;
};
#endif
