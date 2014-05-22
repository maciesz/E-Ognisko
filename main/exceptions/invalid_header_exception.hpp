#ifndef INVALID_HEADER_EXCEPTION_HPP
#define INVALID_HEADER_EXCEPTION_HPP

#include <exception>

// Wyjątek zgłaszany przy odbiorze niepopoprawnie sformatowanego nagłówka
// podczas komunikacji na linii klient-serwer.
class invalid_header_exception : public std::exception
{
};

#endif