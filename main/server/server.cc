#include "server.hh"

using boost::asio::ip::udp;
using boost::asio::ip::tcp;

server::server(
		std::string& port,
		const size_t fifo_size,
		const size_t fifo_low_watermark,
		const size_t fifo_high_watermark,
		const size_t buf_len,
		const size_t tx_interval
		)
	:	
		io_service_(),
		signals_(io_service_),
		//=============================//
		// Inicjalizacja TCP-owych     //
		//=============================//
		acceptor_(io_service_),
		connection_manager_(new connection_manager()),
		tcp_socket_(io_service_),
		clientid_(8),
		raport_timer_period_(1000),
		raport_timer_(io_service_, 
			boost::posix_time::milliseconds(raport_timer_period_)),
		//=============================//
		// Inicjalizacja UDP-owych     //
		//=============================//
		udp_socket_(io_service_, 
			udp::endpoint(udp::v4(), boost::lexical_cast<short>(port))),
		udp_resolver_(io_service_),
		current_datagram_nr_(0),
		port_(port),
		fifo_size_(fifo_size),
		fifo_low_watermark_(fifo_low_watermark),
		fifo_high_watermark_(fifo_high_watermark),
		buf_len_(buf_len),
		tx_interval_(tx_interval),
		mixer_timer_(io_service_, 
			boost::posix_time::milliseconds(tx_interval_)),
		factory_(),
		write_buf_(new char[CLIENT_BUFFER_LEN]),
		udp_dgram_timers_period_(1000)
{	
	//std::cout << "Jestem w konstruktorze!\n";
	//=======================================================================//
	// Rejestracja sygnałów.                                                 //
	//=======================================================================//
	// Zarejestruj obsługę sygnału kończącego proces servera.
	signals_.add(SIGINT);

	//=======================================================================//
	// Zegarek na sygnał.                                                    //
	//=======================================================================//
	// Asynchronicznie oczekuj na sygnał kończący
	do_await_stop();

	//=======================================================================//
	// TCP.                                                                  //
	//=======================================================================//
	// Korzystając z resolvera ustal lokalny endpoint.
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(port);
	// Otwórz akceptor z opcją ponownego użycia adresu.
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();
	// Zacznij akceptować przychodzące połączenia TCP.
	do_accept();
	// Zacznij raportować.
	//run_raport_timer();

	//=======================================================================//
	// UDP.                                                                  //
	//=======================================================================//
	// Zacznij nasłuchiwać na gnieździe UDP.
	do_receive();
	// Miksowanie czas zacząć!
	do_mixery();
}

server::~server()
{
	// Powyrzucaj zegarki z mapy dgramów do śmieci.
	udp_dgram_timers_.clear();
	// Usuń dane z bufora odczytującego dane po UDP.
	std::fill(read_buf_.begin(), read_buf_.end(), 0);
	// Usuń dane wszystkich klientów, które są przechowywane po stronie serwera
	client_data_map_.clear();
	// Usuń dane o endpointach i identyfikatorach.
	client_map_.clear();
	// Zwolnij zarządcę połączeń.
	delete connection_manager_;
	// Zwolnij pamięć wykorzystywaną przez bufor z wymieszaną wiadomością.
	delete write_buf_;
}

void server::do_receive()
{
	udp_socket_.async_receive_from(
		boost::asio::buffer(read_buf_, CLIENT_BUFFER_LEN),
		sender_endpoint_,
		[this](boost::system::error_code error, size_t bytes_transferred) {
			if (!error) {
				// Podziel datagram na nagłówek i ,,body'' wiadomości.
				std::string whole_message;
				std::copy(
					read_buf_.begin(), 
					read_buf_.begin() + bytes_transferred,
					std::back_inserter(whole_message)
				);
				// Stwórz odpowiednią strukturę wiadomości.
				message_structure msg_structure = 
					message_converter::divide_msg_into_sections(
						whole_message, bytes_transferred
					);
				// Spróbuj wywnioskować rodzaj datagramu, po nagłówku.
				/*std::cerr << "------------------------------------------------\n";
				std::cerr << "Dostałem wiadomość! Serwer.\n";
				std::cerr << "whole msg: " << whole_message << "\n";
				std::cerr << "header: " << msg_structure._header << "\n";
				std::cerr << "------------------------------------------------\n";*/
				try {
					base_header* header = 
						factory_.match_header(
							headerline_parser::get_data(
								msg_structure._header
							)
						);
					// Po udanej konwersji obsłuż akcję związaną z tym dgramem.
					do_manage_msg(header, msg_structure._body);			
				} catch (invalid_header_exception& ex) {
					std::cerr << "Do receive: " << ex.what() << " \n";
				}
			} else {
				std::cerr << "Do receive: " << error << "\n";
				do_receive();
			}
		}
	);
}

