#include "headerline_parser.hh"

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
	std::vector<std::uint32_t> param_list;
	// Konwersja wszystkich dodatkowych parametrów na liczbowe
	const int size = static_cast<int>(args.size());
	for (int i = 1; i< size; ++i) {
		try {
			param_list.push_back(boost::lexical_cast<std::uint32_t>(args[i]));
		} catch (boost::bad_lexical_cast &) {
			// Jeśli zdarzy się taki bad_cast, to znaczy, że datagram nie został w pełni przekazany:
			throw invalid_header_exception();
			/*std::cerr << "----------------------------------------------\n";
			std::cerr << "Bad_lexical_cast headerline_parser.cc/get_data\n";
			std::cerr << "Castowany: " << args[i] << "na uint32_t\n";
			std::cerr << "Cały headerline: " << headerline_s << "\n";
			std::cerr << "\n";
			for (int i = 0; i< size; ++i) {
				std::cerr << "args[" << i << "]: " << args[i] << "\n";
			}
			std::cerr << "----------------------------------------------\n";*/

			break;
		}
	}
	// Wyznaczenie nazwy głównej nagłówka
	const std::string header_name = args[0];
	// Zamiatamy po sobie
	args.clear();
	// Zwracamy obiekt z informacjami
	return header_data(header_name, param_list);
}
