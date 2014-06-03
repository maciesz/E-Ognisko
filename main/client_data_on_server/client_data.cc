#include "client_data.hh"

client_data::client_data(
		size_t max_queue_length, 
		const std::string& endpoint,
		const boost::asio::ip::udp::endpoint udp_endpoint,
		std::uint16_t fifo_high_watermark,
		std::uint16_t fifo_low_watermark,
		std::uint16_t buf_len
	)
:
	udp_endpoint_(udp_endpoint),
	max_queue_length_(max_queue_length),
	endpoint_(endpoint),
	fifo_high_watermark_(fifo_high_watermark),
	fifo_low_watermark_(fifo_low_watermark),
	last_dgrams_max_size_(buf_len),
	queue_size_(static_cast<std::int16_t>(0)),
	min_bytes_(static_cast<std::int16_t>(0)),
	max_bytes_(static_cast<std::int16_t>(0)),
	expected_client_datagram_nr_(static_cast<std::uint32_t>(0))
{
	queue_.reserve(max_queue_length_);
}

client_data::~client_data()
{
	queue_.clear();
}

void client_data::actualize_content_after_mixery(
	std::uint16_t bytes_transferred)
{
	// Usuń shift pierwszych elementów z kolejki:
	std::uint16_t shift = bytes_transferred / 2;
	pop_front_sequence(shift);
	// Zaktualizuj rozmiar kolejki:
	queue_size_ = queue_.size();
	// Zaktualizuj minimalną ilość bytów w kolejce:
	min_bytes_ = std::min(min_bytes_, queue_size_);
}

void client_data::actualize_content_after_upload(
	std::vector<std::int16_t>& upload_data)
{
	//std::cerr << "AKTUALIZUJĘ: " << expected_client_datagram_nr_ + 1 << " raz.\n";
	const size_t upload_data_size = upload_data.size();
	// Jeżeli po złączeniu kolejek, długość tej nowopowstałej
	// będzie dłuższa od dopuszczalnej przyjętej w konstruktorze to:
	if (queue_size_ + upload_data_size > max_queue_length_) {
		// -> usuń nadmiar z początku kolejki:
		std::uint16_t shift = 
			max_queue_length_ - (queue_size_ + upload_data_size);
		pop_front_sequence(shift);
	}
	// Niezależnie od przypadku zmerguj queue_ z kolejką z UPLOAD'a:
	queue_.insert(queue_.end(), upload_data.begin(), upload_data.end());
	// Zaktualizuj rozmiar kolejki:
	queue_size_ = queue_.size();
	// Zaktualizuj maksymalną liczbę bajtów w kolejce:
	max_bytes_ = std::max(max_bytes_, queue_size_);
	// Zaktualizuj nr kolejnego oczekiwanego datagramu ze strony klienta.
	actualize_last_dgram_nr();
}

std::string client_data::get_statistics()
{
	// Przygotuj raport jednostkowy:
	// (bo tylko obiektu przechowującego dane konkretnego klienta)
	std::string statistics(std::string(
		endpoint_ /* Endpoint klienta */ +
		" FIFO: " +
		boost::lexical_cast<std::string>(queue_size_) /* Rozmiar kolejki */ +
		"/" +
		boost::lexical_cast<std::string>(max_queue_length_) /* Max rozmiar */ +
		" (min. " +
		boost::lexical_cast<std::string>(min_bytes_) +
		", max. " +
		boost::lexical_cast<std::string>(max_bytes_) +
		")\n"
	));

	// Automatycznie zresetuj statystyki:
	reset_statistics();

	// Przekaż raport dalej
	return statistics;
}

queue_state client_data::rate_queue_state()
{
	if (queue_size_ >= fifo_high_watermark_)
		return queue_state::ACTIVE;
	else if (queue_size_ <= fifo_low_watermark_)
		return queue_state::FILLING;

	return queue_state::INCOMPLETE;
}

void client_data::pop_front_sequence(std::uint16_t shift)
{
	std::vector<decltype(queue_)::value_type>(
		queue_.begin() + shift, queue_.end()
	).swap(queue_);
}

void client_data::reset_statistics()
{
	// Zainicjuj min_bytes_ i max_bytes_ aktualnym rozmiarem kolejki:
	min_bytes_ = static_cast<std::uint16_t>(queue_size_);
	max_bytes_ = static_cast<std::uint16_t>(queue_size_);
}

void client_data::add_to_dgrams_list(
	const std::string& dgram_msg, std::uint32_t dgram_nr)
{
	last_dgrams_.push_back(retransmit_dgram(dgram_msg, dgram_nr));
	if (last_dgrams_.size() > last_dgrams_max_size_)
		last_dgrams_.pop_front();
}

std::vector<std::int16_t>& client_data::get_client_msg_queue()
{
	return queue_;
}


std::list<std::string> client_data::get_last_dgrams(const std::uint32_t inf_nr)
{
	std::list<std::string> ret_list;
	for (auto it = last_dgrams_.begin(); it != last_dgrams_.end(); ++it) {
		if (it->_dgram_nr >= inf_nr)
			ret_list.push_back(it->_message);
	}

	return ret_list;
}

size_t client_data::get_queue_size()
{
	return static_cast<size_t>(queue_size_);
}

size_t client_data::get_available_size()
{
	return static_cast<size_t>(max_queue_length_ - queue_size_);
}

std::uint32_t client_data::get_last_dgram_nr()
{
	return expected_client_datagram_nr_;
}

void client_data::actualize_last_dgram_nr()
{
	++expected_client_datagram_nr_;
}