void server::do_manage_msg(base_header* header, std::string& body)
{
	// Zweryfikuj rodzaj komunikatu na podstawie nagłówka.
	const std::string header_name = std::move(header->_header_name);
	/*std::cerr << "-----------------------------------------\n";
	std::cerr << "Jestem w Managerze wiadomości.\n";
	std::cerr << "Nagłówek: " << header_name << "\n";
	std::cerr << "Body: " << body << "\n";
	std::cerr << "-----------------------------------------\n";*/
	
	// Skopiuj obiekt endpointa nadawcy.
	boost::asio::ip::udp::endpoint remote_udp_endpoint(sender_endpoint_);

	if (header_name == CLIENT) {
		std::shared_ptr<client_header> c_header(
			dynamic_cast<client_header*>(header)
		);
		// Ściągnij identyfikator klienta.
		size_t clientid = c_header->_client_id;
		// Na podstawie id klienta ustal endpoint TCP.
		// Zapisz endpoint do stringa.
		boost::asio::ip::tcp::endpoint remote_tcp_endpoint(
			connection_manager_->get_tcp_endpoint(clientid)
		);
		const std::string client_str_tcp_endpoint =
			convert_remote_tcp_endpoint_to_string(remote_tcp_endpoint);
		// Zbuduj strukturę danych dla klienta.
		client_data* c_data = new client_data(
			fifo_size_ / 2,
			client_str_tcp_endpoint,
			remote_udp_endpoint,
			fifo_high_watermark_,
			fifo_low_watermark_,
			buf_len_
		);
		// Zaktualizuj mapy.
		// 1)
		client_map_.insert(
			std::make_pair(
				remote_udp_endpoint,
				clientid
			)
		);
		// 2)
		client_data_map_.insert(
			std::make_pair(
				clientid,
				c_data
			)
		);
		// 3)
		udp_dgram_timers_.insert(
			std::make_pair(
				clientid,
				new boost::asio::deadline_timer(
					io_service_,
					boost::posix_time::milliseconds(udp_dgram_timers_period_)
				)
			)
		);
		do_receive();
	} else if (header_name == RETRANSMIT) {
		std::shared_ptr<retransmit_header> r_header(
			dynamic_cast<retransmit_header*>(header)
		);
		// Ściągnij identyfikator klienta na podstawie endpointa.
		const size_t client_id = client_map_[remote_udp_endpoint];
		// Ściągnij nr, od którego klient prosi o retransmisję.
		size_t retransmit_inf_nr = r_header->_nr;
		// Ściągnij wszystkie dgramy zapamiętane w buforze rezerwowym
		// spełniające kryterium wskazane przez klienta.
		std::list<std::string> dgram_list = 
			client_data_map_[client_id]->get_last_dgrams(retransmit_inf_nr);
	/*	std::cerr << "--------------------------------------------------\n";
		std::cerr << "Retransmisje\n";
		std::cerr << "Rozmiar dgram_list: " << dgram_list.size() << "\n";
		std::cerr << "\n";*/
		// Przejdź do transmitowania i jednocześnie w tryb nasłuchiwania.
		//
		// Retransmisja:
		// 1) Utwórz inteligentny wskaźnik na kopię aktualnego endpointa.
		std::shared_ptr<boost::asio::ip::udp::endpoint> remote_endpoint(
			new boost::asio::ip::udp::endpoint(sender_endpoint_)
		);
		// Wznów komunikację po gnieździe UDP.
		do_receive();
		// Wywołaj funkcję retransmisji z parametrami:
		// -> aktualnego endpointa,
		// -> listą datagramów
		do_transmit_data(remote_endpoint, dgram_list);
	} else if (header_name == UPLOAD) {
		std::shared_ptr<upload_header> u_header(
			dynamic_cast<upload_header*>(header)
		);
		// Skonwertuj dane z postaci string na wektor liczb 16-bit ze znakiem.
		std::vector<std::int16_t> data = 
			string_converter::to_vector_int16(body);
		// Ściągnij identyfikator klienta.
		const size_t clientid = client_map_[remote_udp_endpoint];
		const size_t client_dgram_nr = u_header->_nr;
		const bool decision = 
			client_dgram_nr == client_data_map_[clientid]->get_last_dgram_nr();
		if (decision) {
			// Zaktualizuj dane po uploadzie w mapie danych klienta.
			client_data_map_[clientid]->actualize_content_after_upload(data);
			// Numer kolejnego oczekiwanego datagramu.
			const size_t next_ack = client_dgram_nr + 1;
			// Rozmiar kolejki.
			const size_t available_size = 
				client_data_map_[clientid]->get_available_size();
			// Wyślij datagram potwierdzający.
			do_send_ack_datagram(
				next_ack, 
				available_size,
				remote_udp_endpoint
			);
		} else {
			do_receive();
		}
	} else if (header_name == KEEPALIVE) {
		// odznacz jakoś czas ostatniej rozmowy.
		// Zacznij odliczać czas dla konkretnego klienta od początku.
		/*run_udp_dgram_timer(
				std::shared_ptr<boost::asio::ip::udp::endpoint>(
					new boost::asio::ip::udp::endpoint(remote_udp_endpoint)
				)
			);*/
		do_receive();
	}
}

