#include "connection.hh"


#include "../connection_manager/connection_manager.hh"

connection::connection(
	boost::asio::ip::tcp::socket socket, 
	connection_manager& manager, 
	const size_t clientid)
:	socket_(std::move(socket)),
	connection_manager_(manager),
	clientid_(clientid)
{
}

void connection::start()
{
	//std::cerr << "Jestem w connection: SZAŁ!\n";
	do_write_clientid();
}

void connection::stop()
{
	socket_.close();
}

void connection::send(const std::string& raport)
{
	do_write_raport(raport);
}

void connection::do_write_clientid()
{
	auto self(shared_from_this());
	// Skonstruuj wiadomość dla klienta:
	std::string msg(
		"CLIENT " +
		std::to_string(clientid_) +
		"\n"
	);
	// Wyślij asynchronicznie identyfikator:
	boost::asio::async_write(
		socket_,
		boost::asio::buffer(std::move(msg)),
		[this, self](boost::system::error_code error, std::size_t) {
			if (error) {
				connection_manager_.stop(clientid_, shared_from_this());
			}
		}
	);
}

void connection::do_write_raport(const std::string& raport)
{
	auto self(shared_from_this());
	// Wyznacz długość raportu:
	const size_t raport_size = raport.size();
	// Oblicz długość napisu przedstawiającego długość raportu:
	const size_t raport_length = get_length(raport_size);
	// Wyznacz długość raportu:
	size_t msg_size = 
		raport_size /* Rozmiar treści raportu bez nagłówka */ +
		std::to_string(raport_size).size() +
		1 /* Znak nowej linii po długości wiadomości */;
	// Oblicz długość napisu przedstawiającego długość raportu:
	size_t msg_length = get_length(msg_size);
	// Porównaj obie długości napisów.
	// Mogą się różnić maksymalnie o 1 dlatego, 
	// że liczba opisująca długość tekstu będzie zawsze niedłuższa 
	// od tekstu opisywanego.
	if (msg_length > raport_length) {
		msg_size++;
	}

	std::string msg(
		std::to_string(msg_size) /* Rozmiar wiadomości wraz z nagłówkiem */ +
		'\n' /* Znak nowej linii następujący po długości raportu */ +
		raport
	);

	//std::cerr << "Wysyłam klientowi wiadomość: " << msg << "\n";// o rozmiarze: " << header.size() << "\n";
	boost::asio::async_write(
		socket_,
		boost::asio::buffer(std::move(msg)),
		boost::asio::transfer_at_least(msg_size),
		[this, self](boost::system::error_code error, std::size_t) {
			if (error) {
				//std::cerr << "Nastąpił błąd w kliencie: " << clientid_ << "\n";
				//std::cerr << "----------------------------------------\n";
				connection_manager_.stop(clientid_, shared_from_this());
			}
		}
	);
}

size_t connection::get_clientid()
{
	return clientid_;
}

size_t connection::get_length(size_t body)
{
	size_t counter = 0;
	while (body) {
		++counter;
		body /= 10;
	}

	return counter;
}