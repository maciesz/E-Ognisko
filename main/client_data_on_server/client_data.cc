#include "client_data.hpp"

client_data::client_data(
		size_t max_queue_length, 
		const std::string& endpoint, 
		const boost::uint16_t fifo_high_watermark,
		const boost::uint16_t fifo_low_watermark
	)
:
	max_queue_length_(max_queue_length),
	endpoint_(endpoint),
	fifo_high_watermark_(fifo_high_watermark),
	fifo_low_watermark_(fifo_low_watermark),
	queue_size_(static_cast<boost::int16_t>(0)),
	min_bytes_(static_cast<boost::int16_t>(0)),
	max_bytes_(static_cast<boost::int16_t>(0))
{
	queue_.reserve(max_queue_length_);
}

client_data::~client_data()
{
	queue_.clear();
}

void client_data::actualize_content_after_mixery(
	boost::uint16_t bytes_transferred)
{
	// Usuń shift pierwszych elementów z kolejki:
	boost::uint16_t shift = bytes_transferred / 2;
	pop_front_sequence(shift);
	// Zaktualizuj rozmiar kolejki:
	queue_size_ = queue_.size();
	// Zaktualizuj minimalną ilość bytów w kolejce:
	min_bytes_ = std::min(min_bytes_, queue_size_);
}

void client_data::actualize_content_after_upload(
	std::vector<boost::int16_t>& upload_data)
{
	const size_t upload_data_size = upload_data.size();
	// Jeżeli po złączeniu kolejek, długość tej nowopowstałej
	// będzie dłuższa od dopuszczalnej przyjętej w konstruktorze to:
	if (queue_size_ + upload_data_size > max_queue_length_) {
		// -> usuń nadmiar z początku kolejki:
		boost::uint16_t shift = 
			max_queue_length_ - (queue_size_ + upload_data_size);
		pop_front_sequence(shift);
	}
	// Niezależnie od przypadku zmerguj queue_ z kolejką z UPLOAD'a:
	queue_.insert(queue_.end(), upload_data.begin(), upload_data.end());
	// Zaktualizuj rozmiar kolejki:
	queue_size_ = queue_.size();
	// Zaktualizuj maksymalną liczbę bajtów w kolejce:
	max_bytes_ = std::max(max_bytes_, queue_size_);
}

std::string client_data::print_statistics()
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
		"\n"
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

void client_data::pop_front_sequence(boost::uint16_t shift)
{
	std::vector<decltype(queue_)::value_type>(
		queue_.begin() + shift, queue_.end()
	).swap(queue_);
}

void client_data::reset_statistics()
{
	// Zainicjuj min_bytes_ i max_bytes_ aktualnym rozmiarem kolejki:
	min_bytes_ = static_cast<boost::uint16_t>(queue_size_);
	max_bytes_ = static_cast<boost::uint16_t>(queue_size_);
}