void server::do_send_ack_datagram(const size_t ack, const size_t free_space,
	boost::asio::ip::udp::endpoint remote_endpoint)
{
	/*std::cerr << "------------------------------------------\n";
	std::cerr << "Wysyłanie ACK\n";
	std::cerr << "Adres: " << remote_endpoint << "\n";
	std::cerr << "ack: " << ack << "\n";
	std::cerr << "wolne miejsce w kolejce: " << free_space << "\n";
	std::cerr << "------------------------------------------\n";*/
	std::shared_ptr<std::string> ack_dgram(
		new std::string(
			"ACK " +
			std::to_string(ack) +
			" " +
			std::to_string(free_space) +
			"\n"
		)
	);
	udp_socket_.async_send_to(
		boost::asio::buffer(*ack_dgram, ack_dgram->size()),
		sender_endpoint_,
		[this, ack, free_space, remote_endpoint, ack_dgram](
			boost::system::error_code error, 
			size_t bytes_transferred
		) {
			if (error) {
				std::cerr << "Do send ack datagram: " << error << " \n";
				// Jeżeli nie udało się przesłać potwierdzenia,
				// to spróbuj jeszcze raz.
				do_send_ack_datagram(ack, free_space, remote_endpoint);
			} else {
				do_receive();
			}
		}
	);
}

void server::do_transmit_data(
	std::shared_ptr<boost::asio::ip::udp::endpoint> remote_endpoint,
	std::list<std::string>& dgram_list)
{
	/*std::cerr << "----------------------------------------------\n";
	std::cerr << "Transmisja danych\n";
	std::cerr << "Adres: " << *remote_endpoint << "\n";
	std::cerr << "----------------------------------------------\n";*/
	for (auto it = dgram_list.begin(); it != dgram_list.end(); ++it) {
		std::shared_ptr<std::string> dgram(new std::string(*it));
		udp_socket_.async_send_to(
			boost::asio::buffer(*dgram), 
			*remote_endpoint,
			[this](
				boost::system::error_code error, 
				size_t bytes_transferred
			) {
				if (error) {
					std::cerr << "Do transmit data: " << error << "\n";
				}
				do_receive();
			}
		);
	}
}


//===========================================================================//
//                                                                           //
// Rozrusznik IO-SERVICE.                                                    //
//                                                                           //
//===========================================================================//
void server::run()
{
	// Rozpocznij działanie io-serwisu, 
	// który jest pośrednikiem w komunikacji z systemem operacyjnym.
	// IO-SERWIS asynchronicznie blokuje działanie procesu 
	// do momentu wykonania wszystkich przydzielonych prac.
	io_service_.run();
}


//===========================================================================//
//                                                                           //
// Oczekiwanie na sygnał kończący działanie serwera.                         //
//                                                                           //
//===========================================================================//
void server::do_await_stop()
{
	signals_.async_wait(
		[this](boost::system::error_code error, size_t signal_code) {
			// Zostają najpierw wycofane wszystkie niezrealizowane 
			// operacje asynchroniczne. Nstępuje wtedy zamknięcie io-service'u.
			acceptor_.close();
			connection_manager_->stop_all();
		}
	);
}

