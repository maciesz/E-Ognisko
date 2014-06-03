#ifndef MESSAGE_CONVERTER_HH
#define MESSAGE_CONVERTER_HH

#include <string>

#include "../common/structures.hh"

class message_converter
{
public:
	static message_structure divide_msg_into_sections(
		std::string& msg,
		size_t bytes_transferred);	
};
#endif
