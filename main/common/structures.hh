#ifndef STRUCTURES_HH
#define STRUCTURES_HH

#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>

#include "../exceptions/invalid_header_exception.hh"

//===========================================================================//
//                                                                           //
// Struktura przechowująca dane z kolejki o stanie ACTIVE.                   //
//                                                                           //
//===========================================================================//
struct mixer_input
{
	/// Wskaźnik na dane w FIFO
	void* data;
	/// Liczba dostępnych bajtów
	size_t len;
	/// Wartość ustawiana przez mikser,
	/// wskazująca ile bajtów należy usunąć z FIFO
	size_t consumed;
};


//===========================================================================//
//                                                                           //
// Struktura opakowująca argumenty wywołania programu,                       //
// które zostaną przekazane do parsera.                                      //
//                                                                           //
//===========================================================================//
struct program_parametres
{
	program_parametres(int argc, char** argv) 
		: _argc(argc), _argv(argv)
	{
	}

	int _argc;
	char** _argv;
};


//===========================================================================//
//                                                                           //
// Struktura przechowująca informacje dotyczące nagłówka wiadomości.         //
//                                                                           //
//===========================================================================//
struct header_data
{
	header_data(const std::string& header_name, 
		const std::vector<size_t>& param_list) 
	: _header_name(header_name), _param_list(param_list)
	{
	}

	const std::string _header_name;
	const std::vector<size_t> _param_list;
};


//===========================================================================//
//                                                                           //
// Struktury nagłówkowe.                                                     //
//                                                                           //
//===========================================================================//
struct base_header
{
	// Konstruktor przyjmujący jako nazwę rodzaj wiadomości
	// Przykład:
	//
	// rodzaj wiadomości: 'UPLOAD', 'DATA'
	base_header()
	{
	}

	virtual ~base_header()
	{
	}

	virtual void set_parametres(const header_data& data) = 0;

	virtual base_header* get_instance() = 0;

	std::string _header_name;
};

/// Nagłówek typu: CLIENT [clientid]
struct client_header: public base_header
{
	client_header()
	{
	}

	void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 1)
			throw invalid_header_exception();

		_client_id = data._param_list[0];
	}

	base_header* get_instance()
	{
		return new client_header;
	}

	size_t _client_id;
};

/// Nagłówek typu: UPLOAD [nr]
struct upload_header: public base_header
{
	upload_header()
	{
	}

	void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 1)
			throw invalid_header_exception();

		_nr = data._param_list[0];
	}

	base_header* get_instance()
	{
		return new upload_header;
	}

	size_t _nr;
};

/// Nagłówek typu: DATA [nr] [ack] [win]
struct data_header: public base_header
{
	data_header()
	{
	}

	void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 3)
			throw invalid_header_exception();

		_nr = data._param_list[0];
		_ack = data._param_list[1];
		_win = data._param_list[2];
	}

	base_header* get_instance()
	{
		return new data_header;
	}

	size_t _nr;
	size_t _ack;
	size_t _win;
};

/// Nagłówek typu: ACK [ack] [win]
struct ack_header: public base_header
{
	ack_header()
	{
	}

	void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 2)
			throw invalid_header_exception();

		_ack = data._param_list[0];
		_win = data._param_list[1];
	}
	
	base_header* get_instance()
	{
		return new ack_header;
	}

	size_t _ack;
	size_t _win;
};

/// Nagłówek typu: RETRANSMIT [nr]
struct retransmit_header: public base_header
{
	retransmit_header()
	{
	}

	void set_parametres(const header_data& data)
	{
		_header_name = data._header_name;

		if (data._param_list.size() != 1)
			throw invalid_header_exception();

		_nr = data._param_list[0];
	}
	
	base_header* get_instance()
	{
		return new retransmit_header;
	}

	size_t _nr;
};

/// Nagłówek typu: KEEPALIVE
struct keepalive_header: public base_header
{
	keepalive_header()
	{
	}

	void set_parametres(const header_data& data)
	{
		//std::cout << "KEEPALIVE\n";
		_header_name = data._header_name;

		if (data._param_list.size() != 0)
			throw invalid_header_exception();
	}

	base_header* get_instance()
	{
		return new keepalive_header;
	}
};


//===========================================================================//
//                                                                           //
// Struktura przechowująca informacje o UPLOAD'zie klienta.                  //
//                                                                           //
//===========================================================================// 
struct retransmit_dgram
{
	retransmit_dgram(const std::string& dgram_msg, size_t dgram_nr) 
	: _message(dgram_msg), _dgram_nr(dgram_nr)
	{
	}

	const std::string _message;
	const size_t _dgram_nr; // Datagramy pochodzące od serwera
};


//===========================================================================//
//                                                                           //
// Struktura przechowująca treść w postaci nagłówka oraz ciała wiadomości.   //
//                                                                           //
//===========================================================================//
struct message_structure
{
	message_structure(const std::string& header, const std::string& body)
	: _header(header), _body(body)
	{
	}

	std::string _header;
	std::string _body;
};
#endif