std::string server::convert_remote_tcp_endpoint_to_string(
	boost::asio::ip::tcp::endpoint remote_tcp_endpoint)
{
	std::ostringstream stream;
	stream << remote_tcp_endpoint;

	return stream.str();
}

//===========================================================================//
//                                                                           //
// TCP.                                                                      //
//                                                                           //
//===========================================================================//
void server::do_accept()
{
	//std::cout << "No i akceptuję\n";
	acceptor_.async_accept(
		tcp_socket_,
		[this](boost::system::error_code error) {
			// Sprawdź, czy serwer został zatrzymany przez sygnał 
			// jeszcze przed rozpoczęciem tego handlera.
			if (!acceptor_.is_open()) {
				return;
			}
			if (!error) {
				connection_manager_->start(
					std::make_shared<connection>(
						std::move(tcp_socket_),
						*connection_manager_,
						get_next_clientid()
					),
					this
				);
			} else {
				std::cerr << "Do async accept: " << error << "\n";
			}
			do_accept();
		}
	);
}

size_t server::get_next_clientid()
{
	return ++clientid_;
}

void server::send_raport()
{
	raport_ = "\n";
	for (auto it = client_map_.begin(); it != client_map_.end(); ++it) {
		const size_t idx = it->second;
		raport_ += client_data_map_[idx]->get_statistics();
	}

	//std::cerr << "Wysyłam raport:" << raport_ << "\n";
	connection_manager_->send_raport(std::move(raport_));
	run_raport_timer();
}


//===========================================================================//
//                                                                           //
// Mixernia.                                                                 //
//                                                                           //
//===========================================================================//
void server::mixer()
{
	/*std::cerr << "-------------------------------------------------\n";
	std::cerr << "Jestem w mixerze.\n";*/
	// Skompletuj dane zanim zostaną one przekazane do mixera:
	std::list<size_t> active_queue_clients_id;
	size_t inputs_size = 0;
	// Wyznacz listę id tych klientów, których kolejki są w stanie ACTIVE:
	for (auto it = client_map_.begin(); it != client_map_.end(); ++it) {
		const size_t clientid = it->second;
		queue_state state = 
			client_data_map_[clientid]->rate_queue_state();

		if (state == queue_state::ACTIVE) {
			active_queue_clients_id.push_back(it->second);
			++inputs_size;
		}
	}
	//std::cerr << "Mamy " << active_queue_clients_id.size() << " aktywnych kolejek\n";
	// Zbuduj strukturę mixer_input*.
	mixer_input* inputs = new mixer_input[inputs_size];
	int next = 0;
	for (auto it = active_queue_clients_id.begin(); 
		it != active_queue_clients_id.end(); ++it) {
		const size_t idx = *it;

		inputs[next].len = 2 * client_data_map_[idx]->get_queue_size();
		inputs[next].consumed = 0;
		inputs[next].data = static_cast<void*>(
			&(client_data_map_[idx]->get_client_msg_queue())
		);
		next++;
	} next = 0;
	// Rzutuj bufor wyjściowy na void*:
	void* output_buf = static_cast<void*>(write_buf_);
	// Wyznacz rozmiar wektora w bajtach:
	size_t output_size = (size_t)CLIENT_BUFFER_LEN;
	// Podaj częstotliwość wywołania mixera po skonwertowaniu na pożądany typ:
	unsigned long tx_interval_ms = static_cast<unsigned long>(tx_interval_);
	// Poproś miksera, aby wymieszał co trzeba:
	mixer::mix(
		&inputs[0],
		inputs_size,
		output_buf,
		&output_size,
		tx_interval_ms
	);
	// Zaktualizuj dane wszystkich wyżej rozpatrzonych klientów:
	//std::cerr << "Mixowanie zakończone\n";
	for (auto it = active_queue_clients_id.begin(); 
		it != active_queue_clients_id.end(); ++it) {
		const size_t idx = *it;

		client_data_map_[idx]->actualize_content_after_mixery(
			inputs[next].consumed
		);
	}

	std::string message(write_buf_, output_size);
	//std::cerr << "--------------------------------------------------------\n";
	/*
	std::cerr << "-----------------------------------------\n";
	std::cerr << "Mixernia.\n";
	std::cerr << "Rozmiar_danych: " << message.size() << "\n";
	std::cerr << "Dane: " << message << "\n";
	std::cerr << "-----------------------------------------\n";
	*/

	// Wyślij do każdego z klientów wynik mixera:
	for(auto it = client_data_map_.begin(); 
		it != client_data_map_.end(); ++it) {

		boost::asio::ip::udp::endpoint receiver_endpoint(
			it->second->udp_endpoint_
		);

		std::string datagram(
			"DATA " +
			boost::lexical_cast<std::string>(current_datagram_nr_) +
			" " +
			boost::lexical_cast<std::string>(it->second->get_last_dgram_nr()) +
			" " +
			boost::lexical_cast<std::string>(
				it->second->get_available_size()
			) +
			"\n" +
			message
		);
		// Wrzuć datagram do kolejki każdemu aktywnego klienta, 
		// któremu zamierzasz go wysłać.
		const size_t clientid = it->first;
		const size_t cnr = current_datagram_nr_;
		client_data_map_[clientid]->add_to_dgrams_list(datagram, cnr);
		// Prześlij datagram klientowi.
		udp_socket_.async_send_to(
			boost::asio::buffer(datagram, CLIENT_BUFFER_LEN), 
			receiver_endpoint,
			[this](boost::system::error_code error, size_t bytes_transferred) {
				if (!error) {
					// Świetnie, poszło.
				} else {
					std::cerr << "Send mixed data: " << error << "\n";
				}
			}
		);
	} current_datagram_nr_++;
	delete[] inputs;
	// Rozpocznij oczekiwanie na kolejne wywołanie miksera:
	do_mixery();
}


