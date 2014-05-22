#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP

#include <boost/cstdint.hpp>

#include <cstring>
#include <vector>

#include "../exceptions/invalid_header_exception.hpp"

//=============================================================================
//
// Struktura odpowiedzialna za transfer danych z kolejki o stanie ACTIVE
//
//=============================================================================
struct mixer_input
{
	// Wskaźnik na dane w FIFO
	void* data;
	// Liczba dostępnych bajtów
	size_t len;
	// Wartość ustawiana przez mikser,
	// wskazująca ile bajtów należy usunąć z FIFO
	size_t consumed;
};


//=============================================================================
//
// Struktura opakowująca argumenty wywołania programu,
// które zostaną przekazane do parsera.
//
//=============================================================================
struct program_parametres
{
	program_parametres(int argc, char** argv) 
		: _argc(argc), _argv(argv)
	{
	}

	int _argc;
	char** _argv;
};


//=============================================================================
//
// Struktura przechowująca informacje dotyczące nagłówka wiadomości.
//
//=============================================================================
struct header_data
{
	header_data(const std::string& header_name, 
		const std::vector<boost::uint16_t>& param_list) 
	: _header_name(header_name), _param_list(param_list)
	{
	}

	const std::string _header_name;
	const std::vector<boost::uint16_t> _param_list;
};


//=============================================================================
//
// Struktury nagłówkowe.
//
//=============================================================================
struct base_header
{
	// Konstruktor przyjmujący jako nazwę rodzaj wiadomości
	// Przykład:
	//
	// rodzaj wiadomości: 'UPLOAD', 'DATA'
	base_header()
	{
	}

	void set_parametres(const header_data& data) {}
	
	//virtual base_header get_instance();
	virtual base_header get_instance() {}

	std::string _header_name;
};

// Nagłówek typu: CLIENT [clientid]
struct client_header: public base_header
{
	client_header()
	{
	}

	virtual void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 1)
			throw invalid_header_exception();

		_client_id = data._param_list[0];
	}

	base_header get_instance()
	{
		return client_header();
	}

	boost::uint16_t _client_id;
};

// Nagłówek typu: UPLOAD [nr]
struct upload_header: public base_header
{
	upload_header()
	{
	}

	virtual void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 1)
			throw invalid_header_exception();

		_nr = data._param_list[0];
	}

	base_header get_instance()
	{
		return upload_header();
	}

	boost::uint16_t _nr;
};

// Nagłówek typu: DATA [nr] [ack] [win]
struct data_header: public base_header
{
	data_header()
	{
	}

	virtual void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 3)
			throw invalid_header_exception();

		_nr = data._param_list[0];
		_ack = data._param_list[1];
		_win = data._param_list[2];
	}

	base_header get_instance()
	{
		return data_header();
	}

	boost::uint16_t _nr;
	boost::uint16_t _ack;
	boost::uint16_t _win;
};

// Nagłówek typu: ACK [ack] [win]
struct ack_header: public base_header
{
	ack_header()
	{
	}

	virtual void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 2)
			throw invalid_header_exception();

		_ack = data._param_list[0];
		_win = data._param_list[1];
	}

	base_header get_instance()
	{
		return ack_header();
	}

	boost::uint16_t _ack;
	boost::uint16_t _win;
};

// Nagłówek typu: RETRANSMIT [nr]
struct retransmit_header: public base_header
{
	retransmit_header()
	{
	}

	virtual void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 1)
			throw invalid_header_exception();

		_nr = data._param_list[0];
	}
	
	base_header get_instance()
	{
		return retransmit_header();
	}

	boost::uint16_t _nr;
};

// Nagłówek typu: KEEPALIVE
struct keepalive_header: public base_header
{
	keepalive_header()
	{
	}

	virtual void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 0)
			throw invalid_header_exception();
	}

	base_header get_instance()
	{
		return keepalive_header();
	}
};

#endif
