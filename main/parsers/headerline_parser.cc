#include "headerline_parser.hpp"

header_data headerline_parser::get_data(const std::string& headerline_s)
{
	// Skonwertuj na stringa
	//std::string headerline_s(headerline);
	// Deklaracja wektora tymczasowo przechowującego dane
	std::vector<std::string> args;
	// Wypełnienie wektora poszczególnymi komponentami z nagłówka
	boost::split(args, headerline_s, boost::algorithm::is_any_of(" \t\n"));
	// Jeżeli nagłówek jest pusty, to jest nieprawidłowy
	if (!args.size())
		throw invalid_header_exception();
	// Deklaracja listy wynikowej
	std::vector<boost::uint16_t> param_list;
	// Konwersja wszystkich dodatkowych parametrów na liczbowe
	const int size = static_cast<int>(args.size());
	for (int i = 1; i< size; ++i) {
		param_list.push_back(boost::lexical_cast<boost::uint16_t>(args[i]));
	}
	// Wyznaczenie nazwy głównej nagłówka
	const std::string header_name = args[0];
	// Zamiatamy po sobie
	args.clear();
	// Zwracamy obiekt z informacjami
	return header_data(header_name, param_list);
}
