#ifndef CONNECTION_MANAGER_HH
#define CONNECTION_MANAGER_HH

#include <map>

#include "../connection/connection.hh"
#include "../server/server.hh"

class server;
/// Zarządza otwartymi połączeniami TCP.
class connection_manager
{
public:
	/// Konstruktor managera połączeń.
	connection_manager();
	/// Destruktor managera połączeń.
	~connection_manager();
	/// Zablokowanie możliwości kopiowania.
	//connection_manager(const connection_manager&) = delete;
	connection_manager& operator=(const connection_manager&) = delete;
	/// Dodaj konkretne połączenie i wyślij identyfikator klientowi.
	void start(connection_ptr connect, server* server);
	/// Zatrzymaj konkretne połącznie.
	void stop(const size_t clientid_, connection_ptr connect);
	/// Zatrzymaj wszystkie połącznia.
	void stop_all();
	/// Wyślij raport do wszystkich klientów.
	void send_raport(const std::string& raport);
	/// Uzyskaj kopię endpointa tcp.
	boost::asio::ip::tcp::endpoint get_tcp_endpoint(const size_t clientid);
private:
	/// Mapa połączeń: [klucz, wartość] = <clientid, connection_ptr>
	std::map<size_t, connection_ptr> connections_map_;	
	/// Wskaźnik na serwer.
	server* server_;
};	
#endif
