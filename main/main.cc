#include <exception>
#include <iostream>
#include <string>
#include <cstring>

#include "common/structures.hpp"
#include "parsers/headerline_parser.hpp"
#include "factories/header_factory.hpp"

void build_correct(std::vector<std::string>& correct_communicates)
{
	correct_communicates.push_back("CLIENT 1312");
	correct_communicates.push_back("UPLOAD 1312");
	correct_communicates.push_back("DATA 1312 213 321");
	correct_communicates.push_back("ACK 1312 1234");
	correct_communicates.push_back("RETRANSMIT 3213");
	correct_communicates.push_back("KEEPALIVE");
}

void build_incorrect(std::vector<std::string>& correct_communicates)
{
	correct_communicates.push_back("CLIENT 1312 932");
	correct_communicates.push_back("UPLOAD 1312 54");
	correct_communicates.push_back("DATA 1312 213");
	correct_communicates.push_back("ACK 1312 1234 6");
	correct_communicates.push_back("RETRANSMIT 321 912");
	correct_communicates.push_back("KEEPALIVE 3");
}

void build_incorrect_with_letters(std::vector<std::string>& correct_communicates)
{
	correct_communicates.push_back("CLIENT 1312 93&2");
	correct_communicates.push_back("UPLOAD 131#2 54");
	correct_communicates.push_back("DATA 131#2 2$13");
	correct_communicates.push_back("ACK 1312 1234@ 6");
	correct_communicates.push_back("RETRANSMIT 321 912*");
	correct_communicates.push_back("KEEPALIVE 3^");
}

void preprocess(std::vector<std::string>& communicates)
{
	header_factory fact;
	headerline_parser header_parser;
	int counter = 0;
	const int size = static_cast<int>(communicates.size());
	for (int i = 0; i< size; ++i) {
		try {
			// String to char*
			std::string str = communicates[i]; /*
			// Przeparsuj linijkę na konkretne dane*/
			const char* headerline = str.c_str();
			const header_data data = headerline_parser::get_data(headerline);
			//delete[] headerline;
			// Dopasuj odpowiedni header
			/*std::cout << "Cokolwiek\n";*/
			base_header* header = fact.match_header(data);
			// Wypisz nazwę headera

			if (header->_header_name == std::string("UPLOAD")) {
				std::cout << "Nazwa headera: " << header->_header_name << "\n";
				upload_header* u_header = dynamic_cast<upload_header*>(header);
				std::cout << "Nr: " << (u_header->_nr) << "\n";
				delete u_header;
			}
		} catch (std::exception& ex) {
			++counter;
		}
	}

	std::cout << "Łącznie złapaliśmy: " << counter << " zgłoszonych wyjątków\n";
}

int main()
{
	// Poprawne komunikaty
	std::vector<std::string> correct_communicates;
	// Niepoprawne komunikaty z liczbami
	std::vector<std::string> incorrect_communicates;
	// Niepoprawne komunikaty z literałami
	std::vector<std::string> incorrect_with_letters;

	build_correct(correct_communicates);
	build_incorrect(incorrect_communicates);
	build_incorrect_with_letters(incorrect_with_letters);

	preprocess(correct_communicates);
	std::cout << "---------------------------------------------------------\n";
	preprocess(incorrect_communicates);
	std::cout << "---------------------------------------------------------\n";
	preprocess(incorrect_with_letters);
	std::cout << "---------------------------------------------------------\n";

	return 0;
}
