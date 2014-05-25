#ifndef MESSAGE_CONVERTER_HPP
#define MESSAGE_CONVERTER_HPP

#include <string>

#include "../common/structures.hpp"

class message_converter
{
public:
	static message_structure divide_msg_into_sections(const std::string& msg);	
};
#endif