//===========================================================================//
//                                                                           //
// Zegarek do miksowania.                                                    //
//                                                                           //
//===========================================================================//
void server::do_mixery()
{
	mixer_timer_.expires_at(
		mixer_timer_.expires_at() +
		boost::posix_time::milliseconds(tx_interval_)
	);
	mixer_timer_.async_wait(
		boost::bind(&server::mixer, this)
	);
}


//===========================================================================//
//                                                                           //
// Zegarek do raportowania.                                                  //
//                                                                           //
//===========================================================================//
void server::run_raport_timer()
{
	// Zacznij wysyłać raporty
	raport_timer_.expires_at(
		raport_timer_.expires_at() + 
		boost::posix_time::milliseconds(raport_timer_period_)
	);

	raport_timer_.async_wait(
		[this](boost::system::error_code error) {
			if (error) {
				std::cerr << "Run raport timer: " << error << " \n";
			} else {
				send_raport();
			}
		}
	);
}


//===========================================================================//
//                                                                           //
// Zegarek odmierzający czas od otrzymania                                   //           
// ostatniego dgramu konkretnego klienta po UDP.                             //
//                                                                           //
//===========================================================================//
void server::run_udp_dgram_timer(
	std::shared_ptr<boost::asio::ip::udp::endpoint> client_endpt)
{
	//std::cerr << "Jestem w tym zegarku\n";
	const size_t clientid = client_map_[*client_endpt];
	auto it = udp_dgram_timers_.find(clientid);
	if (it != udp_dgram_timers_.end()) {
		it->second->expires_at(
			it->second->expires_at() +
			boost::posix_time::milliseconds(udp_dgram_timers_period_)
		);

		it->second->async_wait(
			[this, clientid, client_endpt](boost::system::error_code error) {
				if (error == boost::asio::error::operation_aborted) {
					// Jeżeli otrzymaliśmy komunikat KEEPALIVE
					// w przeciągu #udp_dgram_timers_period_ milisekund,
					// to wszystko jest OK i rozpoczynamy mierzyć czas od początku.
					run_udp_dgram_timer(client_endpt);
				} else {
					// W.p.p, gdy oczekujemy na odpowiedź zbyt długo:
					// -> zwolnij zasoby klienta przekazywane po UDP
					// -> nie przerywaj połączenia po TCP(same raporty).
					std::cerr << "Usuwam zasoby nadane po UDP przez klienta: " 
					<< clientid << ".\n";
					free_resources(clientid);
				}
			}
		);
	}
}


void server::free_resources(const size_t clientid)
{
	const size_t clid = clientid;
	// Iterator wskazujący na parę <clientid, struktura dla klienta>.
	auto it = client_data_map_.find(clid);
	// Usuń krotkę <client_udp_endpoint, client_id>
	client_map_.erase(it->second->udp_endpoint_);
	// Usuń obiekt wskazywany przez iterator
	client_data_map_.erase(it);
	// Usuń zegarek 'od dgramów' dla konkretnego klienta.
	udp_dgram_timers_.erase(clientid);
}