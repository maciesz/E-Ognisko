#include "message_converter.hpp"

message_structure message_converter::divide_msg_into_sections(
	const std::string& msg)
{
	// Znajdź pierwszą pozycję znaku nowej linii wyznaczającej
	// koniec nagłówka:
	const size_t new_line_pos = msg.find(std::string("\n"));
	std::string header = std::string(msg, 0, new_line_pos - 1);
	std::string body = std::string(msg, new_line_pos + 1, 
	bytes_transferred - new_line_pos);

	return message_structure(header, body);
}