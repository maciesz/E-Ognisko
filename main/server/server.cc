#include "server.hpp"

server::server(
		boost::asio::io_service& io_service,
		boost::uint16_t port, 
		boost::uint16_t fifo_size,
		boost::uint16_t fifo_low_watermark, 
		boost::uint16_t fifo_high_watermark,
		boost::uint16_t buf_len,
		boost::uint16_t tx_interval
	)
:
	//tcp_socket_(io_service),
	//udp_socket_(io_service),
	
	tcp_acceptor_(io_service),
	tcp_resolver_(io_service),
	udp_resolver_(io_service),
	header_buf_(CLIENT_BUF_LEN),
	body_buf_(CLIENT_BUF_LEN),
	raport_buf_(CLIENT_BUF_LEN),
	next_clientid_(0),
	current_datagram_nr_(0),
	mixer_buf_(std::vector<boost::int16_t>(0, fifo_size)),
	mixed_msg_size_(MULTIPLIER * tx_interval),
	factory_(), // TODO: sprawdzić, czy oby na pewno ok?
	fifo_size_(fifo_size),
	fifo_low_watermark_(fifo_low_watermark),
	fifo_high_watermark_(fifo_high_watermark),
	port_(port),
	buf_len_(buf_len),
	tx_interval_(tx_interval),
	mixer_timer_(io_service),
	raport_timer_(io_service),
	mixer_(), // TODO: to samo co wyżej
	client_map_(),
	client_data_map_()
{
	write_buf_(0, CLIENT_BUF_LEN);

	//============================//
	// TCP                        //
	//============================//
	// Ustal endpoint TCP serwera:
	boost::asio::ip::tcp::resolver::query tcp_query(port_);
	boost::asio::ip::tcp::resolver::iterator tcp_endpoint_iterator = 
		tcp_resolver_.resolve(tcp_query);
	// Zainicjuj gniazdo do komunikacji po TCP:
	tcp_socket_(io_service, tcp_endpoint_iterator);
	// Zacznij akceptować połączenia:
	acceptor_.async_accept(tcp_socket_,
		boost::bind(&server::pass_clientid, this, 
			boost::asio::placeholders::error));

	//============================//
	// UDP.                       //
	//============================//
	// Ustal endpoint UDP
	boost::asio::ip::udp::resolver::query udp_query(port_);
	boost::asio::ip::udp::resolver::iterator udp_endpoint_iterator =
		udp_resolver_.resolve(udp_query);
	// Przygotuj gniazdo do komunikacji po UDP:
	udp_socket_(io_service, udp_endpoint_iterator);
	// Zacznij nasłuchiwać na gnieździe UDP:
	udp_socket_.async_receive_from(
		boost::asio::buffer(read_buf_), udp_remote_endpoint_,
		boost::bind(&server::handle_udp_message, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
		);

	//============================//
	// Raportowanie.              //
	//============================//
	raport_timer_.expires_from_now(
		boost::posix_time::seconds(1)
	);
	raport_timer_.async_wait(
		boost::bind(&server::send_raport_info, this)
	);

	//============================//
	// Mixerowanie.               //
	//============================//
	handle_next_mixery();
}

void server::handle_next_mixery()
{
	mixer_timer_.expires_from_now(
		boost::posix_time::milliseconds(tx_interval_)
	);
	mixer_timer_.async_wait(
		boost::bind(&server::mixer, this)
	);
}

void server::mixer()
{
	// Skompletuj dane zanim zostaną one przekazane do mixera:
	std::list<boost::uint32_t> active_queue_clients_id;
	size_t inputs_size = 0;
	// Wyznacz listę id tych klientów, których kolejki są w stanie ACTIVE:
	for (auto it = client_map_.begin(); it != client_map_.end(); ++it) {
		const boost::uint32_t clientid = client_map_[it->second];
		queue_state state = client_data_map_[clientid].rate_queue_state();

		if (state == queue_state::ACTIVE) {
			active_queue_clients_id.push_back(it->second);
			++inputs_size;
		}
	}
	// Zbuduj strukturę mixer_input*
	mixer_input* inputs = new mixer_input[inputs_size];

	int next = 0;
	for (auto it = active_queue_clients_id.begin(); 
		it != active_queue_clients_id.end() ++it) {

		inputs[next].len = 2 * client_data_map_[*it].get_queue_size();
		inputs[next].consumed = 0;
		inputs[next].data = static_cast<void*>(
			client_data_map_[*it].get_client_msg_queue()
		);
		next++;
	} next = 0;
	// Rzutuj bufor wyjściowy na void*:
	void* output_buf = static_cast<void*>(&write_buf_);
	// Wyznacz rozmiar wektora w bajtach:
	size_t output_size = write_buf_.size() * 2;
	// Podaj częstotliwość wywołania mixera po skonwertowaniu na pożądany typ:
	unsigned long tx_interval_ms = static_cast<unsigned long>(tx_interval_);
	// Poproś miksera, aby wymieszał co trzeba:
	mixer_.mixer(
		&inputs[0],
		inputs_size,
		output_buf,
		&output_size,
		tx_interval_ms
	);
	// Zaktualizuj dane wszystkich wyżej rozpatrzonych klientów:
	for (auto it = active_queue_clients_id.begin(); 
		it != active_queue_clients_id.end() ++it) {
		client_data_map_[*it].actualize_content_after_mixery(inputs[next].consumed);
	}
	// Wyślij do każdego z klientów wynik mixera:
	for(auto it = client_data_map_.begin(); 
		it != client_data_map_.end(); ++it) {

		boost::asio::ip::udp::endpoint receiver_endpoint(
			it->second->remote_endpoint
		);

		udp_socket_.async_sendto(
			boost::asio::buffer(write_buf_),
			receiver_endpoint
		);
	}
}

Sprawdź server::send_raport_info()
{
	std::string final_raport = std::string("\n");
	// TODO: Wybierz tylko tych, z którymi połączenie się nie urwało.
	//
	// Założenie TYMCZASOWE: Wszyscy są wciąż aktywni ;-).
	for(auto it = client_map_.begin(); it != client_map_.end(); ++it) {
		final_raport += client_data_map_[it->second].get_statistics();
	}

	boost::asio::async_write(tcp_socket_, 
		boost::asio::buffer(final_raport),
		boost::bind(
			&server::handle_after_send_raport, this,
			boost::asio::placeholders::error
		)
	);
}

void server::handle_after_send_raport(const boost::system::error_code& error)
{
	if (!error) {
		raport_timer_.expires_from_now(
			boost::posix_time::seconds(1)
		);
		raport_timer_.async_wait(
			boost::bind(&server::send_raport_info, this)
		);
	} else {
		std::cerr << "Handle after send raport\n";
	}
}

void server::handle_udp_message(const boost::system::error_code& error,
	const size_t bytes_transferred)
{
	if (!error) {
		// Po odebraniu danych od klienta mamy jeden komunikat,
		// niepodzielony na nagłówek i ciało wiadomości:
		std::string whole_message;
		std::copy(read_buf_.begin(), read_buf_.begin() + bytes_transferred, 
		std::back_inserter(whole_message));

		const message_structure msg_structure = 
			message_converter::divide_msg_into_sections(message);

		try {
			base_header* header = 
				factory_.match_header(headerline_parser::get_data(
					msg_structure.header
					)
				);
			manage_read_header(header, msg_structure.body);
		} catch (invalid_header_exception& ex) {
			std::cerr << "Handle read datagram\n";
		}
	} else {
		std::cerr << "Handle UDP message\n";
	}
}

void server::manage_read_header(base_header* header, const std::string& body)
{
	if (header->_header_name == CLIENT) {
		boost::shared_ptr<client_header> c_header(
			dynamic_cast<client_header*>(header)
		);

		// Ściągnij identyfikator klienta:
		boost::uint32_t client_id = c_header->_client_id;

		// Zapisz endpoint w postaci stringa:
		const std::string client_str_udp_endpoint =
			convert_remote_udp_endpoint_to_string();
		// Zbuduj strukturę danych dla klienta:
		client_data data(
			fifo_size_, 
			client_str_udp_endpoint, 
			udp_remote_endpoint_, 
			fifo_high_watermark, 
			fifo_low_watermark, 
			buf_len_
		);

		// Zaktualizuj obie mapy:
		// 1) client_map_:
		client_map_.insert(
			std::make_pair(
				client_str_udp_endpoint, 
				_client_id
			)
		);
		// 2) client_data_map_:
		client_data_map_.insert(std::make_pair(_client_id, data));
	} else if (header->_header_name == RETRANSMIT) {
		boost::shared_ptr<retransmit_header> r_header(
			dynamic_cast<retransmit_header*>(header)
		);

		// Zapisz endpoint w postaci stringa:
		std::string client_str_udp_endpoint =
			convert_remote_udp_endpoint_to_string();
		// Ściągnij identyfikator klienta na podstawie endpointa:
		boost::uint32_t client_id = client_map_[client_str_udp_endpoint];
		// Ściągnij nr, od którego klient prosi o retransmisję:
		retransmit_inf_nr = header->_nr;
		// Ściągnij wszystkie dgramy zapamiętane w buforze rezerwowym
		// spełniające kryterium wskazane przez klienta:
		std::list<std::string> dgram_list = 
			client_data_map_[client_id].get_last_dgrams(retransmit_inf_nr);

		// Przejdź do transmitowania i jednocześnie w tryb nasłuchiwania:
		//
		// Retransmisja:
		// 1) Utwórz inteligentny wskaźnik na kopię aktualnego endpointa:
		boost::shared_ptr<boost::asio::ip:udp::endpoint> remote_endpoint(
			new boost::asio::ip::udp::endpoint(udp_remote_endpoint_)
		);
		// Wywołaj funkcję retransmisji z parametrami:
		// -> aktualnego endpointa,
		// -> listą datagramów
		boost::bind(&server::handle_data_transmission, this,
			remote_endpoint, dgram_list);
	} else if (header->_header_name == UPLOAD) {
		boost::shared_ptr<upload_header> u_header(
			dynamic_cast<upload_header*>(header)
		);
		// Skonwertuj stringa na wektor liczb 16-bitowych ze znakiem:
		std::vector<boost::int16_t> body_vector = 
			string_converter::to_vector_int16(body);
		// Zapisz endpoint w postaci stringa:
		const std::string client_str_udp_endpoint =
			convert_remote_udp_endpoint_to_string();
		// Ściągnij identyfikator klienta:
		boost::uint32_t clientid = client_map_[client_str_udp_endpoint];
		// Zaktualizuj dane po uploadzie w mapie danych klienta:
		client_data_map_[clientid].actualize_content_after_upload(body_vector);
	} else if (header->_header_name == KEEPALIVE) {
		// Odznacz jakoś czas ostatniej rozmowy:
	}
	// Zaktualizuj czas odwiedzenia:
	// ... TODO ...

	// Przejdź w tryb nasłuchiwania:
	boost::bind(&server::handle_receive_udp, this,
		boost::asio::placeholders::error);
}

void server::handle_receive_udp(const boost::system::error_code& error)
{
	if (!error) {
		// Zacznij nasłuchiwać na gnieździe UDP:
		udp_socket_.async_receive_from(
			boost::asio::buffer(read_buf_), udp_remote_endpoint_,
			boost::bind(&server::handle_udp_message, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)
		);
	} else {
		std::cerr << "Handle receive udp\n";
	}
}

void server::handle_data_transmission(
	boost::shared_ptr<boost::asio::ip::udp::endpoint> remote_endpoint,
	std::list<std::string>& dgram_list,
	const boost::system::error_code& error)
{
	if (!error) {
		const size_t dgram_list_size = dgram_list.size();
		if (dgram_list_size > 0) {
			boost::shared_ptr<std::string> dgram(dgram_list.back());
			dgram_list.pop_back();
			udp_socket_.async_send_to(
				boost::asio::buffer(*dgram), *remote_endpoint,
				boost::bind(
					&server::handle_data_transmission, this,
					remote_endpoint, dgram_list,
					boost::asio::placeholders::error
				)
			);
		} else {
			boost::bind(&server::handle_receive_udp, this,
				boost::asio::placeholders::error);
		}
	} else {
		std::cerr << "Handle data transmission\n";
	}
}

void server::start_accepting_on_tcp(const boost::system::error_code& error)
{
	if (!error) {
		acceptor_.async_accept(tcp_socket_,
			boost::bind(&server::pass_clientid, this, 
				boost::asio::placeholders::error));
	} else {
		std::cerr << "Start accepting on TCP\n";
	}
}

void server::pass_clientid(const boost::)
{
	if (!error) {
		std::string msg(
			std::string("CLIENT ") +
			boost::lexical_cast<std::string>(get_next_clientid) +
			"\n"
		);

		boost::asio::async_write(tcp_socket_, 
			boost::asio::buffer(msg),
			boost::bind(&server::start_accepting_on_tcp, this,
				boost::asio::placeholders_error));
	} else {
		std::cerr << "Pass clientid\n";
	}
}

server::~server()
{
	// Wyczyść zawartość bufora mixującego
	mixer_buf_.clear();

	// Usuń dane wszystkich klientów, które są przechowywane po stronie serwera
	client_data_map_.clear();

	// Usuń dane o endpointach i identyfikatorach
	client_map_.clear();	
}

std::string server::convert_remote_udp_endpoint_to_string()
{
	std::ostringstream stream;
	stream << udp_remote_endpoint_;
	
	return stream.str();
}

