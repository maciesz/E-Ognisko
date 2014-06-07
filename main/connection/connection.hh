# ifndef CONNECTION_HH
# define CONNECTION_HH

#include <array>
#include <memory>
#include <utility>
#include <string>
#include <vector>

#include <boost/asio.hpp>

class connection_manager;

/// Reprezentuje połączenie z klientem.
class connection : public std::enable_shared_from_this<connection>
{
public:
	/// Tworzy połącznie z konkretnym gniazdem.
	/// Explicit konstruktor zapobiega niejawej konwersji argumentów.
	explicit connection(
		boost::asio::ip::tcp::socket socket,
		connection_manager& manager, 
		const size_t clientid
	);

	/// Uniemożliwiają kopiowanie konkretnego obiektu klasy connection.
	connection(const connection&) = delete;
	connection& operator=(const connection&) = delete;

	/// Rozpoczyna pierwszą asynchroniczną operację dla połączenia.
	void start();

	/// Zatrzymuje wszystkie asynchroniczne operacje związane z połączeniem.
	void stop();

	/// Wysyła raport do klienta.
	void send(const std::string&);

	/// Przekaż id klienta.
	size_t get_clientid();
	
	/// Gniazdo TCP do komunikacji.
	boost::asio::ip::tcp::socket socket_;
private:
	/// Wykonuje asynchroniczną operację odczytu danych od klienta.
	void do_read();

	/// Wykonuje asynchroniczna operację przekazania klientowi jego id.
	void do_write_clientid();

	/// Wykonuje asynchroniczną operację przekazania klientowi raportu.
	void do_write_raport(const std::string&);

	/// Zwraca długość napisu dla podanej długości tekstu.
	size_t get_length(size_t length);

	/// Manager połączeń po TCP.
	connection_manager& connection_manager_;

	/// Identyfikator klienta na tym połączeniu.
	const size_t clientid_;
};

typedef std::shared_ptr<connection> connection_ptr;
# endif 
