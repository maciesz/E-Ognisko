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
	boost::asio::async_write(
		socket_,
		boost::asio::buffer(raport),//std::move(msg)),
		boost::asio::transfer_at_least(raport_size),
		[this, self](boost::system::error_code error, std::size_t) {
			if (error) {
				connection_manager_.stop(clientid_, shared_from_this());
			}
		}
	);
}

size_t connection::get_clientid()
{
	return clientid_;
}
