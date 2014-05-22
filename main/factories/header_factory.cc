#include "header_factory.hpp"

header_factory::header_factory()
{
	factory.insert(std::make_pair(std::string("CLIENT"), client_header()));
	factory.insert(std::make_pair(std::string("UPLOAD"), upload_header()));
	factory.insert(std::make_pair(std::string("DATA"), data_header()));
	factory.insert(std::make_pair(std::string("ACK"), ack_header()));
	factory.insert(std::make_pair(std::string("RETRANSMIT"), retransmit_header()));
	factory.insert(std::make_pair(std::string("KEEPALIVE"), keepalive_header()));
}

base_header header_factory::match_header(const header_data& data)
{
	auto it = factory.find(data._header_name);
	// Jeżeli początek nagłówka jest poprawnie zdefiniowany
	if (it != factory.end()) {
		// Stwórz instancję obiektu odpowiedniego nagłówka
		upload_header header;// TODO: = it->second.get_instance();
		// Uwaga:
		// W przypadku niewłaściwej liczby parametrów
		// dla określonego typu nagłówka zostaje zgłoszony wyjątek:
		//
		// invalid_header_exception
		// 
		// Przekazując ten wyjątek dalej wskazujemy na ,,kłopotliwe połączenie"
		header.set_parametres(data);
		return header;
	} else 
		throw invalid_header_exception();
}
