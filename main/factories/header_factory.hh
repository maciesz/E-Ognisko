#ifndef HEADER_FACTORY_HH
#define HEADER_FACTORY_HH

#include <map>
#include <utility>
#include <iostream>

#include "../common/structures.hh"
#include "../exceptions/invalid_header_exception.hh"

class header_factory
{
public:
	header_factory();

	base_header* match_header(const header_data& data);
private:
	std::map<std::string, base_header*> factory;
};
#endif
