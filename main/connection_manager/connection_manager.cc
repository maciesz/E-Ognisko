#include "connection_manager.hh"

connection_manager::connection_manager()
{
}

void connection_manager::start(connection_ptr connect)
{
	//std::cerr << "Jestem w managerze połączeń i będę akceptował" << "\n";
	connections_map_.insert(std::make_pair(connect->get_clientid(), connect));
	connect->start();
}

void connection_manager::stop(const size_t clientid, connection_ptr connect)
{
	connections_map_.erase(clientid);
	connect->stop();
}

void connection_manager::send_raport(const std::string& raport)
{
	//std::cerr << "Wysyłam wszystkim następujący string: " << raport << "\n";
	// Wyślij raport każdemu klientowi, z którym jesteśmy połączeni po TCP.
	for (auto it = connections_map_.begin(); 
		it != connections_map_.end(); ++it) {
		// Połącznie:
		it->second->send(raport);
	}
}

void connection_manager::stop_all()
{
	for (auto it = connections_map_.begin(); 
		it != connections_map_.end(); ++it) {

		it->second->stop();
	}
	connections_map_.clear();
}
