#include "message_converter.hh"

message_structure message_converter::divide_msg_into_sections(
	std::string& msg, size_t bytes_transferred)
{
	//std::cerr << "Otrzymałem wiadomość: " << msg << "\n";
	// Znajdź pierwszą pozycję znaku nowej linii wyznaczającej
	// koniec nagłówka:
	const size_t new_line_pos = msg.find(std::string("\n"));
	std::string header = std::string(msg, 0, new_line_pos);
	//std::cerr << "Header: " << header << "\n";
	std::string body = std::string(msg, new_line_pos + 1, 
	bytes_transferred - new_line_pos);

	/*std::cerr << "Po konwersji: [header]: " << header << "\n";
	std::cerr << "\t [body]: " << body << "\n";*/
	return message_structure(header, body);
}
